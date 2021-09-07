#include "queue.h"
#include "at_cmd.h"
#include "modem.h"
#include "misc.h"

#include "common.h"
//#include "assert.h"
#include "uart.h"
#include "test.h"

#if defined(DEBUG)
#define ATCMD_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts("[ATCMD] ");                                                     \
			printf(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0);
#else
#define ATCMD_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts(__VA_ARGS__);                                                    \
		}                                                                                  \
	} while (0);
#endif

static Queue_t *CommandQueue = NULL;

static ATCMD_NOTIFY_PARSER NOTIFY_PARSER = NULL;
static SERVER_NNMI_PARSER NNMI_PARSER = NULL;

static void atCmdFree(void *data)
{
	AtCmd_t *atCmd = (AtCmd_t *)data;
	if (atCmd != NULL) {
		if (atCmd->cmd != NULL) {
			free(atCmd->cmd);
		}
	}
}

void ATCMD_init(ATCMD_NOTIFY_PARSER notifyParser, SERVER_NNMI_PARSER nnmiParser)
{
	if (CommandQueue == NULL) {
		CommandQueue = QUEUE_malloc(atCmdFree);
	}
	NOTIFY_PARSER = notifyParser;
	NNMI_PARSER = nnmiParser;
}

void ATCMD_release()
{
	if (CommandQueue == NULL) {
		return;
	}

	do {
		if (QUEUE_isEmpty(CommandQueue)) {
			break;
		}
		QUEUE_pop(CommandQueue);
	} while (1);
}

BOOL ATCMD_write(uint32 *timeout)
{
	if (CommandQueue == NULL) {
		ATCMD_DBG("queue is not malloc in send\n");
		return FALSE;
	}

	if (QUEUE_isEmpty(CommandQueue)) {
		// ATCMD_DBG("queue is empty in send\n");
		return FALSE;
	}

	AtCmd_t *current = (AtCmd_t *)QUEUE_front(CommandQueue);
	if (current->parser == NULL || current->cmd == NULL) {
		ATCMD_DBG("write fail (not register parser or command)\n");
		return FALSE;
	} else {
		if (timeout != NULL) {
			*timeout = current->timeout;
		}
		ATCMD_WRITER(current->cmd);
	}

	return TRUE;
}

BOOL ATCMD_retry(uint32 *timeout)
{
	if (CommandQueue == NULL) {
		ATCMD_DBG("queue is not malloc in retry\n");
		return FALSE;
	}

	if (QUEUE_isEmpty(CommandQueue)) {
		ATCMD_DBG("queue is empty in retry\n");
		return FALSE;
	}

	AtCmd_t *current = (AtCmd_t *)QUEUE_front(CommandQueue);
	if (current->parser == NULL || current->cmd == NULL) {
		ATCMD_DBG("retry fail (not register parser or command)\n");
		return FALSE;
	}

	if (current->retry >= 1) {
		current->retry--;
		if (timeout != NULL) {
			*timeout = current->timeout;
		}

		if (current->cmd != NULL) {
			ATCMD_WRITER(current->cmd);
		}
		return TRUE;
	} else {
		ATCMD_release();
		ATCMD_DBG("retry fail (count over)\n");
		return FALSE;
	}
}

BOOL ATCMD_read(uint32 *nextDelay)
{
	char buf[LEN_MAX_MODEM_RX];
	int len = ATCMD_READER(buf);
	buf[len - 1] = '\0';

	if (NOTIFY_PARSER != NULL) {
		NOTIFY_PARSER(buf, &len);
	}

	if (CommandQueue == NULL) {
		ATCMD_DBG("queue is not malloc in parse\n");
		return FALSE;
	}

	if (QUEUE_isEmpty(CommandQueue)) {
		ATCMD_DBG("queue is empty in parse\n");
		NNMI_PARSER(buf, &len);
		return FALSE;
	}

	AtCmd_t *current = (AtCmd_t *)QUEUE_front(CommandQueue);

	if (current->parser != NULL) {
		if (current->parser(buf, &len, nextDelay)) {
			if (*nextDelay <= 0) {
				*nextDelay = ATCMD_DEFAULT_NEXT_INTERVAL;
			}
			QUEUE_pop(CommandQueue);
			return TRUE;
		}
	} else {
		ATCMD_DBG("parser is not registered\n");
	}

	return FALSE;
}

