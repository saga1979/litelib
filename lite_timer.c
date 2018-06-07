#include <time.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <malloc.h>
#include <limits.h>
#include <assert.h>
#include <sys/time.h>

#include "lite_timer.h"
#include "lite_log.h"

struct	_lite_timer_triger
{
	lite_timer_t			id;
	lite_timer_triger_fun	fun;
	void					*fun_arg;
	struct itimerval		timerval;
	bool					running;
	bool					remove_able;
};

typedef struct _lite_timer_triger lite_timer_triger;


struct _lite_list
{
	lite_timer_triger *triger;
	struct _lite_list  *pre;
	struct _lite_list  *next;
};

typedef struct _lite_list lite_list;

static lite_list *_timer_trigers = 0;

static bool _module_stop = false;

/**
 * 8是什么？ 
 * 是一个byte包含的bit 
 */
#define  MAX_BYTES 10
#define  MAX_ID 8*MAX_BYTES
static uint8_t _bit_map[MAX_BYTES] = { 0 };

static pthread_t		_check_pid = 0;
static pthread_mutex_t	_list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	_id_pool_lock = PTHREAD_MUTEX_INITIALIZER;

static  lite_list* list_new_node(void)
{
	lite_list *node = (lite_list *)calloc(1, sizeof(lite_list));
	if (node == 0)
	{
		return 0;
	}
	node->triger = (lite_timer_triger *)calloc(1, sizeof(lite_timer_triger));
	if (node->triger == 0)
	{
		free(node);
		return 0;
	}
	return node;
}

static lite_list* list_remove_node(lite_list* list, lite_list *node)
{
	if (pthread_mutex_lock(&_list_lock))
	{
		return list;
	}

	lite_list *tmp = list;

	if (node->pre)
	{
		node->pre->next = node->next;
	}

	if (node->next)
	{
		node->next->pre = node->pre;
	}

	if (node == list)
	{
		tmp = node->next;
	}

	pthread_mutex_unlock(&_list_lock);

	return tmp;
}
static lite_list* list_delete_node(lite_list* list, lite_list *node)
{
	if (node == 0 || list == 0)
	{
		return 0;
	}
	lite_list* ret = list_remove_node(list, node);
	free(node->triger);
	free(node);
	return ret;
}

typedef void (*LFunc)(void *data);

static void list_foreach(lite_list *list, LFunc lf)
{
	if (pthread_mutex_lock(&_list_lock))
	{
		return;
	}
	while (list)
	{
		lf(list->triger);
		list = list->next;
	}

	pthread_mutex_unlock(&_list_lock);
}
static lite_list* list_find_item(lite_list *list, lite_timer_t id)
{
	if (pthread_mutex_lock(&_list_lock))
	{
		return 0;
	}
	lite_list *tmp_list = list;

	while (tmp_list)
	{
		if (tmp_list->triger->id == id)
		{
			break;
		}
		tmp_list = tmp_list->next;
	}

	pthread_mutex_unlock(&_list_lock);
	return tmp_list;
}

//!多线程中，指示状态的函数本来就有时效性，这个函数目前只给list_append使用
static lite_list*	list_last(lite_list *list)
{
	if (list)
	{
		while (list->next)
		{
			list = list->next;
		}
	}
	return list;
}

static lite_list* list_append(lite_list *list, lite_list *new_node)
{
	assert(new_node);
	if (!new_node)
	{
		return list;
	}
	if (pthread_mutex_lock(&_list_lock)) 
	{
		return 0;
	}

	lite_list* ret = 0;
	if (list)
	{
		lite_list *last_node = list_last(list);
		//！ 因为list！=0,所以list_last()返回的结果也不会为0
		last_node->next = new_node;
		new_node->pre = last_node;
		
		ret = list;

	}
	else
	{
		new_node->pre = 0;
		new_node->next = 0;

		ret =  new_node;
	}


	pthread_mutex_unlock(&_list_lock);
	
	return ret;
}

static bool lite_timer_get_id(lite_timer_t *id)
{
	if (pthread_mutex_lock(&_id_pool_lock))
	{
		return false;
	}
	int byte_index = 0;

	while (byte_index < MAX_ID / 8)
	{
		uint8_t value = _bit_map[byte_index];
		int index = 0;
		while (index < 8)
		{
			if (((value >> index) & 0x01) == 0)
			{
				break;
			}
			index++;
		}
		if (index != 8)
		{
			*id = byte_index * 8 + index;
			uint8_t *pos = _bit_map + byte_index;
			*pos |= (0x01 << index);
			pthread_mutex_unlock(&_id_pool_lock);
			return true;
		}
		byte_index++;
	}
	pthread_mutex_unlock(&_id_pool_lock);
	return false;
}

static void lite_timer_release_id(lite_timer_t id)
{
	int byte_index = id / 8;
	int bit_index = id % 8;

	uint8_t *pos = _bit_map + byte_index;
	*pos &= ~(0x01 << bit_index);
}


static void* lite_timer_thread_fun(void *arg)
{
	d_printf_msg("创建线程@tid[%d] pid[%d]\n", pthread_self(), getpid());
	lite_timer_triger *triger = (lite_timer_triger *)arg;


	triger->fun(triger->fun_arg); //!运行指定的用户函数

	triger->remove_able = true;

	return 0;
}

