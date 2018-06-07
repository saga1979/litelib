#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <sys/vfs.h>
#include <dirent.h>
#include "lite_dir.h"
#include "lite_log.h"

#define	MAX_BUF_SIZE  	1024
#define LOG_KEEP_DAYS	7
#define DEFAULT_LOG_FILE  "/main_app.log"

static const off_t 		max_log_file_size = 1024 * 1024; //！最�?MB
static pthread_mutex_t 	file_lock = PTHREAD_MUTEX_INITIALIZER;
static bool				log_module_initialized = false;
static int				log_fd = -1;
static char 			log_buf[MAX_BUF_SIZE] = "";
static bool 			debug_color_revert = false;
static char *_last_log_msg = 0;
static char				_log_file_path[NAME_MAX] = "";
static char				*_log_file_dir = 0;

static const char* gen_log_file_path(const char *base_dir)
{
	assert(base_dir);
	if (!base_dir)
	{
		return 0;
	}
	time_t now = time(0); 

	//删除早期的日志
	time_t past = now - 60 * 60 * 24 * LOG_KEEP_DAYS;
	struct tm *tm_past = gmtime(&past);
	unsigned char past_str[9] = { 0 };
	snprintf(past_str, 8, "%.4d%.2d%.2d", tm_past->tm_year+1900, tm_past->tm_mon+1, tm_past->tm_mday);
	unsigned char file_name_rm[NAME_MAX] = { 0 };

	DIR *dir;
	struct dirent *ptr;

	if ((dir=opendir(base_dir)) == NULL)
	{
		d_printf_err("Open %s error...", base_dir);
	}
	else
	{
		while ((ptr=readdir(dir)) != NULL)
		{
			if(ptr->d_name[0] == 'm')
			{
				if(memcmp(past_str, (ptr->d_name) + 9, 8) >= 0)
				{
					snprintf(file_name_rm, NAME_MAX-1, "%s/%s", base_dir, ptr->d_name);
					unlink(file_name_rm);
				}
			}
			else
				continue;
		}
		closedir(dir);

	}
	//////////////

	struct tm *tm_now = gmtime(&now);
	if (tm_now != 0)
	{
		snprintf(_log_file_path, NAME_MAX - 1, "%s/main_app-%.4d%.2d%.2d.log",
				 base_dir, tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday);
	}
	else
	{
		return DEFAULT_LOG_FILE;
	}

	return _log_file_path;
}
const char* log_file_path(void)
{
	return _log_file_path;
}

