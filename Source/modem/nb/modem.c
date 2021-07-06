#include "modem.h"
#include "at_cmd.h"
#include "message.h"
#include "nbiot.h"
#include "uuid.h"

#include "app.h"
#include "common.h"
#include "device.h"
#include "meter.h"
#include "misc.h"
#include "timer.h"
#include "uart.h"
// #include "assert.h"
#include "alarm.h"
#include "lcd.h"
#include "test.h"

#if defined(DEBUG)
#define MODEM_DBG(...)                                                         \
	do {                                                                   \
		if (TEST_isTestMode() == FALSE) {                              \
			printf_ts("[MODEM] ");                                 \
			printf(__VA_ARGS__);                                   \
		}                                                              \
	} while (0);
#else
#define MODEM_DBG(...)                                                         \
	do {                                                                   \
		if (TEST_isTestMode() == FALSE) {                              \
			printf_ts(__VA_ARGS__);                                \
		}                                                              \
	} while (0);
#endif

typedef struct {
	ModemProc_t currentProc;
	BOOL isStop;
	BOOL isPowerOn;
	uint8 taskId;
	uint8 retry;
} ModemContext_t;

static ModemContext_t modemCtx;

void MODEM_turnOn()
{
	if (!modemCtx.isPowerOn) {
		MODEM_DBG("power on\n");

#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_LDO)
		// LDO work turn on power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT |= BM(MODEM_VCC1_P2_PIN); // make high

		MISC_delayMs(100);

		// MOSFET work turn on power (P1.1)
		P1SEL &= ~BM(MODEM_VCC2_P1_PIN);
		P1DIR |= BM(MODEM_VCC2_P1_PIN);
		P1OUT &= ~BM(MODEM_VCC2_P1_PIN); // make low

		MISC_delayMs(10);
#elif (DEVICE_REVISION == DEV_REV_PWRCTRL_LDO_ONLY)
		// LDO work turn on power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT |= BM(MODEM_VCC1_P2_PIN); // make high

		MISC_delayMs(100);
#elif (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_ONLY)
		// turn on power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT &= ~BM(MODEM_VCC1_P2_PIN); // make low

		MISC_delayMs(10);
#endif
		modemCtx.isPowerOn = TRUE;
	}
}

void MODEM_turnOff()
{
	if (modemCtx.isPowerOn) {
		MODEM_DBG("power off\n");
		NBIOT_release();

#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_LDO)
		// MOSFET work off power (P1.1)
		P1SEL &= ~BM(MODEM_VCC2_P1_PIN);
		P1DIR |= BM(MODEM_VCC2_P1_PIN);
		P1OUT |= BM(MODEM_VCC2_P1_PIN); // make high

		MISC_delayMs(10);

		// LDO work turn off power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT &= ~BM(MODEM_VCC1_P2_PIN); // make low

		MISC_delayMs(100);
#elif (DEVICE_REVISION == DEV_REV_PWRCTRL_LDO_ONLY)
		// LDO work turn off power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT &= ~BM(MODEM_VCC1_P2_PIN); // make low

		MISC_delayMs(100);
#elif (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_ONLY)
		// turn off power (P2.0)
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT |= BM(MODEM_VCC1_P2_PIN); // make high

		MISC_delayMs(10);
#endif

		modemCtx.isPowerOn = FALSE;
	}
}

void MODEM_sleep()
{
#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_LDO)
	if (!modemCtx.isPowerOn) {
		MODEM_DBG("sleep (not power on)\n");
	} else {
		MODEM_DBG("sleep\n");

		// LDO work
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT &= ~BM(MODEM_VCC1_P2_PIN); // make low

		MISC_delayMs(100);
	}
#endif
}

void MODEM_wakeup()
{
#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_LDO)
	if (!modemCtx.isPowerOn) {
		MODEM_DBG("wake-up (not power on)\n");
	} else {
		MODEM_DBG("wake-up\n");

		// LDO work
		P2SEL &= ~BM(MODEM_VCC1_P2_PIN);
		P2DIR |= BM(MODEM_VCC1_P2_PIN);
		P2OUT |= BM(MODEM_VCC1_P2_PIN); // make high

		MISC_delayMs(100);
	}
#endif
}

