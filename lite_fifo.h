#ifndef __lite_fifo_h__
#define __lite_fifo_h__

#include <stdint.h>

typedef struct _FIFOBuf FIFOBuf;

/**
 * 
 * 
 * @author zf (4/23/2014)
 * 
 * @param size FIFO的大小
 * 
 * @return FIFOBuf* 
 *  	   如果失败，返回0,否则返回分配的FIFO 
 */
FIFOBuf* 	lite_fifo_buf_new(size_t size);

int	 		lite_fifo_buf_write(FIFOBuf* fifo,void* data, int len);
/**
 * 不要忘记释放数据占用的内存 
 * @author zf (2/24/2014)
 * @param len -1表示读取所有数据
 * 
 * @return int 
 */
int	 		lite_fifo_buf_read(FIFOBuf* fifo, void** data);

/**
 * 删除FIFO 
 *  
 * @author zf (4/23/2014)
 */
void	 	lite_fifo_buf_del(FIFOBuf **);


#endif
