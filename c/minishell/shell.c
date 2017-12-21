#if !(defined __CMAKE_SYSTEM__)
#include <string.h>
#endif

#include "plat_basic_api.h"

#include "shell.h"

#undef DEBUG_MODULE

#define getchar() ReadChar()
static char  console_buffer[CONFIG_SYS_CBSIZE + 1];	/* console I/O buffer	*/
static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen);
static char erase_seq[] = "\b \b";		/* erase sequence	*/
static char   tab_seq[] = "        ";		/* used to expand TABs	*/

#if 1
#define dbg(...) do {} while(0)
#else
#define dbg	trace
#endif
#define puts trace_frame_without_prefix
#define putchar WriteChar

void shell_loop (void)
{
	static char lastcommand[CONFIG_SYS_CBSIZE] = { 0, };
	int len;
	int rc = 1;
	int flag;

	for (;;) {
		len = readline (CONFIG_SYS_PROMPT);

		flag = 0;
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;

		if (len == -1)
			puts ("<INTERRUPT>\r\n");
		else{
			dbg("Input command:%s,Flag:%d !\n",lastcommand,flag);
			rc = run_command (lastcommand, flag);
		}
		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
}

int readline (const char *const prompt)
{
	/*
	 * If console_buffer isn't 0-length the user will be prompted to modify
	 * it instead of entering it from scratch as desired.
	 */
	console_buffer[0] = '\0';

	return readline_into_buffer(prompt, console_buffer);
}

int readline_into_buffer (const char *const prompt, char * buffer)
{
	char *p = buffer;

	char * p_buf = p;
	int	n = 0;				/* buffer index		*/
	int	plen = 0;			/* prompt length	*/
	int	col;				/* output column cnt	*/
	char c;

	if (prompt) {
		plen = strlen (prompt);
		puts("%s", prompt);
	}
	col = plen;

	for (;;) {
		c = getchar();

		switch (c) {
		case '\r':				/* Enter		*/
		case '\n':
			*p = '\0';
			puts ("\r\n");
			return (p - p_buf);

		case '\0':				/* nul			*/
			continue;

		case 0x03:				/* ^C - break		*/
			p_buf[0] = '\0';	/* discard input */
			return (-1);

		case 0x15:				/* ^U - erase line	*/
			while (col > plen) {
				puts (erase_seq);
				--col;
			}
			p = p_buf;
			n = 0;
			continue;

		case 0x17:				/* ^W - erase word	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			while ((n > 0) && (*p != ' ')) {
				p=delete_char(p_buf, p, &col, &n, plen);
			}
			continue;

		case 0x08:				/* ^H  - backspace	*/
		case 0x7F:				/* DEL - backspace	*/
			p=delete_char(p_buf, p, &col, &n, plen);
			continue;

		default:
			/*
			 * Must be a normal character then
			 */
			if (n < CONFIG_SYS_CBSIZE-2) {
				if (c == '\t') {	/* expand TABs		*/
					puts (tab_seq+(col&07));
					col += 8 - (col&07);
				} else {
					++col;		/* echo input		*/
					putchar (c);
				}
				*p++ = c;
				++n;
			} else {			/* Buffer full		*/
				putchar ('\a');
			}
		}
	}

	return 0 ;
}

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putchar (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}

int parse_line (char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < CONFIG_SYS_MAXARGS) {
		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	puts ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);
	return (nargs);
}

static void process_macros (const char *input, char *output)
{
	char c, prev;
	const char *varname_start = NULL;
	int inputcnt = strlen (input);
	int outputcnt = CONFIG_SYS_CBSIZE;
	int state = 0;		/* 0 = waiting for '$'  */

	/* 1 = waiting for '(' or '{' */
	/* 2 = waiting for ')' or '}' */
	/* 3 = waiting for '''  */
#ifdef DEBUG_MODULE
	char *output_start = output;

	dbg ("[PROCESS_MACROS] INPUT len %d: \"%s\"\n", strlen (input),
		input);
#endif

	prev = '\0';		/* previous character   */

	while (inputcnt && outputcnt) {
		c = *input++;
		inputcnt--;

		if (state != 3) {
			/* remove one level of escape characters */
			if ((c == '\\') && (prev != '\\')) {
				if (inputcnt-- == 0)
					break;
				prev = c;
				c = *input++;
			}
		}

		switch (state) {
		case 0:	/* Waiting for (unescaped) $    */
			if ((c == '\'') && (prev != '\\')) {
				state = 3;
				break;
			}
			if ((c == '$') && (prev != '\\')) {
				state++;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		case 1:	/* Waiting for (        */
			if (c == '(' || c == '{') {
				state++;
				varname_start = input;
			} else {
				state = 0;
				*(output++) = '$';
				outputcnt--;

				if (outputcnt) {
					*(output++) = c;
					outputcnt--;
				}
			}
			break;
		case 2:	/* Waiting for )        */
			if (c == ')' || c == '}') {
				int i;
				char envname[CONFIG_SYS_CBSIZE], *envval;
				int envcnt = input - varname_start - 1;	/* Varname # of chars */

				/* Get the varname */
				for (i = 0; i < envcnt; i++) {
					envname[i] = varname_start[i];
				}
				envname[i] = 0;

				/* Get its value */
				envval = NULL;//getenv (envname);

				/* Copy into the line if it exists */
				if (envval != NULL)
					while ((*envval) && outputcnt) {
						*(output++) = *(envval++);
						outputcnt--;
					}
				/* Look for another '$' */
				state = 0;
			}
			break;
		case 3:	/* Waiting for '        */
			if ((c == '\'') && (prev != '\\')) {
				state = 0;
			} else {
				*(output++) = c;
				outputcnt--;
			}
			break;
		}
		prev = c;
	}

	if (outputcnt)
		*output = 0;
	else
		*(output - 1) = 0;

#ifdef DEBUG_MODULE
	dbg ("[PROCESS_MACROS] OUTPUT len %d: \"%s\"\n",
		strlen (output_start), output_start);
#endif
}

int run_command (const char *cmd, int flag)
{
	cmd_tb_t *cmdtp;
	char cmdbuf[CONFIG_SYS_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CONFIG_SYS_CBSIZE];
	char *str = cmdbuf;
	char *argv[CONFIG_SYS_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;

#ifdef DEBUG_MODULE
	dbg ("[RUN_COMMAND] cmd[%p]=\"", cmd);
	puts (cmd ? cmd : "NULL");	/* use puts - string may be loooong */
	puts ("\"\n");
#endif

	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CONFIG_SYS_CBSIZE) {
		puts ("## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */

#ifdef DEBUG_MODULE
	dbg ("[PROCESS_SEPARATORS] %s\n", cmd);
#endif
	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */
#ifdef DEBUG_MODULE
		dbg ("token: \"%s\"\n", token);
#endif

		/* find macros in this token and replace them */
		process_macros (token, finaltoken);

		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}

		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			puts ("Unknown command '%s' - try 'help'\r\n", argv[0]);
			rc = -1;	/* give up after bad command */
			continue;
		}

		/* found - check max args */
		if (argc > (cmdtp->maxargs + 1)) {
			puts("Too many args!! %s - %s\n\n", cmdtp->name, cmdtp->usage);
			rc = -1;
			continue;
		}


		/* OK - call function to do the command */
		if ((cmdtp->cmd) (cmdtp, argc, argv) != 0) {
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

	}

	return rc ? rc : repeatable;
}