void MODEM_init(uint8 taskId)
{
	memset(&modemCtx, 0, sizeof(ModemContext_t));
	modemCtx.taskId = taskId;
	NBIOT_init();
}

void MODEM_enable()
{
	UartInitParam_t param = { .isTxEnable = TRUE,
				  .isRxEnable = TRUE,
				  .baudrate = UART_BAUDRATE_9600,
				  .parity = UART_PARITY_NONE,
				  .stopbit = UART_STOPBIT_ONE,
				  .charLen = UART_8BITS_CHAR };

	UART_open(UART_A2, &(param));
}

void MODEM_disable()
{
	/* if (nbCtx.status.psmOn) { */
	/* 	UART_close(UART_A2, GPIO_INPUT); */
	/* } else { */
	/* 	UART_close(UART_A2, GPIO_LOW); */
	/* } */
}

void MODEM_write(const char *pCmd)
{
	if (TEST_isTestMode() == FALSE) {
		printf(TP_ANSI_FG_BLUE);
		// AT CMD가 512 byte에 근접할 경우 buffer overflow의 위험이 있어 timestamp
		// 출력을 분리
		printf_ts("");
		printf("%s\n", pCmd);
		printf(TP_ANSI_RESET);
	}

	int len = strlen(pCmd);
	UART_send(UART_A2, (void *)pCmd, len, TRUE);
}

int MODEM_read(char *buf)
{
	int len = UART_receive(UART_A2, buf, LEN_MAX_MODEM_RX);

	if (TEST_isTestMode() == FALSE && len > 0) {
		if (len > LEN_MAX_MODEM_RX) {
			len = LEN_MAX_MODEM_RX;
		}
		// Input end char because of expected data is string
		buf[len - 1] = '\0';

		printf(TP_ANSI_FG_RED);
		PRINT_string(buf, len);
		printf(TP_ANSI_RESET);
	}

	return len;
}

void MODEM_run(ModemProc_t proc, BOOL isLcdOn)
{
	// 무선통신 간의 접속간격이 필요한 경우도 있으므로 getState()를 사용.
	if (MODEM_getState() != DEVICE_STATUS_IDLE) {
		MODEM_DBG("Modem is running, step (%s)\n",
			  NBIOT_STEP_STR(nbCtx.nextStep));
		return;
	}
	modemCtx.isStop = FALSE;
	modemCtx.currentProc = proc;

	if (proc == MODEM_PROC_PERIODIC_REPORT) {
		// Process 시작 전 error code 초기화
		nbCtx.error = NBIOT_ERROR_NONE;
		// Process 시작 전 step 초기화
		nbCtx.currentStep = NBIOT_STEP_ATTACH_NW;
		nbCtx.nextStep = NBIOT_STEP_ATTACH_NW;
		// Process 시작 전 status flag 초기화
		nbCtx.status.error = 0;
		nbCtx.status.setCfg = 0;
		nbCtx.status.waitDL = 0;
		ATCMD_release();
	} else {
		NBIOT_open();
		MODEM_enable();
		// if (nbCtx.status.psmOn) {
		// 	MODEM_wakeup();
		// } else {
		MODEM_turnOn();
		// }
	}

	OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_RUN_PROC);
}

void MODEM_stop(BOOL isVccOff, BOOL isLcdOn)
{
	/* modemCtx.isStop = TRUE; */
	/* modemCtx.retry = 0; */

	/* NBIOT_close(); */
	/* if (nbCtx.currentStep != NBIOT_STEP_FINISH || isVccOff) { */
	/* 	// 모뎀상태와 관계없이 강제 Off */
	/* 	MODEM_turnOff(); */
	/* } else { */
	/* 	if (nbCtx.status.psmOn) { */
	/* 		MODEM_sleep(); */
	/* 	} else { */
	/* 		MODEM_turnOff(); */
	/* 	} */
	/* } */
	/* // 모뎀 off 여부에 따라 동작이 다르므로 위치 주의 */
	/* MODEM_disable(); */

	/* // MODEM disable 및 power off(sleep) 후 event 취소. */
	/* OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_SEND_ATCMD); */
	/* OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_RECV_ATCMD); */
	/* OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_RUN_PROC); */
	/* OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_FIN_PROC); */
	/* OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_TIMEOUT_ATCMD); */

	/* if (isLcdOn) { */
	/* 	char buf[0x10] = ""; */
	/* 	if (MODEM_getResult() == ERROR_CODE_NONE) { */
	/* 		snprintf(buf, 0x10, "SUCCESS"); */
	/* 	} else { */
	/* 		snprintf(buf, 0x10, "FAIL %d", MODEM_getResult()); */
	/* 	} */
	/* 	LCD_displayString(buf); */

	/* 	MISC_delayMs(1000); */
	/* 	LCD_redraw(); */
	/* } */
}

