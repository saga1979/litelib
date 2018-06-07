#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include "lite_hash.h"


#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef	MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


#define HASH_TABLE_MIN_SHIFT 3  /* 1 << 3 == 8 buckets */

#define UNUSED_HASH_VALUE 0
#define TOMBSTONE_HASH_VALUE 1
#define HASH_IS_UNUSED(h_) ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_) ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_) ((h_) >= 2)


static pthread_mutex_t _atomic_lock = PTHREAD_MUTEX_INITIALIZER;

bool lite_atomic_int_dec_and_test(volatile int *atomic)
{
	bool is_zero = false;
	pthread_mutex_lock(&_atomic_lock);
	is_zero = --(*atomic) == 0;
	pthread_mutex_unlock(&_atomic_lock);
	return is_zero;
}

int lite_atomic_int_inc(volatile int *atomic)
{
	pthread_mutex_lock(&_atomic_lock);
	++(*atomic);
	pthread_mutex_unlock(&_atomic_lock);
	return *atomic;
}

struct _LiteHashTable
{
	int             	size;
	int             	mod;
	uint32_t          mask;
	int             	nnodes;
	int             	noccupied;  /* nnodes + tombstones */
	void **keys;
	uint32_t          *hashes;
	void **values;

	LiteHashFunc      hash_func;
	LiteEqualFunc     key_equal_func;
	int             	ref_count;
	LiteDestroyNotify key_destroy_func;
	LiteDestroyNotify value_destroy_func;
};

typedef struct
{
	LiteHashTable  	*hash_table;
	void *dummy1;
	void *dummy2;
	int          		position;
	bool     		dummy3;
	int          		version;
} RealIter;

static const int prime_mod[] =
{
	1,          /* For 1 << 0 */
	2,
	3,
	7,
	13,
	31,
	61,
	127,
	251,
	509,
	1021,
	2039,
	4093,
	8191,
	16381,
	32749,
	65521,      /* For 1 << 16 */
	131071,
	262139,
	524287,
	1048573,
	2097143,
	4194301,
	8388593,
	16777213,
	33554393,
	67108859,
	134217689,
	268435399,
	536870909,
	1073741789,
	2147483647  /* For 1 << 31 */
};

static void lite_hash_table_remove_all_nodes(LiteHashTable *hash_table, bool    notify)
{
	hash_table->nnodes = 0;
	hash_table->noccupied = 0;

	if (!notify ||
		(hash_table->key_destroy_func == NULL &&
		 hash_table->value_destroy_func == NULL))
	{
		memset(hash_table->hashes, 0, hash_table->size * sizeof(uint32_t));
		memset(hash_table->keys, 0, hash_table->size * sizeof(void *));
		memset(hash_table->values, 0, hash_table->size * sizeof(void *));

		return;
	}

	for (int i = 0; i < hash_table->size; i++)
	{
		if (HASH_IS_REAL(hash_table->hashes[i]))
		{
			void *key = hash_table->keys[i];
			void *value = hash_table->values[i];

			hash_table->hashes[i] = UNUSED_HASH_VALUE;
			hash_table->keys[i] = NULL;
			hash_table->values[i] = NULL;

			if (hash_table->key_destroy_func != NULL)
			{
				hash_table->key_destroy_func(key);
			}

			if (hash_table->value_destroy_func != NULL)
			{
				hash_table->value_destroy_func(value);
			}
		}
		else if (HASH_IS_TOMBSTONE(hash_table->hashes[i]))
		{
			hash_table->hashes[i] = UNUSED_HASH_VALUE;
		}
	}
}

static void lite_hash_table_set_shift(LiteHashTable *hash_table, int shift)
{

	uint32_t mask = 0;

	hash_table->size = 1 << shift;
	hash_table->mod  = prime_mod[shift];

	for (int i = 0; i < shift; i++)
	{
		mask <<= 1;
		mask |= 1;
	}

	hash_table->mask = mask;
}
static int lite_hash_table_find_closest_shift(int n)
{
	int i;
	for (int i = 0; n; i++)
		n >>= 1;

	return i;
}

