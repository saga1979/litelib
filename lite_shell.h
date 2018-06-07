#ifndef __shell_utils_h__
#define __shell_utils_h__


#include <stdbool.h>
#include "lite_list.h"

typedef enum _ShellCmdRet
{
	SHELL_CMD_RET_SUCCESS = 0,
	SHELL_CMD_RET_TIMEOUT,
}ShellCmdRet;


/**
 * 执行某个命令
 * 
 * @author zf (4/23/2014)
 * 
 * @param cmd  需要执行的命令
 * @param sync 是否阻塞执行
 * 
 * @return int  0表示成功，其余表示错误码
 */
int	sh_excute_cmd(const char *cmd, bool sync);

/**
 * 执行命令，并取回命令输出(有最大值限制,可根据具体应用调整代码)
 * 
 * @author zf (4/23/2014)
 * 
 * @param cmd 需要执行的命令
 * @param result 命令输出
 * @param wait_seconds 等待多长时间
 * 
 * @return int 0,成功
 */
int sh_excute_and_fetch_result(const char *cmd, char **result, int wait_seconds);


/**
 * 将命令执行后的输出以“行”的方式取回
 * 
 * @author zf (4/23/2014)
 * 
 * @param list 命令输出行
 * @param cmd 执行命令
 * 
 * @return int 
 */
int  	sh_get_ret_in_lines(lite_list **list, const char *cmd);

#endif