static void str2BcdForImeiAndImsi(uchar *pBCD, char *pString)
{
	for (int i = 0; i < 7; i++) {
		*(pBCD + i) = MISC_asciiToBCD(*(pString + i * 2 + 0),
					      *(pString + i * 2 + 1));
	}

	*(pBCD + 7) = MISC_asciiToBCD(*(pString + 14), 'F');
}

void MODEM_getImei(uchar *imei)
{
	str2BcdForImeiAndImsi(imei, nbInfo.imeiStr);
}

void MODEM_getImsi(uchar *imsi)
{
	str2BcdForImeiAndImsi(imsi, nbInfo.imsiStr);
}

void MODEM_getCtn(uchar *ctn)
{
	// 11자리의 CTN을 BCD로 변환 시 가장 앞자리 0을 1byte로 할당한다.
	// EX :  012-1234-5678 -> 0x00 0x12 0x12 0x34 0x56 0x78
	char *p = nbInfo.ctnStr;
	int len = sizeof(nbInfo.ctnStr) / 2;
	*(ctn + 0) = MISC_asciiToBCD('0', '0');
	for (int i = 1; i < len; i++) {
		*(ctn + i) = MISC_asciiToBCD(*(p + i * 2 - 1), *(p + i * 2));
	}
}

void MODEM_getIccid(uchar *iccid)
{
	char *p = nbInfo.iccidStr;
	int len = sizeof(nbInfo.iccidStr) / 2;
	for (int i = 0; i < len; i++) {
		*(iccid + i) = MISC_asciiToBCD(*(p + i * 2), *(p + i * 2 + 1));
	}
}

void MODEM_getCseId(uchar *cseid)
{
	memcpy(cseid, nbInfo.epName, sizeof(nbInfo.epName));
}

int MODEM_getAbsRssi()
{
	return (abs(nbInfo.sigQuality.rssi) / 10);
}

int MODEM_getAbsRsrp()
{
	return (abs(nbInfo.sigQuality.rsrp) / 10);
}

int MODEM_getAbsRsrq()
{
	return (abs(nbInfo.sigQuality.rsrq) / 10);
}

int MODEM_getSNR()
{
	return (nbInfo.sigQuality.snr / 10);
}

int MODEM_getCid()
{
	return nbInfo.sigQuality.cid;
}

int MODEM_getTxPower()
{
	return (nbInfo.sigQuality.txPower / 10);
}

uint32 MODEM_getCgi()
{
	return nbInfo.sigQuality.cgi;
}

int MODEM_getState()
{
	int status = DEVICE_STATUS_UNKNOWN;
	switch (nbCtx.currentStep) {
	case NBIOT_STEP_FINISH:
		status = DEVICE_STATUS_IDLE;
		break;

	case NBIOT_STEP_IDLE:
	case NBIOT_STEP_INIT:
	case NBIOT_STEP_BIP:
		status = DEVICE_STATUS_CHECK_MODEM;
		break;

	case NBIOT_STEP_ATTACH_NW:
	case NBIOT_STEP_DETACH_NW:
	case NBIOT_STEP_FOTA:
	case NBIOT_STEP_TRANSFER:
		status = DEVICE_STATUS_COMM;
		break;
	}

	return status;
}