static void lite_hash_table_set_shift_from_size(LiteHashTable *hash_table, int size)
{
	int shift = lite_hash_table_find_closest_shift(size);
	shift = MAX(shift, HASH_TABLE_MIN_SHIFT);

	lite_hash_table_set_shift(hash_table, shift);
}

static void lite_hash_table_resize(LiteHashTable *hash_table)
{
	lite_hash_table_set_shift_from_size(hash_table, hash_table->nnodes * 2);

	void **new_keys = (void **)malloc(sizeof(void *) * hash_table->size);

	void **new_values;

	if (hash_table->keys == hash_table->values)
	{
		new_values = new_keys;
	}
	else
	{
		new_values = (void **)malloc(sizeof(void *) * hash_table->size);
	}

	uint32_t *new_hashes = (uint32_t *)malloc(sizeof(void *) * hash_table->size);

	int old_size = hash_table->size;

	for (int i = 0; i < old_size; i++)
	{
		uint32_t node_hash = hash_table->hashes[i];
		uint32_t hash_val;
		uint32_t step = 0;

		if (!HASH_IS_REAL(node_hash)) continue;

		hash_val = node_hash % hash_table->mod;

		while (!HASH_IS_UNUSED(new_hashes[hash_val]))
		{
			step++;
			hash_val += step;
			hash_val &= hash_table->mask;
		}

		new_hashes[hash_val] = hash_table->hashes[i];
		new_keys[hash_val] = hash_table->keys[i];
		new_values[hash_val] = hash_table->values[i];
	}

	if (hash_table->keys != hash_table->values)
	{
		free(hash_table->values);
	}

	free(hash_table->keys);
	free(hash_table->hashes);

	hash_table->keys = new_keys;
	hash_table->values = new_values;
	hash_table->hashes = new_hashes;

	hash_table->noccupied = hash_table->nnodes;
}

static inline void  lite_hash_table_maybe_resize(LiteHashTable *hash_table)
{
	int noccupied = hash_table->noccupied;
	int size = hash_table->size;

	if ((size > hash_table->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT) ||
		(size <= noccupied + (noccupied / 16)))
	{
		lite_hash_table_resize(hash_table);
	}
}

LiteHashTable* lite_hash_table_new(LiteHashFunc hash_func, LiteEqualFunc key_equal_func)
{
	return lite_hash_table_new_full(hash_func, key_equal_func, NULL, NULL);
}

LiteHashTable* lite_hash_table_new_full(LiteHashFunc      hash_func,
										LiteEqualFunc     key_equal_func,
										LiteDestroyNotify key_destroy_func,
										LiteDestroyNotify value_destroy_func)
{
	LiteHashTable *hash_table =  (LiteHashTable *)malloc(sizeof(LiteHashTable));
	lite_hash_table_set_shift(hash_table, HASH_TABLE_MIN_SHIFT);
	hash_table->nnodes             = 0;
	hash_table->noccupied          = 0;
	hash_table->hash_func          = hash_func ? hash_func : lite_hash_direct_hash;
	hash_table->key_equal_func     = key_equal_func;
	hash_table->ref_count          = 1;
	hash_table->key_destroy_func   = key_destroy_func;
	hash_table->value_destroy_func = value_destroy_func;
	hash_table->keys               = (void **)calloc(hash_table->size, sizeof(void *));
	hash_table->values             = hash_table->keys;
	hash_table->hashes             = (uint32_t *)calloc(hash_table->size, sizeof(uint32_t));

	return hash_table;
}


void lite_hash_table_destroy(LiteHashTable *hash_table)
{
	if (hash_table == 0)
	{
		return;
	}

	lite_hash_table_remove_all(hash_table);
	lite_hash_table_unref(hash_table);
}

void lite_hash_table_remove_all(LiteHashTable *hash_table)
{
	if (hash_table == 0)
	{
		return;
	}


	lite_hash_table_remove_all_nodes(hash_table, true);
	lite_hash_table_maybe_resize(hash_table);
}

LiteHashTable* lite_hash_table_ref(LiteHashTable *hash_table)
{
	assert(hash_table);
	if (0 == hash_table)
	{
		return 0;
	}

	lite_atomic_int_inc(&hash_table->ref_count);

	return hash_table;
}

