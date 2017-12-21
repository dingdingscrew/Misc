#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "plat_types.h"
#include "plat_basic_api.h"
#include "pmic_api.h"

#define conv_dec(data)	strtol(data,NULL,10)
extern unsigned long Image$$TESTCALLS$$Base;
#define test_calls_base &Image$$TESTCALLS$$Base

extern unsigned long Image$$TESTCALLS$$Limit;
#define test_calls_end &Image$$TESTCALLS$$Limit


cmd_tb_t *find_cmd(const char *cmd)
{
	cmd_tb_t *cmdtp;
	cmd_tb_t *cmdtp_temp = (cmd_tb_t *)test_calls_base;
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = (cmd_tb_t *)test_calls_base;
	     cmdtp < (cmd_tb_t *)test_calls_end;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

static int help(struct cmd_tb_s *m, int argc, char *argv[])
{
    cmd_tb_t *entry, *p = (cmd_tb_t *)test_calls_base;
    for (; p < (cmd_tb_t *)test_calls_end;) {
        trace_frame_without_prefix("%s:  %s\n", p->name, p->usage);
        p++;
    }

	return 0;
}

TEST_CASE_TABLE(help, 0, 0, help, "All command is below:\r\n");
TEST_CASE_TABLE(ls, 0, 0, help, "List All command is below:\r\n");


static int test_all(struct cmd_tb_s *m, int argc, char *argv[])
{
    cmd_tb_t *entry, *p = (cmd_tb_t *)test_calls_base;
    for (; p < (cmd_tb_t *)test_calls_end;) {
        entry = p;
        int (*cmd)(cmd_tb_t*, int argc, char *argv[]) = entry->cmd;
	    p++;
        if (cmd && (cmd != test_all))
            cmd(entry, argc, argv);
    }
    return 0;
}

TEST_CASE_TABLE(test_all, 0, 0, test_all, "Begin to test all cases.\r\n");

static int registers_cmd(struct cmd_tb_s *m, int argc, char *argv[])
{
    if ((argc == 3) && argv[1] && argv[2]) {
        if (strncmp (argv[1], "r32", 3) == 0) {
            unsigned int a = strtol(argv[2],NULL,16);
            unsigned int v = REG32(a);
            trace_frame_without_prefix("0x%x:0x%x\r\n",a,v);
        } else if (strncmp (argv[1], "r64", 3) == 0) {
            unsigned long long a = strtol(argv[2],NULL,16);
            unsigned long long v = REG64(a);
            trace_frame_without_prefix("0x%x:0x%x\r\n",a,v);
        } else if (strncmp (argv[1], "r16", 3) == 0) {
            unsigned int a = strtol(argv[2],NULL,16);
            unsigned short v = REG16(a);
            trace_frame_without_prefix("0x%x:0x%x\r\n",a,v);
        }
    } else if ((argc == 4) && argv[1] && argv[2] && argv[3]) {
        if (strncmp (argv[1], "w32", 3) == 0) {
            unsigned int a = strtol(argv[2],NULL,16);
            unsigned int v = strtol(argv[3],NULL,16);
            write32(a,v);
        } else if (strncmp (argv[1], "w64", 3) == 0) {
            unsigned long long a = strtol(argv[2],NULL,16);
            unsigned long long v = strtol(argv[3],NULL,16);
            write64(a,v);
        } else if (strncmp (argv[1], "w16", 3) == 0) {
            unsigned int a = strtol(argv[2],NULL,16);
            uint16 v = strtol(argv[3],NULL,16);
            write16(a,v);
        }
    }
    return 0;
}

TEST_CASE_TABLE(registers_cmd, 3, 0, registers_cmd, "\r\n\
	Read:registers_cmd r16/r32/r64 0Xaddress\r\n, write:registers_cmd w16/w32/w64 address 0X value\r\n");

static int pmic_reboot_cmd(struct cmd_tb_s *m, int argc, char *argv[])
{
#if defined(HARDWARE_EVB)
	if (argc < 2) {
		trace("Warning cmd error.pls use:reboot X(only support 0,1,2,4,8,16,128,512)\n");
		return 0;
	}
	if(!conv_dec(argv[1])) {
		return reboot_system();
	}
	return pmic_set_wd_timer_int(conv_dec(argv[1]));
#endif
}
TEST_CASE_TABLE(reboot, 1, 0, pmic_reboot_cmd, "reboot: reboot 0, Wd timer set:reboot X(only support 1,2,4,8,16,128,512)\r\n");
