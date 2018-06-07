#ifndef __log_helper_h__
#define __log_helper_h__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum _FORE_GROUND_COLOR
{
	FGC_BLACK = 30,
	FGC_RED,
	FGC_GREEN,
	FGC_BROWN,
	FGC_BLUE,
	FGC_PURPLE,
	FGC_CYAN,
	FGC_WHITE

}FORE_GROUND_COLOR;

typedef enum _BACK_GROUND_COLOR
{
	BGC_BLACK = 40,
	BGC_RED,
	BGC_GREEN,
	BGC_BROWN,
	BGC_BLUE,
	BGC_PURPLE,
	BGC_CYAN,
	BGC_WHITE

}BACK_GROUND_COLOR;


typedef enum _LOG_LEVEL
{
	LL_MESG,
	LL_ALERT,
	LL_CRIT,
	LL_WARNING,
	LL_ERR,
	LL_NOTICE,
	LL_INFO,
	LL_DEBUG,
	LL_NONE
}LOG_LEVEL;

#define LOG_FILE_NAME "main_app.log"
#define DEFAULT_LOG_DIR	"/usr/local/log"
/**
 * 这个函数的打印功能可以被预处理宏（_NO_PRINT_LOG）屏蔽
 * 
 * @author zf (10/28/2013)
 * 
 * @param level 
 * @param bc 
 * @param fc 
 * @param format 
 * 
 * @return int 
 */
int 		printf_with_color(LOG_LEVEL level, BACK_GROUND_COLOR bc, FORE_GROUND_COLOR fc, bool print_level, const char* format, ...);

//!zf 日志中的信息，超出需要记录的级别必须记录



#define LOG_HEADER_STR "%s@%s:%d:\n", __FILE__, __func__, __LINE__
#define PRINT_LOG_HEADER() printf_with_color(LL_DEBUG, BGC_BLACK, FGC_WHITE,false, LOG_HEADER_STR)

/**
 * 条件判断时，使用下面的宏定义必须使用"{}"（这条语句实际被扩展为两条），例： 
 * if(xx) 
 * { 
 *  	d_printf_err("xx is true.\n");
 * }
 */
#define d_printf_crit(...) PRINT_LOG_HEADER();printf_with_color(LL_CRIT, BGC_BLACK, FGC_RED, true, __VA_ARGS__)
#define d_printf_err(...) PRINT_LOG_HEADER();printf_with_color(LL_ERR, BGC_BLACK, FGC_RED, true, __VA_ARGS__)

#define d_printf_warning(...) PRINT_LOG_HEADER(); \
printf_with_color(LL_WARNING, BGC_BLACK, FGC_BROWN, true, __VA_ARGS__)

#define d_printf_errno()	PRINT_LOG_HEADER();printf_with_color(LL_ERR, BGC_BLACK, FGC_RED, true, "%s\n", strerror(errno))

#define d_printf_log(...) PRINT_LOG_HEADER(); printf_with_color(LL_NOTICE, BGC_BLACK, FGC_GREEN, true, __VA_ARGS__)

#ifdef _DEBUG
#define d_printf_with_color	 printf_with_color
#define d_printf_debug(...) PRINT_LOG_HEADER(); printf_with_color(LL_DEBUG, BGC_BLACK, FGC_WHITE, true,__VA_ARGS__)
#define d_printf_msg(...) PRINT_LOG_HEADER(); printf_with_color(LL_INFO, BGC_BLUE,FGC_GREEN , true, __VA_ARGS__)
#define d_assert(value, express) \
	if((value) != express)\
	{\
		d_printf_err("%s@%s:%d---->value:0x%X", __func__, __FILE__, __LINE__,  (value));\
		assert((value) == express);\
	}
#define d_print_if(value, express, msg)\
		if((value) == (express))\
		{\
			d_printf_debug("%s\n", msg);\
		}
#else
#define d_printf_debug(...)
#define d_printf_msg(...)
#define d_assert(value, express)
#define d_print_if(value,express,msg)
#define d_printf_with_color(...)
#endif


#define return_int_value_ifn(value, express)\
{\
	int __ret__ = (value);\
	if(__ret__ != (express))\
	{\
		d_printf_debug("%s@%s:%d---->value:0x%X", __func__, __FILE__, __LINE__,  __ret__);\
		return __ret__;\
	}\
}


#define return_int_value_ifn_with_info(value, express, info)\
{\
	int __ret__ = (value);\
	if(__ret__!= (express))\
	{\
		d_printf_debug("%s@%s@%s:%d---->value:0x%X",info, __func__, __FILE__, __LINE__,  __ret__);\
		return __ret__;\
	}\
}

#define print_int_value_ifn(value, express)\
{\
	int __ret__ = (value);\
	if(__ret__ != (express))\
	{\
		d_printf_debug("%s@%s:%d---->value:0x%X", __func__, __FILE__, __LINE__,  __ret__);\
	}\
}


int 		log_module_init(const char* base_dir);
void 		log_module_finalize(void);
const char* log_file_path(void);
//bool		is_debug_color_revert(void);

#endif
