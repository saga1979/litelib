#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "lite_string.h"


lite_list* splite_buf_to_line(const char *buf, char split)
{
	lite_list *lines = 0;
	int splite_count = 0;
	int index = 0;
	int line_start = 0;
	int line_end = 0;
	while (buf[index] != 0)
	{
		line_start = index;
		while (buf[index] != split)
		{
			index++;
		}
		line_end = index;
		while (buf[index] == split)
		{
			index++;
		}
		int line_len = line_end - line_start;
		char *line = (char *)malloc(line_len + 1);
		if (0 != line)
		{
			memcpy(line, buf + line_start, line_len);
			line[line_len] = 0;
			lines = lite_list_append(lines, line);
		}
	}

	return lines;
}

char*	dup_valid_path(const char *path,  bool backslash_end)
{
	assert(path);
	if (path == 0)
	{
		return 0;
	}
	size_t len = strlen(path) + 1 + 1; //!多出来的1是保证后续可以加'/'
	char *ret_path = (char *)malloc(len);
	if (ret_path == 0)
	{
		return 0;
	}
	memset(ret_path, 0, len);
	int valid_index = 0;
	int index = 0;
	while (path[index] != 0)
	{
		if (path[index] == '/' && path[index + 1] == '/')
		{
			index++;
			continue;
		}
		ret_path[valid_index] = path[index];
		valid_index++;
		index++;
	}


	size_t real_len = strlen(ret_path);
	if (backslash_end)
	{
		if (ret_path[real_len - 1] != '/')
		{
			ret_path[real_len] = '/';
			ret_path[real_len + 1] = 0;
		}
	}
	else
	{
		if (ret_path[real_len - 1] == '/')
		{
			ret_path[real_len - 1] = 0;
		}
	}

	return ret_path;
}