int MODEM_getResult()
{
	int error = ERROR_CODE_UNKNOWN;
	if (MODEM_getState() == DEVICE_STATUS_IDLE) {
		switch (nbCtx.error) {
		case NBIOT_ERROR_NONE:
		case NBIOT_ERROR_PF_EPNS_CHANGE: // EPNS 변경 발생. (error는 아님.)
			error = ERROR_CODE_NONE;
			break;

		case NBIOT_ERROR_AT_CMD_NO_RESP: // AT command no response
		case NBIOT_ERROR_AT_CMD_FAIL: // AT command error
			error = ERROR_CODE_MODEM;
			break;

		case NBIOT_ERROR_USIM_INVALID: // AT+CIMI 무응답 OR ERROR
			error = ERROR_CODE_SIM;
			break;

		case NBIOT_ERROR_PF_CERITY_FAIL: // Bootstrap or Register 실패
		case NBIOT_ERROR_PF_UL_FAIL: // Platform 연동 시 UL 실패
		case NBIOT_ERROR_PF_DL_FAIL: // Platform 연동 시 DL 실패
		case NBIOT_ERROR_PF_FOTA_FAIL: // Platform module FOTA 실패
		case NBIOT_ERROR_ATTACH_FAIL: // 망접속 실패
		case NBIOT_ERROR_UDP_UL_FAIL: // UDP UL 실패
		case NBIOT_ERROR_UDP_DL_FAIL: // UDP DL 실패
			error = ERROR_CODE_COMM;
			break;
		}
	}

	return error;
}

Date_t MODEM_getLastAccessTime()
{
	return nbInfo.lastAccessTime;
}

BOOL MODEM_makeCseID(const char *sc, const char *ctnStr, const char *iccidStr,
		     char *buffer, int bufLen)
{
#define LEN_SHORT_UUID 10
#define LEN_UUID_PARAM (LEN_MODEM_CTN + LEN_MODEM_ICCID)

	char uuidParam[LEN_UUID_PARAM + 1];
	char uuid[LEN_MODEM_UUID + 1];
	char shortUuid[LEN_SHORT_UUID + 1];

	// Make parameter for making Endpoint Name
	const char *ctn = (ctnStr == NULL) ? nbInfo.ctnStr : ctnStr;
	const char *iccid = (iccidStr == NULL) ? nbInfo.iccidStr : iccidStr;
	snprintf(uuidParam, LEN_UUID_PARAM + 1, "%s%s", ctn, iccid);

	// Make UUID
	uuid_t u;
	uuid_create_md5_from_name(&u, uuidParam, LEN_UUID_PARAM);
	snprintf(uuid, LEN_MODEM_UUID + 1, "%8.8lx-%4.4x-%4.4x-%2.2x%2.2x-",
		 u.time_low, u.time_mid, u.time_hi_and_version,
		 u.clock_seq_hi_and_reserved, u.clock_seq_low);
	for (uint8 i = 0; i < 6; i++) {
		snprintf(uuid, LEN_MODEM_UUID + 1, "%s%2.2x", uuid, u.node[i]);
	}

	// Make Short UUID from UUID
	snprintf(shortUuid, 6, "%s", &uuid[0]); // UUDI의 앞 5자리 Copy
	snprintf(&shortUuid[5], 6, "%s",
		 &uuid[LEN_MODEM_UUID - 5]); // UUDI의 뒤 5자리 Copy

	// Make Endpoint Name
	char *epName = (char *)malloc(LEN_MODEM_EP_NAME + 1);
	snprintf(epName, LEN_MODEM_EP_NAME + 1, "ASN_CSE-D-%s-%s", shortUuid,
		 sc);

	if (buffer != NULL && bufLen > 0) {
		memcpy(buffer, epName, bufLen);
	} else {
		memcpy(nbInfo.epName, epName, sizeof(nbInfo.epName));
	}
	free(epName);

	return TRUE;
}

uchar *MODEM_getErrorLog(uint8 idx)
{
	return (uchar *)QUEUE_get(nbInfo.errorLog, idx);
}

int MODEM_getTaskId()
{
	return modemCtx.taskId;
}

ModemProc_t MODEM_getCurrentProc()
{
	return modemCtx.currentProc;
}

static void eventRunProc()
{
	if (NBIOT_sendDataProcess()) {
		OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_FIN_PROC);
	} else {
		OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_SEND_ATCMD);
	}
}

static void eventFinish()
{
	// update & print last context
	NBIOT_finishProcess();
	if (modemCtx.currentProc == MODEM_PROC_PERIODIC_REPORT) {
		if (nbCtx.error == NBIOT_ERROR_NONE) {
		}
	}
	APP_ctrlReportTime();
}

