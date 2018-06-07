#ifndef __file_utils_h__
#define __file_utils_h__

#include <stdbool.h>
#include <stdint.h>

/**
 * 读取各种类型的文件内容 
 *  
 * @author zf (12/19/2013)
 * 
 * @param filename 文件路径+名称;如果传入路径而不是文件，将会产生错误。
 * @param contents 读取的内容，使用后必须释放
 * @param length   读取内容的字节总数
 * @param error    系统的errno
 * 
 * @return bool    成功与否
 */
bool 	file_get_contents (const char  *filename,uint8_t  **contents,size_t  *length,int  *error);

bool 	file_read(int fd, uint8_t *buf, size_t len, size_t *reads, int *error);

bool	file_read_spec_bytes(int fd, uint8_t **buf, size_t to_read, size_t *reads, int *error);

bool 	file_write_fd(int fd, uint8_t *buf, size_t len, int *error);

bool 	file_write_std(FILE *, uint8_t *buf, size_t len, int *error);

bool	file_touch(const char* file,  int *error);

#endif
