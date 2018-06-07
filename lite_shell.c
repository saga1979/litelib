#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

#include "lite_string.h"
#include "lite_shell.h"
#include "lite_log.h"

static pthread_mutex_t _popen_lock =  PTHREAD_MUTEX_INITIALIZER;

int	sh_excute_cmd(const char *cmd, bool sync)
{
	if (sync)
	{
		pid_t pid;
		int ret = 0;
		if ((pid = fork()) < 0)
		{
			return -1;
		}
		else if (0 == pid)
		{
			execl("/bin/sh", "sh", "-c", cmd, 0);
		}
		else
		{
			d_printf_msg("fork child [%d]\n", pid);
			while (waitpid(pid, &ret, 0) < 0)
			{
				if (errno != EINTR)
				{
					ret = -1;
					break;
				}
			}
		}

		return WEXITSTATUS(ret);
//		int ret = system(cmd);
//
//		return WEXITSTATUS(ret);
	}
	else
	{
		pid_t pid = fork();
		if (0 == pid)
		{
			system(cmd);
		}
		else if (pid < 0)
		{
			return errno;
		}
	}

	return 0;
}

typedef struct _PopenArgs
{
	const char *cmd;
	char **result;
}PopenArgs;

void* thread_excute(void *arg)
{

	return 0;
}
int sh_excute_and_fetch_result(const char *cmd, char **result, int wait_seconds)
{
	assert(result);
	if (result == 0)
	{
		return -1;
	}
	if (-1 == wait_seconds)
	{
		pthread_mutex_lock(&_popen_lock);
	}
	else
	{
		bool locked = false;
		while (wait_seconds-- > 0)
		{
			if (pthread_mutex_trylock(&_popen_lock) == 0)
			{
				locked = true;
				break;
			}
			sleep(1);
		}
		if (!locked)
		{
			return SHELL_CMD_RET_TIMEOUT;
		}
	}
	
	FILE *f = popen(cmd, "r");

	if (f == 0)
	{
		return errno;
	}
	char buf[1024] = "";

	size_t read_bytes = 0;

	char *tmp_result = 0;
	uint32_t total_read = 0;
	bool alloc_failed = false;
	while ((read_bytes = fread(buf, 1, 1024, f)) > 0)
	{
		tmp_result = (char *)realloc(*result, total_read + read_bytes);
		if (tmp_result == 0)
		{
			free(*result);
			alloc_failed = true;
			break;
		}

		*result = tmp_result;
		

		memcpy(*result + total_read, buf, read_bytes);
		total_read += read_bytes;
		//!最多不能大于1MB数据
		if (total_read > 1024 * 1024)
		{
			break;
		}
	}

	pclose(f);
	pthread_mutex_unlock(&_popen_lock);

	return ((alloc_failed || total_read == 0) ? -1 : 0);
}



int  sh_get_ret_in_lines(lite_list** list, const char* cmd)
{
	assert(list);
	if (list == 0)
	{
		return -1;
	}

	char *buf = 0;
	
	int ret = sh_excute_and_fetch_result(cmd, &buf, -1);

	if (ret != 0)
	{
		return ret;
	}

	*list = splite_buf_to_line(buf, '\n');
	free(buf);

	if (*list == 0)
	{
		return -1;
	}

	return 0;
}
