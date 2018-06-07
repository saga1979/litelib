/** 
 * 处理IO操作以及出现的IO错误 
 * 所有的磁盘相关操作（读、写）必须通过此接口进行。 
 * 禁止调用系统接口（read、write） 
 */
#ifndef __io_stream_h__
#define __io_stream_h__

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

typedef enum _IOType
{
	IO_TYPE_STD,//! 标准库IO
	IO_TYPE_RAW,//! 系统IO接口
}IOType;

typedef struct _IOStream
{
	IOType type;
	union
	{
		FILE *file;
		int	fd;
	}stream;
}IOStream;


typedef void (*IOErrHandle)(const IOStream* io_stream, int err_no);
/**
 * 调用者自行考虑多线程的安全问题
 */
int		io_stream_registe_err_handle(IOErrHandle io_handle);

void	io_stream_unregiste_err_handle(IOErrHandle io_handle);
/**
 * 此接口是根据目前的记录方式而定（单线程记录所有的音视频通道），如果记录方式改变，
 * 就面临非单一线程读写带来的问题。
 */
int		io_stream_write(const IOStream *io, const void *buf, size_t len, int *err);

/**
 * 从IO对象中读取指定的数据到缓冲区
 * 
 * @author zf (4/23/2014)
 * 
 * @param io IO对象
 * @param buf 已分配内存的缓冲区
 * @param to_read 读取多少个字节
 * @param err 
 * 
 * @return int 
 */
int		io_stream_read(const IOStream *io, void *buf, size_t to_read, int *err);
/*与上个接口的区别就是：数据缓冲区由此函数自行分配（所以记得后续要释放）*/
int		io_stream_read_alloc(const IOStream *io, void **buf, size_t to_read,  int *err);

#endif
