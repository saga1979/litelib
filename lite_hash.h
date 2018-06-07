/**
 * hash表，参考glib中的哈希表说明
 */
#ifndef __lite_hash_h__
#define __lite_hash_h__


#include <stdbool.h>
#include <stdint.h>

typedef uint32_t  (*LiteHashFunc)(const void *key);
typedef bool      (*LiteEqualFunc)(const void*  a,const void*  b);
typedef void      (*LiteDestroyNotify)       (void*       data);

typedef struct _LiteHashTable LiteHashTable;

typedef bool (*LHRFunc)(void *key, void *value, void *data);

typedef struct _LiteHashTableIter LiteHashTableIter;

struct _LiteHashTableIter
{
  /*< 私有数据，采用占位符 >*/
  void*      dummy1;
  void*      dummy2;
  void*      dummy3;
  int        dummy4;
  bool       dummy5;
  void*      dummy6;
};


LiteHashTable* lite_hash_table_new(LiteHashFunc hash_func, LiteEqualFunc key_equal_func);

LiteHashTable* lite_hash_table_new_full(LiteHashFunc hash_func,
                                            LiteEqualFunc  key_equal_func,
                                            LiteDestroyNotify  key_destroy_func,
                                            LiteDestroyNotify  value_destroy_func);
                                            
void		lite_hash_table_insert (LiteHashTable *hash_table,void*    key,void*    value) ;

bool 		lite_hash_table_remove (LiteHashTable  *hash_table, const void*  key);                                         
                                            
void        lite_hash_table_destroy(LiteHashTable *hash_table);

void        lite_hash_table_remove_all(LiteHashTable *hash_table);

void		lite_hash_table_unref (LiteHashTable *hash_table);

void 		lite_hash_table_foreach (LiteHashTable *hash_table, LHRFunc func, void* user_data);

void* 		lite_hash_table_lookup (LiteHashTable    *hash_table, const void*  key);

void* 		Lite_hash_table_find (LiteHashTable *hash_table,LHRFunc predicate,void* user_data);

uint32_t 	lite_hash_table_size (LiteHashTable *hash_table);


bool 		lite_hash_str_equal    (const void *  v1, const void *  v2);
uint32_t    lite_hash_str_hash     (const void *  v);

bool 		lite_hash_int_equal    (const void *  v1, const void *  v2);
uint32_t   	lite_hash_int_hash     (const void *  v);

bool	 	lite_hash_int64_equal  (const void *  v1, const void *  v2);
uint32_t    lite_hash_int64_hash   (const void *  v);

bool	 	lite_hash_double_equal (const void *  v1, const void *  v2);
uint32_t    lite_hash_double_hash  (const void *  v);

bool 		lite_hash_direct_equal (const void *  v1, const void *  v2) ;
uint32_t    lite_hash_direct_hash  (const void *  v) ;


#endif