void lite_hash_table_unref(LiteHashTable *hash_table)
{
	if (hash_table == 0)
	{
		return;
	}

	if (lite_atomic_int_dec_and_test(&hash_table->ref_count))
	{
		lite_hash_table_remove_all_nodes(hash_table, true);
		if (hash_table->keys != hash_table->values)
		{
			free(hash_table->values);
		}
		free(hash_table->keys);
		free(hash_table->hashes);
		free(hash_table);
	}
}

static inline uint32_t
lite_hash_table_lookup_node(LiteHashTable    *hash_table,
							const void *key,
							uint32_t         *hash_return)
{
	uint32_t first_tombstone = 0;
	bool have_tombstone = false;
	uint32_t step = 0;

	uint32_t hash_value = hash_table->hash_func(key);
	if (!HASH_IS_REAL(hash_value)) hash_value = 2;

	*hash_return = hash_value;

	uint32_t node_index = hash_value % hash_table->mod;
	uint32_t node_hash = hash_table->hashes[node_index];

	while (!HASH_IS_UNUSED(node_hash))
	{
		/* We first check if our full hash values
		 * are equal so we can avoid calling the full-blown
		 * key equality function in most cases.
		 */
		if (node_hash == hash_value)
		{
			void *node_key = hash_table->keys[node_index];

			if (hash_table->key_equal_func)
			{
				if (hash_table->key_equal_func(node_key, key)) return node_index;
			}
			else if (node_key == key)
			{
				return node_index;
			}
		}
		else if (HASH_IS_TOMBSTONE(node_hash) && !have_tombstone)
		{
			first_tombstone = node_index;
			have_tombstone = true;
		}

		step++;
		node_index += step;
		node_index &= hash_table->mask;
		node_hash = hash_table->hashes[node_index];
	}

	if (have_tombstone) return first_tombstone;

	return node_index;
}

static void
lite_hash_table_insert_node(LiteHashTable *hash_table,
							uint32_t       node_index,
							uint32_t       key_hash,
							void *key,
							void *value,
							bool    keep_new_key,
							bool    reusinlite_key)
{


	if (hash_table->keys == hash_table->values && key != value)
	{

		hash_table->values = malloc(sizeof(void *) * hash_table->size);
		memcpy(hash_table->values, hash_table->keys, sizeof(void *) * hash_table->size);
	}

	uint32_t old_hash = hash_table->hashes[node_index];
	void *old_key = hash_table->keys[node_index];
	void *old_value = hash_table->values[node_index];

	if (HASH_IS_REAL(old_hash))
	{
		if (keep_new_key) hash_table->keys[node_index] = key;
		hash_table->values[node_index] = value;
	}
	else
	{
		hash_table->keys[node_index] = key;
		hash_table->values[node_index] = value;
		hash_table->hashes[node_index] = key_hash;

		hash_table->nnodes++;

		if (HASH_IS_UNUSED(old_hash))
		{
			/* We replaced an empty node, and not a tombstone */
			hash_table->noccupied++;
			lite_hash_table_maybe_resize(hash_table);
		}

	}

	if (HASH_IS_REAL(old_hash))
	{
		if (hash_table->key_destroy_func && !reusinlite_key) hash_table->key_destroy_func(keep_new_key ? old_key : key);
		if (hash_table->value_destroy_func) hash_table->value_destroy_func(old_value);
	}
}

static void
lite_hash_table_insert_internal(LiteHashTable *hash_table,
								void *key,
								void *value,
								bool    keep_new_key)
{
	assert(hash_table);
	uint32_t key_hash;
	if (hash_table == 0)
	{
		return;
	}

	uint32_t node_index = lite_hash_table_lookup_node(hash_table, key, &key_hash);

	lite_hash_table_insert_node(hash_table, node_index, key_hash, key, value, keep_new_key, false);
}

static void
lite_hash_table_remove_node(LiteHashTable   *hash_table,
							int          i,
							bool      notify)
{
	void *key = hash_table->keys[i];
	void *value = hash_table->values[i];

	/* Erect tombstone */
	hash_table->hashes[i] = TOMBSTONE_HASH_VALUE;

	/* Be GC friendly */
	hash_table->keys[i] = NULL;
	hash_table->values[i] = NULL;

	hash_table->nnodes--;

	if (notify && hash_table->key_destroy_func) hash_table->key_destroy_func(key);

	if (notify && hash_table->value_destroy_func) hash_table->value_destroy_func(value);

}

