#include "shell.h"
#include "device.h"
#include "menu.h"
#include "modem_test.h"
#include "test.h"
#include "uart.h"

#include "app.h"
#include "common.h"
#include "flash.h"
#include "misc.h"
#include "sunrise.h"

#define PREV_KEY 0x5B /* "[" key */
#define NEXT_KEY 0x5D /* "]" key */

#define CLEAR_LINE() printf("\33[2K\r");
#define PROMPT() printf("\r[Hitec]%% ");

static ShellCmdBuffer_t *ShellCmdBuf = NULL;

void SHELL_init()
{
	ShellCmdBuf = (ShellCmdBuffer_t *)malloc(sizeof(ShellCmdBuffer_t));
	if (ShellCmdBuf == NULL) {
		printf("malloc(ShellCmdBuf) failed\n");
		exit(1);
	}
	memset(ShellCmdBuf, 0, sizeof(ShellCmdBuffer_t));
}

char SHELL_getChar()
{
	if (TEST_checkTestMode() == FALSE) {
		REBOOT_SYSTEM();
	}

	TEST_runTasks();

	char c;
	if (!UART_getChar(UART_A1, &c)) {
		return 0;
	}

	static int escMode = 0;

	switch (escMode) {
	case 0:
		if (c == 0x1B) {
			escMode++;
			c = 0;
		}
		break;

	case 1:
		if (c == 0x5B) {
			escMode++;
		} else {
			escMode = 0;
		}
		c = 0;
		break;

	default:
		switch (c) {
		case 'A':
			c = CHAR_UP_ARROW;
			break;
		case 'B':
			c = CHAR_DOWN_ARROW;
			break;
		case 'C':
			c = CHAR_RIGHT_ARROW;
			break;
		case 'D':
			c = CHAR_LEFT_ARROW;
			break;
		default:
			c = 0;
		}
		escMode = 0;
	}
	return c;
}

int SHELL_gets(char *buf)
{
	char c;
	char *s = buf;
	int hptr = (ShellCmdBuf->wptr > 0) ? ((ShellCmdBuf->wptr - 1) % MAX_HISTORY) : 0;
	ShellCmdBuffer_t *h = ShellCmdBuf;

	while (1) {
		c = SHELL_getChar();
		if (c == 0) {
			continue;
		}

		if (c == '\n' || c == '\r') {
			printf("\n");
			break;
		}

		switch (c) {
		case CHAR_UP_ARROW:
		case CHAR_LEFT_ARROW:
			CLEAR_LINE();
			PROMPT();
			snprintf(buf, MAX_COLUMN, "%s", h->cmd[hptr]);
			if (--hptr < 0) {
				hptr = MAX_HISTORY - 1;
			}
			printf("%s", buf);
			s = buf + strlen(buf);
			break;

		case CHAR_DOWN_ARROW:
		case CHAR_RIGHT_ARROW:
			CLEAR_LINE();
			PROMPT();
			snprintf(buf, MAX_COLUMN, "%s", h->cmd[hptr]);
			if (++hptr >= MAX_HISTORY) {
				hptr = 0;
			}
			printf("%s", buf);
			s = buf + strlen(buf);
			break;

		case PREV_KEY:
			CLEAR_LINE();
			PROMPT();
			snprintf(buf, MAX_COLUMN, "%s", h->cmd[hptr]);
			if (--hptr < 0) {
				hptr = MAX_HISTORY - 1;
			}
			printf("%s", buf);
			s = buf + strlen(buf);
			break;

		case NEXT_KEY:
			CLEAR_LINE();
			PROMPT();
			snprintf(buf, MAX_COLUMN, "%s", h->cmd[hptr]);
			if (++hptr >= MAX_HISTORY) {
				hptr = 0;
			}
			printf("%s", buf);
			s = buf + strlen(buf);
			break;

		case '\r':
			printf("%c", c);
			break;

		case '\b':
			if (s > buf) {
				printf("%c", c);
				--s;
				printf(" \b");
			}
			break;

		case 0x7f:
			while (s > buf) {
				--s;
				printf("\b \b");
			}
			break;

		default:
			if (c >= ' ' && c <= '~')
				*s = c;
			if (s < buf + MAX_COLUMN - 1)
				s++;
			else {
				printf("\b");
			}
			printf("%c", c);
		}
	}

	h->hptr = h->wptr;
	*s = '\0';
	return ((int)(s - buf));
}

uint32 SHELL_atoi(char *p)
{
	uint32 ch = 0, num = 0, base = 16;
	switch (*p) {
	case '&':
		p += 1;
		base = 10;
		break;
	case '$':
		p += 1;
		base = 16;
		break;
	}

	while (TRUE) {
		ch = *p++;
		if (ch >= '0' && ch <= '9')
			ch -= '0';
		else if (ch >= 'A' && ch <= 'F')
			ch -= ('A' - 10);
		else if (ch >= 'a' && ch <= 'f')
			ch -= ('a' - 10);
		else
			break;
		if (ch >= base)
			break;
		num = num * base + ch;
	}

	return (num);
}