int log_module_init(const char *base_dir)
{
	assert(base_dir);
	if (!base_dir)
	{
		return -1;
	}
	if (log_module_initialized) 
	{
		return 0;
	}
	if (dir_tool_mkdir(base_dir, S_IRWXU) != 0)
	{
		return -1;
	}
	log_fd = open(gen_log_file_path(base_dir), O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
	if (log_fd == -1)
	{
		return -1;
	}
	

	if (_log_file_dir != 0)
	{
		free(_log_file_dir);
	}
	_log_file_dir = (char *)calloc(1, strlen(base_dir) + 1);
	if (0 == _log_file_dir)
	{
		return -1;
	}
	strcpy(_log_file_dir, base_dir);

	log_module_initialized = true;
	return 0;
}

void log_module_finalize(void)
{
	d_printf_debug("日志模块终止\n");
	if (log_fd != -1)
	{
		close(log_fd);
		log_fd = -1;
	}
	log_module_initialized = false;
}

/**
 * 避免重复。不会连续记录重复的信息�? *
 * @author zf (10/28/2013)
 */

static int	write_log(const char *level, const char *msg)
{
	static unsigned int today_log_index = 1;
	if (!log_module_initialized)
	{
		return -1;
	}

	assert(msg != 0);
	if (msg == 0)
	{
		return -1;
	}

	if(log_fd == -1)
	{
		log_fd = open(log_file_path(), O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
		if (log_fd == -1)
		{
			return -1;
		}
	}
//	if (_last_log_msg != 0)
//	{
//		if (strcmp(_last_log_msg, msg) == 0)
//		{
//			return 0;
//		}
//		else
//		{
//			free(_last_log_msg);
//		}
//	}
//
//	_last_log_msg = (char *)malloc(strlen(msg) + 1);
//	if (0 == _last_log_msg)
//	{
//		return -1;
//	}
//	strcpy(_last_log_msg, msg);
	time_t now = time(0);
	struct tm *tm_now = gmtime(&now);
	static int year, month, day;
	//GPS校时之后如果日期发生变化，需要重新建立日志文件
	if(year != tm_now->tm_year || month != tm_now->tm_mon || day != tm_now->tm_mday)
	{
		close(log_fd);
		year = tm_now->tm_year;
		month = tm_now->tm_mon;
		day = tm_now->tm_mday;

		snprintf(_log_file_path, NAME_MAX - 1, DEFAULT_LOG_DIR"/main_app-%.4d%.2d%.2d.log",
					 tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday);
		log_fd = open(log_file_path(), O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
		if (log_fd == -1)
		{
			return -1;
		}
	}

	struct stat log_file_stat;
	int ret = fstat(log_fd, &log_file_stat);
	if (ret != 0)
	{
		return -1;
	}
	//日志文件过大，切换日志文件
	if (log_file_stat.st_size > max_log_file_size)
	{
		close(log_fd);
		//remove(log_file_path());
		unsigned char *old_log_name = log_file_path();
		unsigned char new_log_name[NAME_MAX+3] = { 0 };
		memcpy(new_log_name, old_log_name, strlen(old_log_name)-4);
		unsigned char tmp[8] = { 0 };
		sprintf(tmp, "_%d.log", today_log_index);
		strncat(new_log_name, tmp, strlen(tmp));
		rename(old_log_name, new_log_name);
		today_log_index++;
		log_fd = open(log_file_path(), O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
		if (log_fd == -1)
		{
			return -1;
		}

	}
//	time_t now = time(0);
//	struct tm *tm_now = gmtime(&now);
	char time_str[128] = "";
	strftime(time_str, 127, "[%Y-%m-%d %T]", tm_now);
//	snprintf(log_buf, MAX_BUF_SIZE - 1, "%s\t", time_str);

	ssize_t write_bytes = 0;
	pthread_mutex_lock(&file_lock);
	//! 如果是记录日志正文，就写入时间以及日志级�
	if (strlen(level))
	{
		write(log_fd, time_str, strlen(time_str));
		write(log_fd, level, strlen(level));
	}
	write_bytes = write(log_fd, msg, strlen(msg));
	pthread_mutex_unlock(&file_lock);

//	if (write_bytes <= 0)
//	{
//		printf_with_color(LL_MESG, BGC_BLACK, FGC_RED,  true,"%s@%s %s\n", __FILE__, __func__, strerror(errno));
//		struct statfs disk_stat;
//
//		if (statfs(_log_file_dir, &disk_stat) == 0)
//		{
//			printf_with_color(LL_MESG, BGC_BLACK, FGC_RED, true, "%s@disk space: free blocks[%u] block size[%u]\n",
//							  __FILE__, disk_stat.f_bfree, disk_stat.f_bsize);
//		}
//	}

	return 0;
}

void log_level_to_string(LOG_LEVEL level, unsigned char *level_string)
{
	switch (level)
	{
	case LL_MESG:
		memcpy(level_string, "[MESG]", 6);
		break;
	case LL_ALERT:
		memcpy(level_string, "[ALERT]", 7);
		break;
	case LL_CRIT:
		memcpy(level_string, "[CRIT]", 6);
		break;
	case LL_WARNING:
		memcpy(level_string, "[WARNING]", 9);
		break;
	case LL_ERR:
		memcpy(level_string, "[ERR]", 5);
		break;
	case LL_NOTICE:
		memcpy(level_string, "[NOTICE]", 8);
		break;
	case LL_INFO:
		memcpy(level_string, "[INFO]", 6);
		break;
	case LL_DEBUG:
		memcpy(level_string, "[DEBUG]", 7);
		break;
	case LL_NONE:
		memcpy(level_string, " ", 1);
		break;
	default:
		break;
	}
	return;
}

//const char* log_level_to_string(LOG_LEVEL level)
//{
//	switch (level)
//	{
//	case LL_MESG:
//		return "[MESG]";
//	case LL_ALERT:
//		return "[ALERT]";
//	case LL_CRIT:
//		return "[CRIT]";
//	case LL_WARNING:
//		return "[WARNING]";
//	case LL_ERR:
//		return "[ERR]";
//	case LL_NOTICE:
//		return "[NOTICE]";
//	case LL_INFO:
//		return "[INFO]";
//	case LL_DEBUG:
//		return "[DEBUG]";
//	case LL_NONE:
//		return " ";
//	default:
//		return 0;
//	}
//}

int 	printf_with_color(LOG_LEVEL level, BACK_GROUND_COLOR bc, FORE_GROUND_COLOR fc,bool print_level, const char *format, ...)
{
	char msg[MAX_BUF_SIZE] = { 0 };

	va_list vl;
	va_start(vl, format);
	vsnprintf(msg, MAX_BUF_SIZE - 1, format, vl);
	va_end(vl);

	unsigned char log_level[10] = { 0 };
	if (print_level)
	{
		log_level_to_string(level, log_level);

		time_t now = time(0);
		struct tm *tm_now = gmtime(&now);
		char time_str[128] = "";
		strftime(time_str, 127, "%Y-%m-%d %T", tm_now);
		snprintf(log_buf, MAX_BUF_SIZE - 1, "%s", time_str);
		if (log_level)
		{
			printf("[%s] ", log_buf);//![time]
		}
	}
	switch (level)
	{
	case LL_MESG:
	case LL_ALERT:
	case LL_CRIT:
	case LL_WARNING:
	case LL_ERR:
		{
#ifndef _NO_PRINT_LOG
			if (print_level)
			{
				printf("\033[%d;%d;5m%s %s\033[0m\n", bc, fc, log_level, msg);
			}
			else
			{
				printf("\033[%d;%d;5m%s\033[0m", bc, fc, msg);
			}
#endif
			return write_log(log_level, msg);
		}
	case LL_NOTICE:
	case LL_INFO:
	case LL_DEBUG:
	case LL_NONE:
		{
#ifndef _NO_PRINT_LOG
			if (print_level)
			{
				printf("\033[%d;%d;40m%s %s\033[0m\n", bc, fc, log_level, msg);
			}
			else
			{
				printf("\033[%d;%d;40m%s\033[0m", bc, fc, msg);
			}
#endif
			return write_log(log_level, msg);
		}
	default:
		break;
	}
	return 0;
}

bool	is_debug_color_revert(void)
{
	debug_color_revert = !debug_color_revert;
	return debug_color_revert;
}


