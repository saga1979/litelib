#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "lite_shell.h"
#include "lite_dir.h"
#include "lite_log.h"


int  dir_tool_rmdir(const char *dir)
{
	char cmd[512] = "";

	snprintf(cmd, sizeof(cmd) - 1, "rm %s -Rf", dir);

	return sh_excute_cmd(cmd, true);
}

int	dir_tool_mkdir(const char *dir,__mode_t mode)
{
	assert(dir);
	
	if (dir == 0)
	{
		return -1;
	}
	if (access(dir, F_OK) == 0)
	{
		return 0;
	}
	char *tmp = strdup(dir);
	int len = strlen(tmp);
	if (tmp[len - 1] == '/')
	{
		tmp[len - 1] = 0;
	}
	int ret = 0;
	for (char* p = tmp + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;
			if ( (ret=mkdir(tmp, mode)) != 0 && errno != EEXIST)
			{
				break;
			}
			*p = '/';
		}
	}

	ret = mkdir(tmp, mode);

	free(tmp);

	if (ret != 0 && errno == EEXIST)
	{
		return 0;
	}

	return ret;
}
