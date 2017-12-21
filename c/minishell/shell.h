#ifndef __SHELL_H__
#define __SHELL_H__

#include "plat_types.h"

/* Default to a width of 8 characters for help message command width */
#ifndef CONFIG_SYS_HELP_CMD_WIDTH
#define CONFIG_SYS_HELP_CMD_WIDTH	8
#endif
/* Miscellaneous configurable options */
#define CONFIG_SYS_PROMPT			"ctest# "
/* Console I/O Buffer Size */
#define CONFIG_SYS_CBSIZE			128
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
/* max number of command args */
#define CONFIG_SYS_MAXARGS			16
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE


/*
 * Command Flags:
 */

#define CMD_FLAG_REPEAT		0x0001	/* repeat last command		*/
#define CMD_FLAG_BOOTD		0x0002	/* command is from bootd	*/

#include <stdarg.h>

void	shell_loop	(void);
int	run_command	(const char *cmd, int flag);
int	readline	(const char *const prompt);
int	readline_into_buffer	(const char *const prompt, char * buffer);
int	parse_line (char *, char *[]);
cmd_tb_t *find_cmd(const char *cmd);
cmd_tb_t *find_cmd_tb(const char *cmd, cmd_tb_t *table, int table_len);
int cmd_usage(cmd_tb_t *cmdtp,char *s);

#define ROUND(a,b)		(((a) + (b)) & ~((b) - 1))
#define DIV_ROUND(n,d)		(((n) + ((d)/2)) / (d))
#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#endif

