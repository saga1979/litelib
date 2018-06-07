#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include "lite_file.h"
#include "lite_log.h"

#define	MIN(a,b) (((a)<(b))?(a):(b))

#ifndef O_BINARY
#define O_BINARY 0
#endif


static bool get_contents_regfile(const char  *filename,
								 struct stat  *st,
								 int          fd,
								 uint8_t       **contents,
								 size_t        *length,
								 int      *error)
{
	size_t alloc_size = st->st_size + 1;
	char *buf = (char *)malloc(alloc_size);
	if (buf == 0)
	{
		close(fd);
		*error = errno;
		return false;
	}

	int bytes_read_total = 0;

	while (bytes_read_total < st->st_size)
	{
		int bytes_read = read(fd, buf + bytes_read_total, st->st_size - bytes_read_total);
		if (bytes_read < 0)
		{
			if (errno != EINTR)
			{
				*error = errno;
				close(fd);
				return false;
			}
		}
		else if (bytes_read == 0)
		{
			break;
		}
		else if (bytes_read > 0)
		{
			bytes_read_total += bytes_read;
		}
	}
	buf[bytes_read_total] = '\0';

	if (length)
	{
		*length = bytes_read_total;
	}
	*contents = buf;
	close(fd);
	return true;
}


static bool get_contents_stdio(const char  *filename,
							   FILE *file,
							   uint8_t       **contents,
							   size_t        *length,
							   int      *error)
{
	assert(file != 0);

	if (file ==0)
	{
		return false;
	}
	int total_bytes = 0;
	int total_allocated = 0;
	char buf[1024] = "";
	char *read_str = 0;
	while (!feof(file))
	{
		int errno_1 = 0;
		int bytes_read = fread(buf, 1, sizeof(buf), file);
		errno_1 = errno;
		while ((total_bytes + bytes_read + 1) > total_allocated)
		{
			if (read_str)
			{
				total_allocated *= 2;
			}
			else
			{
				total_allocated = MIN(bytes_read + 1, (int)sizeof(buf));
			}
			char *tmp = realloc(read_str, total_allocated);
			if (tmp == 0)
			{
				*error = errno;
				free(read_str);
				fclose(file);
				return false;
			}
			read_str = tmp;
		}

		if (ferror(file))
		{
			*error = errno_1;
			free(read_str);
			fclose(file);
			return false;
		}
		memcpy(read_str + total_bytes, buf, bytes_read);
		//! 文件过大
		if (total_bytes + bytes_read < total_bytes)
		{
			free(read_str);
			fclose(file);
			return false;
		}

		total_bytes += bytes_read;
	}

	fclose(file);

	if (total_allocated == 0)
	{
		read_str = (char *)malloc(1);
		total_bytes = 0;
	}

	read_str[total_bytes] = '\0';

	if (length)
	{
		*length = total_bytes;
	}

	*contents = read_str;
	return true;
}

bool file_get_contents(const char  *filename, uint8_t  **contents, size_t  *length, int  *error)
{
	assert(filename);
	int fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
	{
		*error = errno;
		return false;
	}

	struct stat st;

	if (fstat(fd, &st) != 0)
	{
		*error = errno;
		close(fd);
		return false;
	}

	if (st.st_size > 0 && S_ISREG(st.st_mode))
	{
		return get_contents_regfile(filename, &st, fd, contents, length, error);
	}
	else
	{
		FILE *file = fdopen(fd, "r");
		if (fd == 0)
		{
			*error = errno;
			return false;
		}

		return get_contents_stdio(filename, file, contents, length, error);
	}
}

bool file_read(int fd, uint8_t *buf, size_t len, size_t *reads, int *error)
{
	if (0 == len)
	{
		if (reads)
		{
			*reads = 0;
		}
		return true;
	}
	int bytes_read_total = 0;

	while (bytes_read_total < (int)len)
	{
		int bytes_read = read(fd, buf + bytes_read_total, len - bytes_read_total);
		if (bytes_read < 0)
		{
			if (errno != EINTR && error != 0)
			{
				*error = errno;
				return false;
			}
		}
		else if (bytes_read == 0)
		{
			break;
		}
		else if (bytes_read > 0)
		{
			bytes_read_total += bytes_read;
		}
	}

	if (reads)
	{
		*reads = bytes_read_total;
	}

	return true;
}

bool	file_read_spec_bytes(int fd, uint8_t **buf, size_t to_read, size_t *reads, int *error)
{
	assert(buf != 0);
	if (buf ==0)
	{
		return false;
	}
	*buf = (uint8_t *)malloc(to_read); 
	if (0 == *buf)
	{
		if (error)
		{
			*error = errno;
		}
		return false;
	}
	if (!file_read(fd, *buf, to_read, reads, error))
	{
		return false;
	}

	if (to_read != *reads)
	{
		return false;
	}
	return true;
}

bool file_write_fd(int fd, uint8_t *buf, size_t len, int *error)
{
	int bytes_write_total = 0;

	while (bytes_write_total < (int)len)
	{
		int bytes_write = write(fd, buf + bytes_write_total, len - bytes_write_total);
		if (bytes_write < 0)
		{
			d_printf_errno();
			if (errno != EINTR)
			{
				*error = errno;
				return false;
			}
		}
		else if (bytes_write == 0)
		{
			break;
		}
		else if (bytes_write > 0)
		{
			bytes_write_total += bytes_write;
		}
	}

	return true;
}

bool file_write_std(FILE *file, uint8_t *buf, size_t len, int *error)
{
	int bytes_write = fwrite(buf , 1, len, file);
	if (bytes_write != len)
	{
		d_printf_errno();
		if (/*ferror(file) &&*/errno != EINTR)
		{
			*error = errno;
			return false;
		}
	}


	return true;
}

#ifndef __file_util_self_test__

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: xxx [filepath]\n");

		return -1;
	}

	uint8_t *contents = 0;
	int length = 0;
	int error = 0;

	if (file_get_contents(argv[1], &contents, &length, &error))
	{
		printf("%s@\n%s\n", argv[1], contents);
		free(contents);
	}
	else
	{
		printf("%s@%s\n", argv[1], strerror(errno));
	}

	return 0;
}

bool	file_touch(const char* file, int *error)
{
	mode_t mode = S_IRGRP|S_IWGRP|S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH;
	int fd = -1;
	if ((fd = creat(file,mode)) == -1)
	{
		if (error)
		{
			*error = errno;
		}
		return false;
	}


	while(close(fd) !=0)
	{
		if (errno == EINTR)
		{
			continue;
		}
		break;
	}

	return true;
}








#endif