static void eventSendCmd()
{
	uint32 timeout = 0;
	if (ATCMD_write(&timeout)) {
		nbCtx.status.error = 0;
		OSAL_startEventTimer(modemCtx.taskId, MODEM_EVENT_TIMEOUT_ATCMD,
				     timeout);
	} else {
		if (nbCtx.currentStep != nbCtx.nextStep) {
			MODEM_DBG("next step (%s)\n",
				  NBIOT_STEP_STR(nbCtx.nextStep));
			nbCtx.currentStep = nbCtx.nextStep;
			modemCtx.retry = 0;

			if (nbCtx.currentStep == NBIOT_STEP_TRANSFER) {
				int len = 0;
				char *data = NULL;
				data = NBMSG_getMeterReport(FALSE, &len);
				NBIOT_registerData(data, len);
			}
		}
		OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_RUN_PROC);
	}
}

static void eventRecvResp()
{
	uint32 nextDelay = 0;
	if (ATCMD_read(&nextDelay)) {
		OSAL_stopEventTimer(modemCtx.taskId, MODEM_EVENT_TIMEOUT_ATCMD);
		if (nbCtx.error != NBIOT_ERROR_NONE) {
			OSAL_startEventTimer(modemCtx.taskId,
					     MODEM_EVENT_FIN_PROC, nextDelay);
		} else {
			OSAL_startEventTimer(modemCtx.taskId,
					     MODEM_EVENT_SEND_ATCMD, nextDelay);
		}
	}
}

static void eventTimeoutCmd()
{
	uint32 timeout = 0;
	if (ATCMD_retry(&timeout)) {
		OSAL_startEventTimer(modemCtx.taskId, MODEM_EVENT_TIMEOUT_ATCMD,
				     timeout);
	} else {
		int maxRetry;
		if (conf.isUsingPF) {
			maxRetry = (nbCtx.currentStep == NBIOT_STEP_BIP) ?
					   MODEM_BIP_RETRY :
					   (nbCtx.currentStep ==
					    NBIOT_STEP_ATTACH_NW) ?
					   MODEM_ATTACH_RETRY :
					   (nbCtx.currentStep ==
					    NBIOT_STEP_TRANSFER) ?
					   MODEM_ONEM2M_RETRY :
					   0;
		} else {
			maxRetry = (nbCtx.currentStep == NBIOT_STEP_BIP) ?
					   MODEM_BIP_RETRY :
					   (nbCtx.currentStep ==
					    NBIOT_STEP_ATTACH_NW) ?
					   MODEM_ATTACH_RETRY :
					   0;
		}
		MODEM_DBG("current retry step count (%d/%d)\n", modemCtx.retry,
			  maxRetry);
		if (modemCtx.retry < maxRetry) {
			BOOL isHwReset = (modemCtx.retry == (maxRetry - 1)) ?
						 TRUE :
						 FALSE;
			BOOL isNeedAttach =
				(nbCtx.currentStep == NBIOT_STEP_TRANSFER) ?
					TRUE :
					FALSE;
			NBIOT_retryProcess(isNeedAttach, isHwReset);

			nbCtx.nextStep =
				nbCtx.currentStep; // Set retry step to NEXT STEP
			modemCtx.retry++;
			OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_SEND_ATCMD);
		} else {
			OSAL_setEvent(modemCtx.taskId, MODEM_EVENT_FIN_PROC);
		}
	}
}

event32_t MODEM_tasks(uint8 taskId, event32_t events)
{
	// 모뎀 종료 후 이미 등록된 event에 대한 처리를 막기위한 처리.
	if (modemCtx.isStop) {
		return 0;
	}

	if (events & MODEM_EVENT_RUN_PROC) {
		eventRunProc();
		return (events ^ MODEM_EVENT_RUN_PROC);
	}

	if (events & MODEM_EVENT_FIN_PROC) {
		eventFinish();
		return (events ^ MODEM_EVENT_FIN_PROC);
	}

	if (events & MODEM_EVENT_SEND_ATCMD) {
		eventSendCmd();
		return (events ^ MODEM_EVENT_SEND_ATCMD);
	}

	if (events & MODEM_EVENT_RECV_ATCMD) {
		eventRecvResp();
		return (events ^ MODEM_EVENT_RECV_ATCMD);
	}

	if (events & MODEM_EVENT_TIMEOUT_ATCMD) {
		eventTimeoutCmd();
		return (events ^ MODEM_EVENT_TIMEOUT_ATCMD);
	}

	return 0;
}