void SHELL_string(char *p, char *strbuf)
{
	*strbuf = '\0';

	while (*p && *p != 0x20) {
		*strbuf++ = *p++;
	}
	*strbuf = '\0';
}

uint32 SHELL_decimal(char *p)
{
	uint32 ch = 0, num = 0;
	while (TRUE) {
		ch = *p++;
		if (ch >= '0' && ch <= '9')
			ch -= '0';
		else if (ch == ' ')
			break;
		else if (ch == '.')
			continue;
		else
			break;

		num = num * 10 + ch;
	}

	return (num);
}

static int getArguments(char *p, char **cmdArgs, int maxArgs)
{
	int numOfArguments = 0;
	while (1) {
		// search space
		while (*p != 0x20) {
			if (*p == '\0' || *p == '\r' || *p == '\n')
				return numOfArguments;
			p++;
		}

		while (1) {
			if (*p == '\0' || *p == '\r' || *p == '\n')
				return numOfArguments;
			if (*p != 0x20) {
				cmdArgs[numOfArguments++] = p;
				break;
			}
			p++;
		}

		if (numOfArguments >= maxArgs)
			return numOfArguments;
	}
}

static void help()
{
	printf("------------------------------------------------------------------------"
	       "------------------\n");
	printf("                                       Command List                     "
	       "                  \n");
	printf("------------------------------------------------------------------------"
	       "------------------\n");

	int nCmd = sizeof(CmdSet) / sizeof(ShellCommandSet_t);
	for (int i = 0; i < nCmd; i++) {
		int len = strlen(CmdSet[i].usage);
		if (len > 0) {
			printf(" %s", CmdSet[i].usage);
			for (int j = MAX_CMD_USAGE_LEN - len; j > 0; j--) {
				printf(" ");
			}
			printf("* %s\n", CmdSet[i].desc);
		}
	}
	printf("------------------------------------------------------------------------"
	       "------------------\n");
}

static const ShellCommandSet_t *whatCommand(char *p)
{
	int i;
	char tmp[0x10];

	i = 0;
	while (*p != 0x20 && *p != '\0' && i < 0x10)
		tmp[i++] = *p++;
	tmp[i] = '\0';

	int nCmd = sizeof(CmdSet) / sizeof(ShellCommandSet_t);
	for (i = 0; i < nCmd; i++) {
		if (!strcmp(CmdSet[i].cmd, tmp)) {
			return &CmdSet[i];
		}
	}
	return NULL;
}

