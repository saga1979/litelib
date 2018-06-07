/**
 * 通用链表
 */
#ifndef __lite_list_h__
#define __lite_list_h__

#include <stdbool.h>

struct _lite_list
{
	void *data;
	struct _lite_list  *pre;
	struct _lite_list  *next;
};

typedef struct _lite_list lite_list;

typedef void (*LiteListFunc)(void *data);
typedef void (*LiteListDestroy)(void *data);
typedef bool (*LiteListComp)(const void* d1, const void *d2);

lite_list* 		lite_list_new(void);
/**
 * 删除链表中第一个数据匹配传递的参数的节点，并释放此节点占用的内存。
 * 
 * @author zf (12/27/2013)
 * 
 * @param list 
 * @param data 需要自行删除
 * 
 * @return lite_list* 
 */
lite_list*		lite_list_remove(lite_list *list, void *data);

lite_list*		lite_list_remove_all(lite_list *list, void *data);

lite_list* 		lite_list_remove_node(lite_list *list, lite_list *node);

lite_list* 		lite_list_delete_node(lite_list *list, lite_list *node);

void 			lite_list_foreach(lite_list *list, LiteListFunc lf);

lite_list* 		lite_list_find_item(lite_list *list, void* data);

lite_list*		lite_list_find_item_ex(lite_list *list, void* data, LiteListComp comp);

lite_list*		lite_list_last(lite_list *list);

lite_list*		lite_list_nth(lite_list *list, int n);

lite_list* 		lite_list_append_node(lite_list *list, lite_list *new_node);

lite_list*		lite_list_append(lite_list *list, void *data);

void			lite_list_destroy(lite_list *list, LiteListDestroy free_func);
/**
 * 注意：只free节点，以及对节点数据简单调用一次free（调用者要确保此方法合适）
 * 
 * @author zf (12/26/2013)
 * 
 * @param list 
 */
void			lite_list_destroy_simple(lite_list *list);

#endif
