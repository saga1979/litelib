/**
 * 目前版本的uclib的timer_xxx一簇函数实现不完整， 
 * 只好写个简化的版本来应对。但此模块依赖system,而system老莫名其妙的阻塞. 
 */
#ifndef __lite_timer_h__
#define __lite_timer_h__

#include <stdint.h>

typedef int 	lite_timer_t;

typedef void 	(*lite_timer_triger_fun)(void *arg);

int		lite_timer_create(void *triger_arg, lite_timer_triger_fun fun, lite_timer_t* timer);

int 	lite_timer_delete(lite_timer_t timer);

int		lite_timer_start(lite_timer_t timer, int seconds);

int		lite_timer_stop(lite_timer_t timer, int *last_seconds);

void	lite_timer_finalize(void);

#endif
