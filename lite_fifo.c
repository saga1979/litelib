#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>


#include "lite_fifo.h"

typedef struct _FIFOBuf
{
	uint8_t *buf;
	size_t	 buf_size;
	size_t   read_offset;
	size_t   write_offset;
	bool	 readable;
	pthread_mutex_t lock;
};


FIFOBuf* lite_fifo_buf_new(size_t size)
{
	FIFOBuf *fifo = (FIFOBuf *)malloc(sizeof(FIFOBuf));
	if (0 == fifo)
	{
		return 0;
	}
	fifo->buf = (uint8_t *)malloc(size);
	fifo->buf_size = size;
	fifo->read_offset = 0;
	fifo->write_offset = 0;
	fifo->readable = false;
	pthread_mutex_init(&fifo->lock, 0);

	return fifo;
}

int	 lite_fifo_buf_write(FIFOBuf *fifo, void *data, int len)
{
	assert(fifo && data);
	if (0 == fifo || 0 == data || len > (int)fifo->buf_size)
	{
		return -1;
	}
	pthread_mutex_lock(&fifo->lock);

	if (fifo->write_offset + len > fifo->buf_size)
	{
		memcpy(fifo->buf + fifo->write_offset, data, fifo->buf_size - fifo->write_offset);

		memcpy(fifo->buf, data + fifo->buf_size - fifo->write_offset, fifo->write_offset + len - fifo->buf_size);

		fifo->write_offset = fifo->write_offset + len - fifo->buf_size;
	}
	else
	{
		memcpy(fifo->buf + fifo->write_offset, data, len);

		fifo->write_offset += len;
	}

	fifo->readable = true;
	
	pthread_mutex_unlock(&fifo->lock);

	return len;
}

int	 lite_fifo_buf_read(FIFOBuf* fifo, void** data)
{
	assert(fifo && data);
	if (0 == fifo || 0 == data)
	{
		return -1;
	}
	if (!fifo->readable) 
	{
		return 0;
	}

	pthread_mutex_lock(&fifo->lock);

	size_t data_len = 0;

	if (fifo->read_offset < fifo->write_offset)
	{
		data_len = fifo->write_offset - fifo->read_offset;
		*data = malloc(data_len);
		if (0 == *data)
		{
			pthread_mutex_unlock(&fifo->lock);
			return -1;
		}
		memcpy(*data, fifo->buf + fifo->read_offset, data_len);
	}
	else
	{
		data_len = fifo->buf_size -(fifo->read_offset - fifo->write_offset);
		*data = malloc(data_len);

		if (0 == *data)
		{
			pthread_mutex_unlock(&fifo->lock);
			return -1;
		}


		memcpy((uint8_t*)*data, fifo->buf + fifo->read_offset, fifo->buf_size- fifo->read_offset);
		memcpy((uint8_t*)*data+(fifo->buf_size- fifo->read_offset), fifo->buf, fifo->write_offset);
	}

	fifo->read_offset = fifo->write_offset;
	fifo->readable = false;

	pthread_mutex_unlock(&fifo->lock);

	return data_len;

}

void	 lite_fifo_buf_del(FIFOBuf **fifo)
{
	if (0 == fifo)
	{
		return;
	}

	FIFOBuf *tmp = *fifo;
	if (0 == tmp)
	{
		return;
	}

	pthread_mutex_destroy(&tmp->lock);
	free(tmp->buf);

	*fifo = 0;
}