static bool
lite_hash_table_remove_internal(LiteHashTable    *hash_table,
								const void *key,
								bool       notify)
{
	assert(hash_table);
	uint32_t node_hash;

	if (0 == hash_table)
	{
		return false;
	}

	uint32_t node_index = lite_hash_table_lookup_node(hash_table, key, &node_hash);

	if (!HASH_IS_REAL(hash_table->hashes[node_index])) return false;

	lite_hash_table_remove_node(hash_table, node_index, notify);
	lite_hash_table_maybe_resize(hash_table);

	return true;
}

void lite_hash_table_insert(LiteHashTable *hash_table,
							void *key,
							void *value)
{
	lite_hash_table_insert_internal(hash_table, key, value, false);
}



bool lite_hash_table_remove(LiteHashTable    *hash_table,
							const void *key)
{
	return lite_hash_table_remove_internal(hash_table, key, true);
}



void lite_hash_table_foreach(LiteHashTable *hash_table, LHRFunc func, void *user_data)
{
	assert(hash_table);
	assert(func);
	if (0 == hash_table || 0 == func)
	{
		return;
	}


	for (int i = 0; i < hash_table->size; i++)
	{
		uint32_t node_hash = hash_table->hashes[i];
		void *node_key = hash_table->keys[i];
		void *node_value = hash_table->values[i];

		if (HASH_IS_REAL(node_hash)) (*func)(node_key, node_value, user_data);


	}
}

void* lite_hash_table_lookup (LiteHashTable    *hash_table, const void*  key)
{
	assert(hash_table);
	if(0 == hash_table)
	{
		return 0;
	}

  uint32_t node_hash;

  uint32_t node_index = lite_hash_table_lookup_node (hash_table, key, &node_hash);

  return HASH_IS_REAL (hash_table->hashes[node_index])
    ? hash_table->values[node_index]
    : 0;
}

void* lite_hash_table_find(LiteHashTable *hash_table,
						   LHRFunc     predicate,
						   void *user_data)
{
	assert(hash_table);
	assert(predicate);

	if (0 == hash_table || 0 == predicate)
	{
		return 0;
	}

	bool match = false;

	for (int i = 0; i < hash_table->size; i++)
	{
		uint32_t node_hash = hash_table->hashes[i];
		void *node_key = hash_table->keys[i];
		void *node_value = hash_table->values[i];

		if (HASH_IS_REAL(node_hash)) match = predicate(node_key, node_value, user_data);

		if (match) return node_value;
	}

	return 0;
}

uint32_t lite_hash_table_size(LiteHashTable *hash_table)
{
	assert(hash_table);
	if (0 == hash_table)
	{
		return 0;
	}

	return hash_table->nnodes;
}















/* Simple Hash functions. */


bool lite_hash_str_equal(const void *v1, const void *v2)
{
	const char *string1 = v1;
	const char *string2 = v2;

	return strcmp(string1, string2) == 0;
}


uint32_t lite_hash_str_hash(const void *v)
{
	const signed char *p;
	uint32_t h = 5381;

	for (p = v; *p != '\0'; p++)
		h = (h << 5) + h + *p;

	return h;
}


uint32_t lite_hash_direct_hash(const void *v)
{
	return ((uint32_t)(uint32_t)(v));
}


bool lite_hash_direct_equal(const void *v1, const void *v2)
{
	return v1 == v2;
}

bool lite_hash_int_equal(const void *v1, const void *v2)
{
	return *((const int *)v1) == *((const int *)v2);
}


uint32_t lite_hash_int_hash(const void *v)
{
	return *(const int *)v;
}


bool lite_hash_int64_equal(const void *v1, const void *v2)
{
	return *((const int64_t *)v1) == *((const int64_t *)v2);
}

uint32_t lite_hash_int64_hash(const void *v)
{
	return (uint32_t)*(const int64_t *)v;
}


bool lite_hash_double_equal(const void *v1, const void *v2)
{
	return *((const double *)v1) == *((const double *)v2);
}


uint32_t lite_hash_double_hash(const void *v)
{
	return (uint32_t)*(const double *)v;
}