static void check_fun(void *data)
{
	assert(data);
	if (!data)
	{
		return;
	}
	lite_timer_triger *triger = (lite_timer_triger *)data;
	if (!triger->running)
	{
		return;
	}
	struct timeval now;
	if (gettimeofday(&now, 0) != 0)
	{
		return;
	}

	if (now.tv_sec < triger->timerval.it_value.tv_sec)
	{
		return;
	}

	pthread_t tmp_tid;
	
	triger->running = false; //!防止重复执行
	pthread_create(&tmp_tid, 0, lite_timer_thread_fun, (void*)triger);
	d_printf_debug("tid=%u\n", tmp_tid);
	pthread_detach(tmp_tid);
}

static void* thread_check_timer(void *arg)
{
	d_printf_msg("创建线程@tid[%d] pid[%d]\n", pthread_self(), getpid());
	struct timeval tv;
	while (!_module_stop)
	{
		list_foreach(_timer_trigers, check_fun);

		//! 删除废弃的节点
		lite_list *list = _timer_trigers;
		while (list)
		{
			lite_list *node = 0;
			if (list->triger->remove_able)
			{
				node = list;
				_timer_trigers = list_remove_node(_timer_trigers, node);
			}

			list = list->next;
			if (node != 0)
			{
				free(node->triger);
				free(node);
			}
		}
		//gettimeofday(&tv, 0);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(0, 0, 0, 0, &tv) == -1)
		{
			sleep(1);
		}
	}

	return 0;
}



int	lite_timer_create(void *triger_arg, lite_timer_triger_fun fun, lite_timer_t *id)
{
	if (!lite_timer_get_id(id))
	{
		d_printf_err("get timer id.\n");
		return -1;
	}

	lite_list *new_triger = list_new_node();
	if (new_triger == 0)
	{
		return -1;
	}
	new_triger->triger->id = *id;
	new_triger->triger->fun_arg = triger_arg;
	new_triger->triger->fun = fun;
	new_triger->triger->running = false;
	new_triger->triger->remove_able = false;

	_timer_trigers = list_append(_timer_trigers, new_triger);

	return 0;
}

int lite_timer_delete(lite_timer_t id)
{
	if (id < 0)
	{
		return 0;
	}
	lite_list *node = list_find_item(_timer_trigers, id);

	if (node == 0)
	{
		return -1;
	}

	lite_timer_release_id(node->triger->id);
	_timer_trigers = list_delete_node(_timer_trigers, node);

	return 0;
}

int	lite_timer_start(lite_timer_t timer, int seconds)
{
	assert(timer >= 0);
	if (timer < 0)
	{
		return -1;
	}
	struct timeval now;
	if (gettimeofday(&now, 0) != 0)
	{
		return -1;
	}
	now.tv_sec += seconds;

	lite_list *node = list_find_item(_timer_trigers, timer);
	if (node == 0)
	{
		printf("no node with id:%d.\n", timer);
		return -1;
	}

	node->triger->timerval.it_value = now;
	node->triger->running = true;

	//! 如果检测线程没有启动，在这里将其启动
	if (_check_pid ==0 || pthread_kill(_check_pid, 0) == ESRCH)
	{
		if (pthread_create(&_check_pid, 0, thread_check_timer, 0) != 0)
		{
			return -1;
		}
	}

	return 0;
}

int	lite_timer_stop(lite_timer_t timer, int *last_seconds)
{
	lite_list *node = list_find_item(_timer_trigers, timer);
	if (node == 0)
	{
		return -1;
	}
	node->triger->running = false;
	return 0;
}

void	lite_timer_finalize(void)
{
	d_printf_debug("定时器终止\n");

	_module_stop = true;
	if (pthread_kill(_check_pid, 0) != ESRCH)
	{
		pthread_join(_check_pid, 0);
	}

	lite_list *tmp = _timer_trigers;
	while (tmp)
	{
		_timer_trigers = list_delete_node(_timer_trigers, tmp);
		if (_timer_trigers)
		{
			tmp = _timer_trigers->next; 
		}
	}
}

/*下面是测试代码，可单独测试*/
//#include <stdio.h>
//
//static bool _run = true;
//static void end_fun(void* arg)
//{
//	const char* msg = (const char*)arg;
//	printf("msg@%s\n", msg);
//	_run = false;
//}
//
//static void test_fun(void *arg)
//{
//	int id = (int)arg;
//	printf("id=%d\n", id);
//}
//
//int main()
//{
//	int id;
//
//	for(int i =0; i< 10; i++)
//	{
//		if (lite_timer_create(i, test_fun, &id) != 0)
//		{
//			printf("create timer failed:%d\n", i);
//			continue;
//		}
//		printf("create timer:%d\n", i);
//		lite_timer_start(id, i);
//	}
//	lite_timer_stop(6, 0);
//	printf("stop timer:%d\n", 6);
//	lite_timer_delete(8);
//	printf("delete timer:%d\n", 8);
//	if(lite_timer_create("hello, lite timer", end_fun, &id)!= 0)
//	{
//		printf("create lite timer failed.\n");
//		return -1;
//	}
//
//	printf("create timer:%d\n", id);
//
//	if(lite_timer_start(id, 10) !=0)
//	{
//		printf("start timer failed.\n");
//		return -1;
//	}
//
//	while(_run)
//	{
//		sleep(1);
//	}
//	return 0;
//}
//