void SHELL_run()
{
#define LEN_ARG_STRING 0x30
#define MAX_NUM_OF_ARGS 10

	int cmdIdx;
	int nCmdArgs;
	char *cmdArgs[MAX_NUM_OF_ARGS];
	char cmdBuf[MAX_COLUMN];
	ShellCmdBuffer_t *h = ShellCmdBuf;

	while (1) {
		PROMPT();

		// Get shell string
		if (SHELL_gets(cmdBuf) == 0) {
			continue;
		}

		// Copy shell string into command buffer
		snprintf(h->cmd[h->wptr], MAX_COLUMN, "%s", cmdBuf);
		if (++h->wptr >= MAX_HISTORY) {
			h->wptr = 0;
		}

		// Get shell command from shell string
		const ShellCommandSet_t *shellCmd = whatCommand(cmdBuf);
		cmdIdx = (shellCmd != NULL) ? shellCmd->index : -1;

		// Get shell command arguments from shell string
		nCmdArgs = getArguments(cmdBuf, cmdArgs, MAX_NUM_OF_ARGS);
		switch (cmdIdx) {
		case CMDN_QMARK:
			help();
			break;

		case CMDN_PWRTC:
			if (nCmdArgs >= 6) {
				int year = SHELL_decimal(cmdArgs[0]); /* year */
				int month = SHELL_decimal(cmdArgs[1]); /* month */
				int day = SHELL_decimal(cmdArgs[2]); /* day */
				int hour = SHELL_decimal(cmdArgs[3]); /* hour */
				int min = SHELL_decimal(cmdArgs[4]); /* minute */
				int sec = SHELL_decimal(cmdArgs[5]); /* second */
				TEST_setRTC((cmdIdx == CMDN_PWRTC) ? 1 : 0, year, month, day, hour,
					    min, sec);
			} else {
				if (nCmdArgs == 0) {
					TEST_readRTC(0);
				} else {
					goto SHELL_CMD_ERROR;
				}
			}
			break;

		case CMDN_RFLASH:
			if (nCmdArgs >= 1) {
				int sector = SHELL_decimal(cmdArgs[0]); /* sector */
				TEST_readFlash(sector);
			} else {
				TEST_readFlash(0xFF);
			}
			break;

		case CMDN_EFLASH:
			if (nCmdArgs >= 1) {
				int sector = SHELL_decimal(cmdArgs[0]); /* sector */
				TEST_eraseFlash(sector);
			} else {
				TEST_eraseFlash(0xFF);
			}
			break;

		case CMDN_PBATT:
			TEST_readBattery();
			break;

		case CMDN_RSN:
			TEST_readSerialNum();
			break;

		case CMDN_WSN:
			if (nCmdArgs >= 1) {
				char sn[LEN_ARG_STRING];
				SHELL_string(cmdArgs[0], sn); /* serial id */
				TEST_writeSerialNum(sn);
			} else {
				goto SHELL_CMD_ERROR;
			}
			break;

		case CMDN_PRESET:
			REBOOT_SYSTEM();
			break;

		case CMDN_CONFIG:
			TEST_config();
			break;

		case CMDN_MP:
			if (nCmdArgs >= 1) {
				int ri = SHELL_decimal(cmdArgs[0]); /* ri */
				TEST_meteringParamer(ri);
			} else {
				goto SHELL_CMD_ERROR;
			}
			break;

		case CMDN_MT:
			if (nCmdArgs >= 1) {
				int type = SHELL_decimal(cmdArgs[0]); /* meter type */
				TEST_setMeterType(type);
			} else {
				TEST_setMeterType(0xFF);
			}
			break;

		case CMDN_SI:
			if (nCmdArgs >= 1) {
				int flag = SHELL_decimal(cmdArgs[0]);
				TEST_shortInterval(flag);
			} else {
				TEST_shortInterval(0xFF);
			}
			break;

		case CMDN_DEBUG:
			if (nCmdArgs >= 1) {
				int flag = SHELL_decimal(cmdArgs[0]);
				TEST_debugPrint(flag);
			} else {
				TEST_debugPrint(0xFF);
			}
			break;

		case CMDN_RMETER:
			if (nCmdArgs >= 2) {
				int type = SHELL_decimal(cmdArgs[0]); /* meter type */
				int sec = SHELL_decimal(cmdArgs[1]); /* Time(sec) */
				TEST_readMeter(type, sec);
			} else {
				TEST_readMeter(0xFF, 0xFF);
			}
			break;

		case CMDN_MODEM:
			TEST_modem();
			break;

		case CMDN_PVER:
			TEST_readFwVersion();
			break;

		case CMDN_PMODEM:
			TEST_pmodem();
			break;

		case CMDN_PNFC:
			TEST_checkNFC();
			break;

		case CMDN_RNFC:
			TEST_resetNFC();
			break;

		case CMDN_PWSN:
			if (nCmdArgs >= 2) {
				char sn[LEN_ARG_STRING];
				SHELL_string(cmdArgs[0], sn); /* serial num */
				int ri = SHELL_decimal(cmdArgs[1]); /* ri */
				TEST_pwsn(sn, ri);
			} else {
				goto SHELL_CMD_ERROR;
			}
			break;

		case CMDN_PRSN:
			TEST_prsn();
			break;

		case CMDN_PRNBID:
			TEST_prnbid();
			break;

		case CMDN_PWSERVER:
			if (nCmdArgs >= 3) {
				char ip[LEN_ARG_STRING];
				char sc[LEN_ARG_STRING];
				SHELL_string(cmdArgs[0], ip); /* server ip */
				int port = SHELL_decimal(cmdArgs[1]); /* server port */
				SHELL_string(cmdArgs[2], sc); /* service code */
				TEST_pwserver(ip, port, sc);
			} else {
				goto SHELL_CMD_ERROR;
			}
			break;

		case CMDN_PRSERVER:
			TEST_prserver();
			break;

		case CMDN_PWFOTA:
			if (nCmdArgs >= 3) {
				char ip[LEN_ARG_STRING];
				SHELL_string(cmdArgs[0], ip); /* fota server ip */
				int port = SHELL_decimal(cmdArgs[1]); /* fota port */
				int interval = SHELL_decimal(cmdArgs[2]); /* fota interval */
				TEST_pwfota(ip, port, interval);
			} else {
				goto SHELL_CMD_ERROR;
			}
			break;

		case CMDN_PRFOTA:
			TEST_prfota();
			break;

		case CMDN_PNBCTX:
			TEST_pnbctx();
			break;

		case CMDN_RELAY:
			if (nCmdArgs >= 1) {
				int flag = SHELL_decimal(cmdArgs[0]);
				TEST_relayMode(flag);
			} else {
				printf("command usage: %s\n", shellCmd->usage);
			}
			break;

		case CMDN_GPIO_WRITE:
			if (nCmdArgs >= 3) {
				int port = SHELL_decimal(cmdArgs[0]);
				int pin = SHELL_decimal(cmdArgs[1]);
				int status = SHELL_decimal(cmdArgs[2]);

				TEST_gpioWrite(port, pin, status);
			} else {
				printf("command usage: %s\n", shellCmd->usage);
			}
			break;

		case CMDN_GPIO_READ:
			if (nCmdArgs >= 1) {
				int port = SHELL_decimal(cmdArgs[0]);

				TEST_gpioRead(port);
			} else {
				printf("command usage: %s\n", shellCmd->usage);
			}
			break;

		case CMDN_SUNRISE:
			calcSunrise();
			break;

		default:
			printf("unknown command\n");
			break;
		SHELL_CMD_ERROR:
			if (cmdIdx > 0) {
				printf("command usage: %s\n", shellCmd->usage);
			}
		}
	}
}