BOOL ATCMD_Instant(uint32 timeout, int retry, ATCMD_PARSER parser, const char *cmd, ...)
{
	char buffer[LEN_MAX_MODEM_TX];

	if (cmd != NULL) {
		va_list arg;
		va_start(arg, cmd);
		int cmdLen = vsnprintf(buffer, LEN_MAX_MODEM_TX, cmd, arg);
		va_end(arg);
	}

	ATCMD_WRITER(buffer);

	do {
		uint32 time = timeout;

		while (time) {
			MISC_delayMs(50);
			time -= 50;

			if (checkRxIdle()) {
				uint32 nextDelay = ATCMD_DEFAULT_NEXT_INTERVAL;
				char buf[LEN_MAX_MODEM_RX];
				memset(buf, 0, LEN_MAX_MODEM_RX);

				int len = ATCMD_READER(buf);
				buf[len - 1] = '\0';

				if (NOTIFY_PARSER != NULL) {
					NOTIFY_PARSER(buf, &len);
				}

				if (parser(buf, &len, &nextDelay)) {
					MISC_delayMs((int)nextDelay);
					retry = 0;
					break;
				}
			}
		}

		if (retry > 0) {
			ATCMD_WRITER(buffer);
		}
	} while (retry--);

	return TRUE;
}

BOOL ATCMD_push(uint32 timeout, int retry, ATCMD_PARSER parser, const char *cmd, ...)
{
	if (CommandQueue == NULL) {
		ATCMD_DBG("queue is not malloc in push\n");
		return FALSE;
	}

	AtCmd_t *atCmd = (AtCmd_t *)malloc(sizeof(AtCmd_t));
	if (atCmd == NULL) {
		ATCMD_DBG("fail to malloc AT command structure\n");
		return FALSE;
	}
	memset((void *)atCmd, 0, sizeof(AtCmd_t));
	atCmd->timeout = timeout;
	atCmd->retry = retry;
	atCmd->parser = parser;

	if (cmd != NULL) {
		va_list arg;
		va_start(arg, cmd);
		char buffer[LEN_MAX_MODEM_TX];
		int cmdLen = vsnprintf(buffer, LEN_MAX_MODEM_TX, cmd, arg);
		va_end(arg);

		atCmd->cmd = (char *)malloc(cmdLen + 1);
		if (atCmd->cmd == NULL) {
			ATCMD_DBG("fail to malloc AT command\n");
			return FALSE;
		}
		memset(atCmd->cmd, 0, cmdLen + 1);
		memcpy(atCmd->cmd, buffer, cmdLen);
	}

	if (atCmd->parser == NULL || atCmd->cmd == NULL) {
		ATCMD_DBG("register fail (parser or command empty)\n");
		return FALSE;
	}

	if (QUEUE_isFull(CommandQueue)) {
		ATCMD_DBG("fail register command (queue full)\n");
		return FALSE;
	}

	if (QUEUE_push(CommandQueue, (void *)atCmd)) {
		return TRUE;
	} else {
		ATCMD_DBG("fail register command (queue : %d)\n", CommandQueue->size);
		return FALSE;
	}
}

AtCmd_t *ATCMD_pull()
{
	if (CommandQueue == NULL) {
		ATCMD_DBG("queue is not malloc in pull\n");
		return FALSE;
	}

	if (QUEUE_isEmpty(CommandQueue)) {
		ATCMD_DBG("queue is empty in pull\n");
		return NULL;
	}

	return (AtCmd_t *)QUEUE_front(CommandQueue);
}
