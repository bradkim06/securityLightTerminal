#ifndef __AT_COMMAND_H__
#define __AT_COMMAND_H__

#include "modem.h"
#include "var_type.h"

#define ATCMD_RESP_DEFAULT "OK"
#define ATCMD_DEFAULT_NEXT_INTERVAL 100

//=========================================================
// NFC ISR macro interface for external access
//=========================================================
#define ATCMD_WRITER(x) MODEM_write(x)
#define ATCMD_READER(x) MODEM_read(x)

//=========================================================
// TYPES
//=========================================================
typedef void (*ATCMD_NOTIFY_PARSER)(const char *, const int *);
typedef BOOL (*ATCMD_PARSER)(const char *, const int *, uint32 *);
typedef struct {
	uint32 timeout;
	int retry;
	char *cmd;
	ATCMD_PARSER parser;
} AtCmd_t;

void ATCMD_init(ATCMD_NOTIFY_PARSER parser);
void ATCMD_release();
BOOL ATCMD_write(uint32 *timeout);
BOOL ATCMD_retry(uint32 *timeout);
BOOL ATCMD_read(uint32 *nextDelay);
BOOL ATCMD_push(uint32 timeout, int retry, ATCMD_PARSER parser, const char *cmd,
		...);
AtCmd_t *ATCMD_pull();

#endif // __AT_COMMAND_H__
