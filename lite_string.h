#ifndef __string_utils_h__
#define __string_utils_h__

#include <stdbool.h>

#include "lite_list.h"

/**
 * 分割缓冲区成行
 * 
 * @author zf (4/25/2014)
 * 
 * @param buf 要分割的缓冲区
 * @param split 分割符
 * 
 * @return lite_list*  分割后的行列表
 */
lite_list* splite_buf_to_line(const char *buf, char split);

/**
 * 删除字符串中重复的'/'
 */
char*	dup_valid_path(const char *path, bool backslash_end);









#endif
