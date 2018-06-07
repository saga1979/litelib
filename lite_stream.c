#include "lite_stream.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>
#include "lite_log.h"
#include "lite_list.h"
#include "lite_file.h"


static lite_list *_io_err_handles = 0;
static pthread_mutex_t _io_lock = PTHREAD_MUTEX_INITIALIZER;

int	io_stream_registe_err_handle(IOErrHandle io_handle)
{
	if (lite_list_find_item(_io_err_handles, io_handle) != 0)
	{
		return -1;
	}
	pthread_mutex_lock(&_io_lock);
	_io_err_handles = lite_list_append(_io_err_handles, io_handle);
	pthread_mutex_unlock(&_io_lock);

	return _io_err_handles != 0 ? 0 : -1;
}

void	io_stream_unregiste_err_handle(IOErrHandle io_handle)
{
	pthread_mutex_lock(&_io_lock);
	_io_err_handles = lite_list_remove(_io_err_handles, io_handle);
	pthread_mutex_unlock(&_io_lock);
}
static void notify_io_err(const IOStream *io, int err)
{
	pthread_mutex_lock(&_io_lock);

	lite_list *tmp = _io_err_handles;

	while (tmp)
	{
		((IOErrHandle)tmp->data)(io, err);
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&_io_lock);
}
int	io_stream_write(const IOStream *io, const void *buf, size_t len, int *err)
{
	assert(io && buf);
	if (!io)
	{
		return -1;
	}

	if (!buf || len <= 0)
	{
		return 0;
	}
	if (err) 
	{
		*err = 0;
	}

	bool success = false;

	int err_no =0;

	switch (io->type)
	{
	case IO_TYPE_STD:
		{
			success = file_write_std(io->stream.file, (uint8_t*)buf, len, &err_no);
		}
		break;
	case IO_TYPE_RAW:
		{
			success = file_write_fd(io->stream.fd, (uint8_t*)buf, len, &err_no);
		}
		break;
	default:
		return -1;
	}

	if (!success)
	{
		if (err)
		{
			*err = err_no;
		}
		if (_io_err_handles != 0)
		{
			notify_io_err(io, err_no);
		}
		d_printf_errno();
		return -1;
	}

	return len;
}

int io_stream_read(const IOStream *io, void *buf, size_t to_read, int *err)
{
	assert(io && buf);

	if (!io)
	{
		return -1;
	}

	if (!buf)
	{
		return 0;
	}
	
	if (err)
	{
		*err = 0;
	}

	int reads = -1;


	switch (io->type)
	{
	case IO_TYPE_STD:
		{
			reads = fread(buf, 1, to_read, io->stream.file);
		}
		break;
	case IO_TYPE_RAW:
		{
			reads = read(io->stream.fd, buf, to_read);
		}
		break;
	default:
		return -1;
	}


	if ((io->type == IO_TYPE_RAW && reads != (int)to_read)
		||(io->type == IO_TYPE_STD && reads != (int)to_read && !feof(io->stream.file)) )
	{
		d_printf_err("reads:%d\n", reads);
		d_printf_errno();
		if (err)
		{
			*err = errno;
		}
		if (_io_err_handles != 0)
		{
			notify_io_err(io, errno);
		}
		return -1;
	}

	return reads;
}

int	io_stream_read_alloc(const IOStream *io, void **buf, size_t to_read,  int *err)
{
	if (0 == to_read)
	{
		return 0;
	}
	*buf = malloc(to_read);

	if (!*buf)
	{
		return -1;
	}

	int ret = io_stream_read(io, *buf, to_read, err);

	if (ret <= 0)
	{
		free(*buf);
	}
	return ret;
}

