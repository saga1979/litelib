#ifndef __dir_utils_h__
#define __dir_utils_h__

#include <bits/types.h>





int  		dir_tool_rmdir(const char *dir);

/**
 * 相当于mkdir -p [dir]，但是路径不支持多个‘/’结尾 
 * 如果路径已经存在，直接返回成功 
 */
int			dir_tool_mkdir(const char *dir, __mode_t mode);




#endif
