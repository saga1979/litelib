#include <stdlib.h>
#include <assert.h>
#include "lite_list.h"


lite_list* lite_list_new(void)
{
	lite_list *node = (lite_list *)calloc(1, sizeof(lite_list));

	return node;
}
lite_list*	lite_list_remove(lite_list *list, void *data)
{
	lite_list *tmp = list;

	while (tmp)
	{
		if (tmp->data == data)
		{
			if (tmp->pre)
			{
				tmp->pre->next = tmp->next;
			}
			if (tmp->next)
			{
				tmp->next->pre = tmp->pre;
			}
			if (list == tmp)
			{
				list = list->next;
			}
			free(tmp);
			break;
		}
		tmp = tmp->next;
	}
	return list;
}

lite_list*	lite_list_remove_all(lite_list *list, void *data)
{
	lite_list *tmp = list;

	while (tmp)
	{
		if (tmp->data == data)
		{
			if (tmp->pre)
			{
				tmp->pre->next = tmp->next;
			}
			if (tmp->next)
			{
				tmp->next->pre = tmp->pre;
			}
			if (list == tmp)
			{
				list = list->next;
			}
			free(tmp);
		}
		tmp = tmp->next;
	}
	return list;
}

lite_list* lite_list_remove_node(lite_list *list, lite_list *node)
{
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
	return tmp;
}
lite_list* lite_list_delete_node(lite_list *list, lite_list *node)
{
	if (node == 0 || list == 0)
	{
		return 0;
	}
	lite_list *ret = lite_list_remove_node(list, node);
	free(node);
	return ret;
}


void lite_list_foreach(lite_list *list, LiteListFunc lf)
{
	assert(lf);
	if (!list || !lf)
	{
		return;
	}
	while (list) 
	{
		lite_list *tmp_node = list;
		list = list->next;
		lf(tmp_node->data);
	}
}
lite_list* lite_list_find_item(lite_list *list, void *data)
{
	lite_list *tmp_list = list;

	while (tmp_list)
	{
		if (tmp_list->data == data)
		{
			break;
		}
		tmp_list = tmp_list->next;
	}
	return tmp_list;
}

lite_list*		lite_list_find_item_ex(lite_list *list, void* data, LiteListComp comp)
{
	lite_list *tmp_list = list;

	while (tmp_list)
	{
		if (comp(tmp_list->data, data))
		{
			break;
		}
		tmp_list = tmp_list->next;
	}
	return tmp_list;
}

lite_list*	lite_list_last(lite_list *list)
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

lite_list*		lite_list_nth(lite_list *list, int n)
{
	while (n-- > 0 && list)
	{
		list = list->next;
	}
	return list;
}

lite_list* lite_list_append_node(lite_list *list, lite_list *new_node)
{
	assert(new_node);
	if (0 == new_node)
	{
		return list;
	}

	new_node->next = 0;
	lite_list *ret = 0;
	if (list)
	{
		lite_list *last_node = lite_list_last(list);
		//！ 因为list！=0,所以list_last()返回的结果也不会为0
		last_node->next = new_node;
		new_node->pre = last_node;

		ret = list;

	}
	else
	{
		new_node->pre = 0;
		ret =  new_node;
	}

	return ret;
}

lite_list*		lite_list_append(lite_list *list, void *data)
{
	lite_list *node = (lite_list *)malloc(sizeof(lite_list));
	node->data = data;

	return lite_list_append_node(list, node);
}
static void lite_list_free(lite_list *list)
{
	lite_list *tmp = list;
	while (list)
	{
		tmp = list;
		list = list->next;
		free(tmp);
	}
}
void	lite_list_destroy(lite_list *list, LiteListDestroy free_func)
{
	if (free_func)
	{
		lite_list_foreach(list, free_func);
	}
	lite_list_free(list);
}

void	lite_list_destroy_simple(lite_list *list)
{
	lite_list *tmp_list = list;
	while (list)
	{
		tmp_list = list;
		list = list->next;
		free(tmp_list->data);
		free(tmp_list);
	}
}


