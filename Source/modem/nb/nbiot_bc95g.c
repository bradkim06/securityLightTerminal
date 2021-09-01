#include "at_cmd.h"
#include "message.h"
#include "modem.h"
#include "nbiot.h"

#include "common.h"
// #include "assert.h"
#include "alarm.h"
#include "flash.h"
#include "misc.h"
#include "rtc.h"
#include "test.h"
#include "sunrise.h"

#if defined(DEBUG)
#define NBIOT_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts("[NBIOT] ");                                                     \
			printf(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0);
#else
#define NBIOT_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts(__VA_ARGS__);                                                    \
		}                                                                                  \
	} while (0);
#endif

#define ATCMD "AT" // No action just response 'OK'
#define ATCMD_MBIPST "AT+MBIPST" // Get BIP status(only TPB23)
#define ATCMD_NRB "AT+NRB" // S/W Reset
#define ATCMD_CIMI "AT+CIMI" // Get IMSI
#define ATCMD_CGSN "AT+CGSN" // Get IMEI (use case : AT+CGSN=1)
#define ATCMD_NCCID "AT+NCCID" // Get ICCID
#define ATCMD_CGMR "AT+CGMR" // GET F/W version
#define ATCMD_CGATT "AT+CGATT" // PS attach/detach
#define ATCMD_CCLK "AT+CCLK" // Get time
#define ATCMD_NUESTATS "AT+NUESTATS" // Get UE statistics
#define ATCMD_CPSMS "AT+CPSMS" // Get/Set PSM setting
#define ATCMD_NBAND "AT+NBAND" // Get/Set band
#define ATCMD_NCONFIG "AT+NCONFIG" // Set UE behaviour
#define ATCMD_CEREG "AT+CEREG" // Get Network registration status
#define ATCMD_NPSMR "AT+NPSMR" // PSM status report
#define ATCMD_CFUN "AT+CFUN" // Set UE functionality
#define ATCMD_NSOCR "AT+NSOCR" // Socket create
#define ATCMD_NSOST "AT+NSOST" // Socket send uplink
#define ATCMD_NSOCL "AT+NSOCL" // Socket close
#define ATCMD_NSORF "AT+NSORF" // Socket receive downlink
#define ATCMD_CEDRXS "AT+CEDRXS" // eDRX Setting

#define ATCMD_QGMR "AT+QGMR" // GET F/W version (BC95G)
#define ATCMD_QBOOTSTRAPHOLDOFF "AT+QBOOTSTRAPHOLDOFF" // Set LWM2M bootstrap mode
#define ATCMD_QREGSWT "AT+QREGSWT" // Switch LWM2M mode
#define ATCMD_QLWSERVERIP "AT+QLWSERVERIP" // Get/Set LWM2M server settings
#define ATCMD_QLWEPNS "AT+QLWEPNS" // Set LWM2M endpoint
#define ATCMD_QLWMBSPS "AT+QLWMBSPS" // Set LWM2M bootstrap parameters
#define ATCMD_QLWFOTAIND "AT+QLWFOTAIND" // Get LWM2M FOTA update mode
#define ATCMD_QLWSREGIND "AT+QLWSREGIND" // Send LwM2M register
#define ATCMD_QLWULDATA "AT+QLWULDATA" // Send LwM2M uplink
#define ATCMD_QLWULDATAEX "AT+QLWULDATAEX" // Send LwM2M confirmable uplink
#define ATCMD_NMGR "AT+NMGR" // Check Downlink

// LWM2M event notify value
#define LWM2M_EVENT_REG_FINISH 0
#define LWM2M_EVENT_REG_DELETE 1
#define LWM2M_EVENT_REG_UPDATE 2
#define LWM2M_EVENT_OBS_OBJ10250 3
#define LWM2M_EVENT_BOOTSTRAP 4
#define LWM2M_EVENT_OBS_OBJ503 5
#define LWM2M_EVENT_FOTA_DOWNLOAD 6
#define LWM2M_EVENT_FOTA_UPDATE 7
#define LWM2M_EVENT_OBS_OBJ16241 8
#define LWM2M_EVENT_CXL_OBJ10250 9

// Macro of converting LWM2M event notify value to string
#define LWM2M_EVENT_STR(x)                                                                         \
	(((x) == LWM2M_EVENT_REG_FINISH) ? "REG FINISH" :                                          \
					   ((x) == LWM2M_EVENT_REG_DELETE) ?                       \
					   "REG DELETE" :                                          \
					   ((x) == LWM2M_EVENT_REG_UPDATE) ?                       \
					   "REG UPDATE" :                                          \
					   ((x) == LWM2M_EVENT_OBS_OBJ10250) ?                     \
					   "OBSERVE OBJ 10250" :                                   \
					   ((x) == LWM2M_EVENT_BOOTSTRAP) ?                        \
					   "BOOTSTRPA" :                                           \
					   ((x) == LWM2M_EVENT_OBS_OBJ503) ?                       \
					   "OBSERVE OBJ 503" :                                     \
					   ((x) == LWM2M_EVENT_FOTA_DOWNLOAD) ?                    \
					   "FOTA DOWNLOAD" :                                       \
					   ((x) == LWM2M_EVENT_FOTA_UPDATE) ?                      \
					   "FOTA UPDATE" :                                         \
					   ((x) == LWM2M_EVENT_OBS_OBJ16241) ?                     \
					   "OBSERVE OBJ 16241" :                                   \
					   ((x) == LWM2M_EVENT_CXL_OBJ10250) ? "CANCEL OBJ "       \
									       "10250" :           \
									       "UNKNOWN")

NbIotContext_t nbCtx;
NbIotInfo_t nbInfo;

static int RegisteredDataLen = 0;
static char *RegisteredData = NULL;

/*
 ***************************************************************
    AT command parser
 ***************************************************************
 */
static void notifyParser(const char *pHead, const int *len)
{
	char *p = NULL;

#define NOTIFY_LWM2M_EVENT "+QLWEVTIND:"
	if ((p = strstr(pHead, NOTIFY_LWM2M_EVENT))) {
		p += strlen(NOTIFY_LWM2M_EVENT);
	}
	while (p != NULL) {
		int event = atoi(p);
		NBIOT_DBG("LWM2M event (%s)\n", LWM2M_EVENT_STR(event));
		switch (event) {
		case LWM2M_EVENT_REG_FINISH: {
			nbCtx.lwm2m.regFinish = 1;
		} break;
		case LWM2M_EVENT_REG_DELETE: {
			nbCtx.lwm2m.regDelete = 1;
			nbCtx.lwm2m.regFinish = 0;
			nbCtx.lwm2m.obsObj10250 = 0;
			nbCtx.lwm2m.obsObj16241 = 0;
		} break;
		case LWM2M_EVENT_REG_UPDATE: {
			nbCtx.lwm2m.regUpdate = 1;
		} break;
		case LWM2M_EVENT_OBS_OBJ10250: {
			nbCtx.lwm2m.obsObj10250 = 1;
		} break;
		case LWM2M_EVENT_BOOTSTRAP: {
			nbCtx.lwm2m.bsFinish = 1;
		} break;
		case LWM2M_EVENT_OBS_OBJ503: {
			nbCtx.lwm2m.obsObj503 = 1;
		} break;
		case LWM2M_EVENT_FOTA_DOWNLOAD: {
			nbCtx.lwm2m.fotaDownReq = 1;
		} break;
		case LWM2M_EVENT_FOTA_UPDATE: {
			nbCtx.lwm2m.fotaUpdateReq = 1;
		} break;
		case LWM2M_EVENT_OBS_OBJ16241: {
			nbCtx.lwm2m.obsObj16241 = 1;
		} break;
		case LWM2M_EVENT_CXL_OBJ10250: {
			nbCtx.lwm2m.cxlObj10250 = 1;
		} break;
		}

		if ((p = strstr(p, NOTIFY_LWM2M_EVENT))) {
			p += strlen(NOTIFY_LWM2M_EVENT);
		}
	}

// Check PSM on/off event
#define PATTERN_CHK_PSM_PREFIX "+NPSMR:"
#define PATTERN_CHK_PSM_PREFIX2 "+NPSMR:1,"
#define PATTERN_ENTER_PSM 1
	if ((p = strstr(pHead, PATTERN_CHK_PSM_PREFIX2))) {
		p += strlen(PATTERN_CHK_PSM_PREFIX2);
		nbCtx.status.psmOn = (atoi(p) == PATTERN_ENTER_PSM) ? 1 : 0;
		printf_ts("MODEM : Get PSM Status(%d).\n", nbCtx.status.psmOn);
	} else if ((p = strstr(pHead, PATTERN_CHK_PSM_PREFIX))) {
		p += strlen(PATTERN_CHK_PSM_PREFIX);
		nbCtx.status.psmOn = (atoi(p) == PATTERN_ENTER_PSM) ? 1 : 0;
		printf_ts("MODEM : event of changing PSM(%d).\n", nbCtx.status.psmOn);
	}

// Check exception service event.  if detected, attach flag clear.
#define PATTERN_CEREG_NOT_REG_EVENT "+CEREG:0"
#define PATTERN_CEREG_ATTACH_EVENT "+CEREG:1"
#define PATTERN_CEREG_TRY_ATTACH_EVENT "+CEREG:2"
#define PATTERN_CEREG_REJECT_EVENT "+CEREG:3"
#define PATTERN_CEREG_NO_SERVICE_PREFIX "+CEREG:4"
	if ((p = strstr(pHead, PATTERN_CEREG_ATTACH_EVENT))) {
		nbCtx.cellreg = NBIOT_CELLREG_ATTACHED;
	} else if ((p = strstr(pHead, PATTERN_CEREG_NOT_REG_EVENT))) {
		nbCtx.lwm2m.regFinish = 0;
		nbCtx.lwm2m.obsObj10250 = 0;
		nbCtx.lwm2m.obsObj16241 = 0;
		nbCtx.cellreg = NBIOT_CELLREG_NOT_REG;
	} else if ((p = strstr(pHead, PATTERN_CEREG_TRY_ATTACH_EVENT))) {
		nbCtx.lwm2m.regFinish = 0;
		nbCtx.lwm2m.obsObj10250 = 0;
		nbCtx.lwm2m.obsObj16241 = 0;
		nbCtx.cellreg = NBIOT_CELLREG_TRY_ATTACH;
	} else if ((p = strstr(pHead, PATTERN_CEREG_REJECT_EVENT))) {
		nbCtx.lwm2m.regFinish = 0;
		nbCtx.lwm2m.obsObj10250 = 0;
		nbCtx.lwm2m.obsObj16241 = 0;
		nbCtx.cellreg = NBIOT_CELLREG_REJECTED;
	} else if ((p = strstr(pHead, PATTERN_CEREG_NO_SERVICE_PREFIX))) {
		nbCtx.lwm2m.regFinish = 0;
		nbCtx.lwm2m.obsObj10250 = 0;
		nbCtx.lwm2m.obsObj16241 = 0;
		nbCtx.cellreg = NBIOT_CELLREG_NO_SERVICE;
	}

#define NOTIFY_ATCMD_ERROR "ERROR"
	if ((p = strstr(pHead, NOTIFY_ATCMD_ERROR))) {
		nbCtx.status.error = 1;
		NBIOT_DBG("AT command error\n");
	}
}

static BOOL parseOk(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		return TRUE;
	}
	return FALSE;
}

static BOOL parseOkWithDelay(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		*nextDelay = DELAY_NBIOT_UPLINK;
		return TRUE;
	}
	return FALSE;
}

static BOOL parseQaReportFinish(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		*nextDelay = DELAY_NBIOT_UPLINK;
		nbCtx.proc.updateQa = 0;
		nbInfo.udpUlCnt = nbInfo.udpDlCnt = 0;
		nbInfo.pfUlCnt = nbInfo.pfDlCnt = 0;
		while (!QUEUE_isEmpty(nbInfo.errorLog)) {
			QUEUE_pop(nbInfo.errorLog);
		}
		RTC_read(&nbInfo.lastQaReportTime);
		return TRUE;
	}
	return FALSE;
}

static BOOL parseDoHwReset(const char *pHead, const int *len, uint32 *nextDelay)
{
	*nextDelay = DELAY_NBIOT_BOOTUP;
	MODEM_turnOff();
	MODEM_turnOn();
	return TRUE;
}

static BOOL parseBootup(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, "REBOOT_CAUSE_")) {
		*nextDelay = DELAY_NBIOT_BOOTUP;
		return TRUE;
	} else if (strstr(pHead, "OK")) {
		return TRUE;
	}
	return FALSE;
}

static BOOL parseLwm2mBootstrap(const char *pHead, const int *len, uint32 *nextDelay)
{
	return (nbCtx.lwm2m.bsFinish) ? TRUE : FALSE;
}

static BOOL parseLwm2mRegStart(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (nbCtx.lwm2m.regFinish && nbCtx.lwm2m.obsObj16241) {
		if (ALARM_isTimeSync()) {
			RTC_read(&nbInfo.lastCertifyTime);
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL parseLwm2mFotaDown(const char *pHead, const int *len, uint32 *nextDelay);
static BOOL parseLwm2mFotaUpdate(const char *pHead, const int *len, uint32 *nextDelay);

static BOOL parseLwm2mFotaWait(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (nbCtx.lwm2m.fotaDownReq) {
		ATCMD_push(TIMEOUT_FOTA_DOWNLOAD, RETRY_LWM2M_FOTA, parseLwm2mFotaDown,
			   ATCMD_QLWFOTAIND "=2");
		ATCMD_push(TIMEOUT_FOTA_UPDATE, RETRY_LWM2M_FOTA, parseLwm2mFotaUpdate,
			   ATCMD_QLWFOTAIND "=4");
		return TRUE;
	}

	return FALSE;
}

static BOOL parseLwm2mFotaDown(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (nbCtx.lwm2m.fotaUpdateReq) {
		return TRUE;
	}
	return FALSE;
}

static BOOL parseLwm2mFotaUpdate(const char *pHead, const int *len, uint32 *nextDelay)
{
#define LWM2M_FOTA_FINISH "REBOOT_CAUSE_SECURITY_FOTA_UPGRADE"
	if (strstr(pHead, LWM2M_FOTA_FINISH)) {
		nbCtx.proc.runFota = 0;
		nbCtx.lwm2m.fotaFinish = 1;

		conf.isModemInit = FALSE;
		FLASH_saveConfigInfo(&conf);

		*nextDelay = TIMEOUT_FOTA_COMPLETE;
		return TRUE;
	}
	return FALSE;
}

static BOOL parseNRB(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_NRB_CAUSE_APP "REBOOT_CAUSE_"
	if (strstr(pHead, ATCMD_RESP_NRB_CAUSE_APP)) {
		nbCtx.lwm2m.value = 0;
		return TRUE;
	}

	return FALSE;
}

static BOOL parseCGSN(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_CGSN "+CGSN:"

	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_CGSN))) {
		p = p + strlen(ATCMD_RESP_CGSN);
		if (*len < LEN_MODEM_IMEI_IMSI) {
			return FALSE;
		}
		for (int i = 0; i < LEN_MODEM_IMEI_IMSI; i++) {
			if (*p >= '0' && *p <= '9') {
				nbInfo.imeiStr[i] = *p++;
			} else {
				return FALSE;
			}
		}

		uchar imei[8];
		MODEM_getImei(imei);
		if (memcmp(conf.imei, imei, 8) != 0) {
			memcpy(conf.imei, imei, 8);
			conf.isModemInit = 0;
			FLASH_saveConfigInfo(&conf);
			NBIOT_DBG("IMEI is changed\n");
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL parseCIMI(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_CIMI IMSI_MCC_KOREA

	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_CIMI))) {
		if (*len < LEN_MODEM_IMEI_IMSI) {
			return FALSE;
		}
		for (int i = 0; i < LEN_MODEM_IMEI_IMSI; i++) {
			if (*p >= '0' && *p <= '9') {
				nbInfo.imsiStr[i] = *p++;
			} else {
				return FALSE;
			}
		}

#define IMSI_UPLUS_VALID IMSI_MCC_MNC_UPLUS "12"
		if (strncmp(nbInfo.imsiStr, IMSI_UPLUS_VALID, strlen(IMSI_UPLUS_VALID)) == 0) {
			snprintf(nbInfo.ctnStr, LEN_MODEM_CTN + 1, "0%s", &nbInfo.imsiStr[5]);

			uchar imsi[8];
			memset(imsi, 0, sizeof(imsi));
			MODEM_getImsi(imsi);
			if (memcmp(conf.imsi, imsi, sizeof(imsi)) != 0) {
				memcpy(conf.imsi, imsi, 8);
				conf.isModemInit = 0;
				nbCtx.cellreg = NBIOT_CELLREG_NOT_REG;
				FLASH_saveConfigInfo(&conf);
				NBIOT_DBG("USIM is changed\n");
			}

			return TRUE;
		}
	}
	return FALSE;
}

static BOOL parseNCCID(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_NCCID "+NCCID:"

	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_NCCID))) {
		p = p + strlen(ATCMD_RESP_NCCID);
		if (*len < LEN_MODEM_ICCID) {
			return FALSE;
		}
		for (int i = 0; i < LEN_MODEM_ICCID; i++) {
			if (*p >= '0' && *p <= '9') {
				nbInfo.iccidStr[i] = *p++;
			} else {
				return FALSE;
			}
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL parseQLWEPNS(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_QLWEPNS "+QLWEPNS: "

	BOOL isSetCfg = FALSE;
	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_QLWEPNS))) {
		p += strlen(ATCMD_RESP_QLWEPNS);
		if (strncmp((const char *)p, (const char *)nbInfo.epName, strlen(nbInfo.epName)) ==
		    0) {
		} else {
			NBIOT_DBG("LWM2M endpoint name is changed \n");
			isSetCfg = TRUE;
		}
	} else {
		if (nbCtx.status.error) {
			nbCtx.status.error = 0;
			NBIOT_DBG("LWM2M endpoint name have to set \n");
			isSetCfg = TRUE;
		}
	}

	if (isSetCfg) {
		nbCtx.status.setCfg = 1;
		conf.isModemInit = 0;
		ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			      ATCMD_QLWEPNS "=1,\"%s\"", nbInfo.epName);
	}

	return TRUE;
}

static BOOL parseQLWSERVERIP(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_QLWSERVERIP_BS "+QLWSERVERIP:BS,"
#define ATCMD_RESP_QLWSERVERIP_LWM2M "+QLWSERVERIP:LWM2M,"
	BOOL isSetCfg = FALSE;
	char addr[22];

	snprintf(addr, sizeof(addr), "%s,%d", conf.serverIp, conf.serverPort);

	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_QLWSERVERIP_BS))) {
		p += strlen(ATCMD_RESP_QLWSERVERIP_BS);
		if (strncmp((const char *)p, (const char *)addr, strlen(addr)) == 0) {
			return TRUE;
		} else {
			NBIOT_DBG("BS server address is not matched \n");
			isSetCfg = TRUE;
		}
	} else {
		if (nbCtx.status.error) {
			nbCtx.status.error = 0;
			NBIOT_DBG("BS server address have to set \n");
			isSetCfg = TRUE;
		}
	}

	if (isSetCfg) {
		nbCtx.status.setCfg = 1;
		conf.isModemInit = 0;

		ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			      ATCMD_QLWSERVERIP "=BS,%s", addr);
		ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			      ATCMD_QLWSERVERIP "=LWM2M,%s", addr);

		return TRUE;
	}

	return FALSE;
}

static BOOL parseQLWMBSPS(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_QLWMBSPS "+QLWMBSPS: "
#define ATCMD_RESP_ERROR "ERROR"

	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_QLWMBSPS))) {
		p += strlen(ATCMD_RESP_QLWMBSPS);
		int len = LEN_MODEM_BS_PARAM;
		char str[LEN_MODEM_BS_PARAM];
		memcpy(str, p, len);

		char *pStr = str;
		for (; *pStr != '\0'; pStr++) { //종료 문자를 만날 때까지 반복
			if (*pStr == '\"') {
				strcpy(pStr, pStr + 1);
				len--;
				pStr--;
			}
			if (len-- <= 0)
				break;
		}

		if (strncmp((const char *)str, (const char *)nbInfo.bsParam,
			    strlen(nbInfo.bsParam)) != 0) {
			printf_ts("LWM2M : bootstrap parameter is not matched\n");
			ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
				      ATCMD_QLWMBSPS "=%s", nbInfo.bsParam);
		} else {
			printf_ts("LWM2M : bootstrap parameter is matched\n");
		}
		return TRUE;
	} else if ((p = strstr(pHead, ATCMD_RESP_ERROR))) {
		printf_ts("LWM2M : bootstrap parameter init\n");
		ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			      ATCMD_QLWMBSPS "=%s", nbInfo.bsParam);
		return TRUE;
	}
	return FALSE;
}

static BOOL parseQREGSWT(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_QREGSWT "+QREGSWT:"
#define DISABLE_LWM2M 2
#define MANUAL_LWM2M 0
	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_QREGSWT))) {
		int status = atoi(p + strlen(ATCMD_RESP_QREGSWT));

		// LWM2M On/Off switching
		if (nbCtx.proc.runFota) {
			if (status != MANUAL_LWM2M) {
				nbCtx.status.setCfg = 1;
				ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
					      ATCMD_QREGSWT "=0");
			}
		}

		// Set FOTA manual working if LWM2M on
		if (status != DISABLE_LWM2M && !conf.isModemInit) {
			nbCtx.status.setCfg = 1;
			ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
				      ATCMD_QLWFOTAIND "=1");
		}

		return TRUE;
	}
	return FALSE;
}

static BOOL parseQGMR(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		NBIOT_DBG("IMEI     : %s\n", nbInfo.imeiStr);
		NBIOT_DBG("IMSI     : %s\n", nbInfo.imsiStr);
		NBIOT_DBG("CTN      : %s\n", nbInfo.ctnStr);
		NBIOT_DBG("ICCID    : %s\n", nbInfo.iccidStr);

		// Make End point name (= CSE ID)
		if (MODEM_makeCseID(conf.serviceCode, NULL, NULL, NULL, 0)) {
			NBIOT_DBG("Endpoint : %s\n", nbInfo.epName);
		}

		// Make Bootstrap paramter
		char model[TERM_MODEL_STRING_LEN + 1];
		memcpy(model, "HAT-124Q", TERM_MODEL_STRING_LEN + 1);

		// char devSn[13];
		// // Bootstrap 상의 serial number 는 IMEI의 13자리 수만 사용.
		// // EX : IMEI=359689090501559 >> devSn=968909050155
		// memcpy(devSn, &nbInfo.imeiStr[2], 12);
		// devSn[12] = 0; // Last char is set string end
		char devSn[11];
		// Bootstrap 상의 serial number 는 ICCID의 10자리 사용.
		// USIM Model num 4 + Usim S/N num 6
		// EX : ICCID = 8982068086002814399 >> devSn = 8600281439
		memcpy(devSn, &nbInfo.iccidStr[LEN_MODEM_ICCID - 11], 10);
		devSn[10] = 0; // Last char is set string end

		snprintf(nbInfo.bsParam, LEN_MODEM_BS_PARAM + 1, "%s,%s,%s,%s,%s", conf.serviceCode,
			 devSn, nbInfo.ctnStr, &nbInfo.iccidStr[LEN_MODEM_ICCID - 6], model);

		NBIOT_DBG("BS param : %s\n", nbInfo.bsParam);
		return TRUE;
	}
	return FALSE;
}

static BOOL parseCGATT(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		// CGATT only use when attached modem detach
		nbCtx.cellreg = NBIOT_CELLREG_NOT_REG;
		*nextDelay = DELAY_NBIOT_UPLINK;
		return TRUE;
	}
	return FALSE;
}

static BOOL parseCEREG(const char *pHead, const int *len, uint32 *nextDelay)
{
	char *p = NULL;
#define PATTERN_CEREG_PREFIX "+CEREG:5"
	if ((p = strstr(pHead, PATTERN_CEREG_PREFIX))) {
#define CEREG_ATTACHED 1
#define CEREG_NOT_REG 2
#define CEREG_REJECTED 3
#define CEREG_UNKNOWN 4
#define CEREG_ROAMING 5

#define CEREG_STATUS_POS 1
#define CEREG_REJECT_CAUSE_POS 6
		int pos = 0;
		char *token = strtok(p, ",");
		while (token != NULL) {
			if (pos == CEREG_STATUS_POS) {
				int state = atoi(token);
				if (state == CEREG_ATTACHED) {
					nbCtx.cellreg = NBIOT_CELLREG_ATTACHED;
					break;
				} else {
					// PSM On -> Off 전환 시 Cellreg 변화 체크
					if (state == CEREG_REJECTED) {
						nbCtx.cellreg = NBIOT_CELLREG_REJECTED;
					} else {
						nbCtx.cellreg = NBIOT_CELLREG_NOT_REG;
					}
					nbCtx.lwm2m.regFinish = 0;
					nbCtx.lwm2m.obsObj10250 = 0;
					nbCtx.lwm2m.obsObj16241 = 0;
				}
			}

			token = strtok(NULL, ",");
			pos++;
		}
	}

	if (nbCtx.cellreg == NBIOT_CELLREG_ATTACHED) {
		return TRUE;
	}
	return FALSE;
}

static BOOL parseCCLK(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_CCLK "+CCLK:"
	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_CCLK))) {
		p = p + strlen(ATCMD_RESP_CCLK);

		Date_t date;
		date.year = atoi(p) + 2000;
		date.mon = atoi(p + 3);
		date.day = atoi(p + 6);
		date.hour = atoi(p + 9);
		date.min = atoi(p + 12);
		date.sec = atoi(p + 15);

		if (MISC_isValidDate(&date) == 0) {
			return FALSE;
		}
		ALARM_writeTime(&date);
		RTC_read(&nbInfo.lastAttachTime);

		if (modemCtx.currentProc == MODEM_PROC_INIT) {
			updateSunTime(&date);
			APP_runMetering(NO_REPORT, AUTO);
		}
		return TRUE;
	}

	return FALSE;
}

static BOOL parseNUESTATS(const char *pHead, const int *len, uint32 *nextDelay)
{
#define ATCMD_RESP_NUESTATS "NUESTATS:RADIO,"
	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_NUESTATS))) {
		p = p + strlen(ATCMD_RESP_NUESTATS);

#define NUESTATS_RSRP "Signal power:"
#define NUESTATS_RSRQ "RSRQ:"
#define NUESTATS_SNR "SNR:"
#define NUESTATS_RSSI "Total power:"
#define NUESTATS_PCI "PCI:"
#define NUESTATS_CGI "Cell ID:"
#define NUESTATS_TX_POWER "TX power:"
		// NUESTATS:RADIO,Cell ID:51939434
		// NUESTATS:RADIO,TX power:-90

		char *cgi_pos = strstr(p, NUESTATS_CGI);
		if (cgi_pos) {
			cgi_pos = cgi_pos + strlen(NUESTATS_CGI);
			nbInfo.sigQuality.cgi = atol(cgi_pos);
		} else {
			return FALSE;
		}

		char *pci_pos = strstr(p, NUESTATS_PCI);
		if (pci_pos) {
			pci_pos = pci_pos + strlen(NUESTATS_PCI);
			nbInfo.sigQuality.cid = atoi(pci_pos);
		} else {
			return FALSE;
		}

		char *tx_power_pos = strstr(p, NUESTATS_TX_POWER);
		if (tx_power_pos) {
			tx_power_pos = tx_power_pos + strlen(NUESTATS_TX_POWER);
			nbInfo.sigQuality.txPower = atoi(tx_power_pos);
		} else {
			return FALSE;
		}

		char *rsrp_pos = strstr(p, NUESTATS_RSRP);
		if (rsrp_pos) {
			rsrp_pos = rsrp_pos + strlen(NUESTATS_RSRP);
			nbInfo.sigQuality.rsrp = atol(rsrp_pos);
		} else {
			return FALSE;
		}

		char *snr_pos = strstr(p, NUESTATS_SNR);
		if (snr_pos) {
			snr_pos = snr_pos + strlen(NUESTATS_SNR);
			nbInfo.sigQuality.snr = atol(snr_pos);
		} else {
			return FALSE;
		}

		char *rsrq_pos = strstr(p, NUESTATS_RSRQ);
		if (rsrq_pos) {
			rsrq_pos = rsrq_pos + strlen(NUESTATS_RSRQ);
			nbInfo.sigQuality.rsrq = atoi(rsrq_pos);
		} else {
			return FALSE;
		}

		char *rssi_pos = strstr(p, NUESTATS_RSSI);
		if (rssi_pos) {
			rssi_pos = rssi_pos + strlen(NUESTATS_RSSI);
			nbInfo.sigQuality.rssi = atoi(rssi_pos);
		} else {
			return FALSE;
		}

		printf_ts(
			"MODEM : CGI(%ld) CID(%d) TX_POWER(%d) RSRP(%d) SNR(%d) RSRQ(%d) RSSI(%d) \n",
			nbInfo.sigQuality.cgi, nbInfo.sigQuality.cid, nbInfo.sigQuality.txPower,
			nbInfo.sigQuality.rsrp, nbInfo.sigQuality.snr, nbInfo.sigQuality.rsrq,
			nbInfo.sigQuality.rssi);

		return TRUE;
	}
	return FALSE;
}

static BOOL parseNSOCR(const char *pHead, const int *len, uint32 *nextDelay)
{
	char *p = NULL;
	if ((p = strstr(pHead, ATCMD_RESP_DEFAULT))) {
		nbCtx.socket = atoi(p - 5);
		return TRUE;
	}

	return FALSE;
}

static uint8 ascii2Hex(char ch)
{
	uint8 hex = 0;

	if (ch >= 'A' && ch <= 'F') {
		hex = (ch - 'A') + 10;
	} else if (ch >= 'a' & ch <= 'f') {
		hex = (ch - 'a') + 10;
	} else if (ch >= '0' & ch <= '9') {
		hex = ch - '0';
	}
	return hex;
}

static BOOL parseNNMI(const char *pHead, const int *len, uint32 *nextDelay)
{
}

static BOOL parseQLWULDATAEX(const char *pHead, const int *len, uint32 *nextDelay)
{
	if (strstr(pHead, ATCMD_RESP_DEFAULT)) {
		nbInfo.pfUlCnt++;
	}

#define LWM2M_UPLINK_STATUS "+QLWULDATASTATUS:4"
	char *p = NULL;
	if ((p = strstr(pHead, LWM2M_UPLINK_STATUS))) {
		nbCtx.status.waitDL = 1;
	}

#define LWM2M_DOWNLINK "+NNMI:"
	if ((p = strstr(pHead, LWM2M_DOWNLINK))) {
		nbInfo.pfDlCnt++;

		p += strlen(LWM2M_DOWNLINK);
		int len = atoi(p);

		if (len > LEN_MAX_DL_DATA) {
			printf_ts("DL : data is too long (max : %d, current :%d)\n",
				  LEN_MAX_DL_DATA, len);
			return FALSE;
		} else if (len == 0) {
			printf_ts("DL : data empty (current :%d)\n", LEN_MAX_DL_DATA, len);
			return FALSE;
		}

		printf_ts("DL : data len (current :%d)\n", len);

		// skip first comma
		if ((p = strstr(p, ",")) == NULL) {
			printf_ts("DL : can't find comma\n");
			return FALSE;
		}
		p++;

		uchar dlData[len];
		for (int i = 0; i < len; i++) {
			dlData[i] =
				ascii2Hex(*(p + i * 2 + 0)) * 0x10 + ascii2Hex(*(p + i * 2 + 1));
		}

		NBMSG_recvAckMsg(dlData, len);
		nbCtx.status.waitDL = 0;
		return TRUE;
	}
	return FALSE;
}

/*
 ***************************************************************
    AT command register
 ***************************************************************
 */

static void idle()
{
	// platform 연동 시 FOTA 요청을 받은 이후 FOTA모드로만 동작하므로
	// 매번 FOTA 요청 여부를 매번 확인한다. (FOTA 요청받은 후 데이터 통신 X)
	nbCtx.proc.runFota = 1;
	nbCtx.proc.updateQa = 0;
	nbCtx.proc.updateReg = 0;

	// FOTA관련 FLAG는 동작 여부와 관계없이 매번 초기화
	nbCtx.lwm2m.fotaDownReq = 0;
	nbCtx.lwm2m.fotaUpdateReq = 0;
	nbCtx.lwm2m.fotaFinish = 0;

	Date_t now;
	RTC_read(&now);
	if (ALARM_isTimeSync()) {
		if (MISC_calcHourDiff(&nbInfo.lastQaReportTime, &now) >= NBIOT_QA_REPORT_INTERVAL) {
			nbCtx.proc.updateQa = 1;
		}

		if (MISC_calcHourDiff(&nbInfo.lastCertifyTime, &now) >= NBIOT_ONEM2M_REG_INTERVAL) {
			// MODEM PSM이 아닌 경우 update register를 수행하지 않는다. (초기 인증
			// 재 수행)
			nbCtx.proc.updateReg = (nbCtx.status.psmOn) ? 1 : 0;
		}
	} else {
		nbCtx.proc.updateReg = 0;
		nbCtx.proc.updateQa = 1;
	}

	if (nbCtx.proc.updateReg) {
		NBIOT_DBG("procedure enable, UPDATE REG.\n");
	}
	if (nbCtx.proc.updateQa) {
		NBIOT_DBG("procedure enable, REPORT QA.\n");
	}
	if (nbCtx.proc.runFota) {
		NBIOT_DBG("procedure enable, RUN FOTA.\n");
	}
}

static void bip()
{
	if (!nbCtx.status.psmOn) {
		ATCMD_push(TIMEOUT_NRB, RETRY_NRB, parseBootup, ATCMD);
	}
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_CPSMS "=0");
	ATCMD_push(TIMEOUT_BIP, RETRY_BIP, parseCIMI, ATCMD_CIMI);
}

static void init()
{
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseNCCID, ATCMD_NCCID);
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseCGSN, ATCMD_CGSN "=1");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseQGMR, ATCMD_QGMR);
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseQREGSWT, ATCMD_QREGSWT "?");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseQLWEPNS, ATCMD_QLWEPNS "?");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseQLWSERVERIP,
		   ATCMD_QLWSERVERIP "?");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseQLWMBSPS, ATCMD_QLWMBSPS "?");

	if (!conf.isModemInit) {
		ATCMD_push(TIMEOUT_NRB, RETRY_NRB, parseNRB, ATCMD_NRB);
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_CFUN "=0");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_CPSMS "=0");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_NBAND "=5");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			   ATCMD_NCONFIG "=CELL_RESELECTION,TRUE");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk,
			   ATCMD_QBOOTSTRAPHOLDOFF "=0");
		nbCtx.status.setCfg = 1;
	}
}

static void attach()
{
	if (!conf.isModemInit) {
		conf.isModemInit = TRUE;
		FLASH_saveConfigInfo(&conf);
	}

	if (nbCtx.status.setCfg) {
		nbCtx.status.setCfg = 0;
		ATCMD_push(TIMEOUT_NRB, RETRY_NRB, parseNRB, ATCMD_NRB);
	}
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_CEREG "=5");
	ATCMD_push(TIMEOUT_ATTACH, RETRY_ATTACH, parseCEREG, ATCMD_CEREG "?");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_NPSMR "=1");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseCCLK, ATCMD_CCLK "?");
	ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseNUESTATS,
		   ATCMD_NUESTATS "=RADIO");
}

static void detach()
{
	if (nbCtx.cellreg == NBIOT_CELLREG_ATTACHED) {
		if (nbCtx.lwm2m.regFinish) {
			ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOkWithDelay,
				   ATCMD_QLWSREGIND "=1");
		}
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseCGATT,
			   ATCMD_CGATT "=0");
	}
}

static void transfer()
{
	if (RegisteredData == NULL || RegisteredDataLen <= 0) {
		NBIOT_DBG("Not register sending data.\n");
		return;
	}

	// PSM on 상태인 경우 데이터 전송을 위해 강제 off설정
	nbCtx.status.psmOn = 0;
	nbCtx.proc.updateQa = 0;

	if (nbCtx.proc.updateQa) {
		ATCMD_Instant(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseNSOCR,
			      ATCMD_NSOCR "=DGRAM,17,560,0");

		int qaMsgLen = 0;
		char *qaMsg = NBMSG_getQaReport(&qaMsgLen);
		// No wait response
		ATCMD_push(TIMEOUT_UDP_SEND, RETRY_UDP_SEND, parseOkWithDelay,
			   ATCMD_NSOST "=%d,%s,%ld,%d,%s", nbCtx.socket, ONEM2M_QA_SERVER_IP,
			   ONEM2M_QA_SERVER_PORT, qaMsgLen, qaMsg);
		free(qaMsg);

		int errMsgLen = 0;
		char *errMsg = NBMSG_getErrorReport(&errMsgLen);
		// No wait response
		ATCMD_push(TIMEOUT_UDP_SEND, RETRY_UDP_SEND, parseQaReportFinish,
			   ATCMD_NSOST "=%d,%s,%ld,%d,%s", nbCtx.socket, ONEM2M_ERR_SERVER_IP,
			   ONEM2M_ERR_SERVER_PORT, errMsgLen, errMsg);
		free(errMsg);
		// 플랫폼 연동의 경우 UDP 통신이 더 이상 없으므로 socket close를 무조건
		// 실행.
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_NSOCL "=%d",
			   nbCtx.socket);
	}

	if (!nbCtx.lwm2m.bsFinish) {
		ATCMD_push(TIMEOUT_LWM2M_RECV, RETRY_LWM2M_RECV, parseLwm2mBootstrap, ATCMD);
	}
	if (!nbCtx.lwm2m.regFinish || !nbCtx.lwm2m.obsObj16241) {
		ATCMD_push(TIMEOUT_LWM2M_RECV, RETRY_LWM2M_RECV, parseLwm2mRegStart,
			   ATCMD_QLWSREGIND "=0");
	}
	ATCMD_push(TIMEOUT_LWM2M_RECV, RETRY_LWM2M_RECV, parseQLWULDATAEX,
		   ATCMD_QLWULDATAEX "=0,%d,%s,0x0100,0", RegisteredDataLen, RegisteredData);
	// Free original Data afeter Seding Uplink registration is finished
	free(RegisteredData);
	RegisteredData = NULL;
	RegisteredDataLen = 0;
}

static void fota()
{
	ATCMD_push(TIMEOUT_FOTA_DOWN_REQ, RETRY_LWM2M_FOTA, parseLwm2mFotaWait, ATCMD);
}

void NBIOT_retryProcess(BOOL isNeedAttach, BOOL isHwReset)
{
	ATCMD_release();

	if (nbCtx.cellreg == NBIOT_CELLREG_ATTACHED) {
		if (nbCtx.lwm2m.regFinish) {
			ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOkWithDelay,
				   ATCMD_QLWSREGIND "=1");
		}
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseCGATT,
			   ATCMD_CGATT "=0");
	}

	if (isHwReset) {
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseDoHwReset, ATCMD);
	} else {
		ATCMD_push(TIMEOUT_NRB, RETRY_NRB, parseNRB, ATCMD_NRB);
	}

	if (isNeedAttach) {
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_CEREG "=5");
		ATCMD_push(TIMEOUT_ATTACH, RETRY_ATTACH, parseCEREG, ATCMD_CEREG "?");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseOk, ATCMD_NPSMR "=1");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseCCLK, ATCMD_CCLK "?");
		ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseNUESTATS,
			   ATCMD_NUESTATS "=RADIO");
		if (nbCtx.proc.updateQa) {
			ATCMD_push(TIMEOUT_NBIOT_DEFAULT, RETRY_NBIOT_DEFAULT, parseNSOCR,
				   ATCMD_NSOCR "=DGRAM,17,560,0");
		}

		int len = 0;
		char *data = NULL;
		data = NBMSG_getMeterReport(FALSE, &len);
		NBIOT_registerData(data, len);
	}
}

BOOL NBIOT_sendDataProcess()
{
	if (nbCtx.currentStep == NBIOT_STEP_IDLE) {
		idle();
		nbCtx.nextStep = NBIOT_STEP_BIP;
	} else if (nbCtx.currentStep == NBIOT_STEP_BIP) {
		bip();
		nbCtx.nextStep = NBIOT_STEP_INIT;
	} else if (nbCtx.currentStep == NBIOT_STEP_INIT) {
		init();
		nbCtx.nextStep = NBIOT_STEP_ATTACH_NW;
	} else if (nbCtx.currentStep == NBIOT_STEP_ATTACH_NW) {
		attach();
		nbCtx.nextStep = NBIOT_STEP_TRANSFER;
	} else if (nbCtx.currentStep == NBIOT_STEP_TRANSFER) {
		transfer();
		nbCtx.nextStep = NBIOT_STEP_FOTA;
	} else if (nbCtx.currentStep == NBIOT_STEP_FOTA) {
		if (nbCtx.lwm2m.fotaDownReq) {
			fota();
		}
		nbCtx.nextStep = NBIOT_STEP_FINISH;
	} else if (nbCtx.currentStep == NBIOT_STEP_DETACH_NW) {
		nbCtx.nextStep = NBIOT_STEP_FINISH;
	} else if (nbCtx.currentStep == NBIOT_STEP_FINISH) {
		if (nbCtx.cellreg != NBIOT_CELLREG_ATTACHED) {
			nbCtx.status.psmOn = 0;
		}
		return TRUE;
	} else {
		printf_ts("Unknown Step\n");
	}
	return FALSE;
}

// void NBIOT_sendData(char* data, int len)
void NBIOT_registerData(char *data, int len)
{
	if (RegisteredData != NULL) {
		free(RegisteredData);
	}

	RegisteredData = data;
	RegisteredDataLen = len;
}

void NBIOT_init()
{
	memset(&nbCtx, 0, sizeof(NbIotContext_t));
	memset(&nbInfo, 0, sizeof(NbIotInfo_t));
	nbCtx.currentStep = NBIOT_STEP_FINISH;
	nbCtx.nextStep = nbCtx.currentStep;
	nbInfo.errorLog = QUEUE_malloc(NULL);

	if (RegisteredData != NULL) {
		free(RegisteredData);
	}
	RegisteredData = NULL;
	RegisteredDataLen = 0;

	ATCMD_init(notifyParser);
	ATCMD_release();
}

void NBIOT_open()
{
	// Process 시작 전 error code 초기화
	nbCtx.error = NBIOT_ERROR_NONE;
	// Process 시작 전 step 초기화
	nbCtx.currentStep = NBIOT_STEP_IDLE;
	nbCtx.nextStep = NBIOT_STEP_IDLE;
	// Process 시작 전 status flag 초기화
	nbCtx.status.error = 0;
	nbCtx.status.setCfg = 0;
	nbCtx.status.waitDL = 0;

	ATCMD_release();
}

void NBIOT_close()
{
	// initialize context
	nbCtx.currentStep = NBIOT_STEP_FINISH;
	nbCtx.nextStep = nbCtx.currentStep;

	ATCMD_release();
}

static NbIotError_t getErrorCode(AtCmd_t *atCmd)
{
	// If FOTA request is not exist and psm on, that is not fail
	if (nbCtx.currentStep == NBIOT_STEP_FOTA) {
		if (!nbCtx.lwm2m.fotaDownReq && nbCtx.status.psmOn) {
			return NBIOT_ERROR_NONE;
		}
	}

	if (atCmd != NULL) {
		char *p = strtok(atCmd->cmd, "=");
		if (strstr(p, ATCMD_CIMI)) {
			return NBIOT_ERROR_USIM_INVALID;
		} else if (strstr(p, ATCMD_QLWULDATAEX)) {
			return (nbCtx.status.waitDL) ? NBIOT_ERROR_UDP_DL_FAIL :
						       NBIOT_ERROR_UDP_UL_FAIL;
		} else if (strstr(p, ATCMD_NSOST)) {
			return (nbCtx.status.waitDL) ? NBIOT_ERROR_UDP_DL_FAIL :
						       NBIOT_ERROR_UDP_UL_FAIL;
		} else {
			// If working dose not belong to a specific case, set no resp or cmd fail
			if (nbCtx.currentStep == NBIOT_STEP_ATTACH_NW) {
				if (nbCtx.cellreg != NBIOT_CELLREG_ATTACHED) {
					return NBIOT_ERROR_ATTACH_FAIL;
				}
			}

			if (nbCtx.currentStep == NBIOT_STEP_TRANSFER) {
				if (!nbCtx.lwm2m.obsObj16241) {
					return NBIOT_ERROR_PF_CERITY_FAIL;
				}
			}

			if (nbCtx.currentStep == NBIOT_STEP_FOTA) {
				if (!nbCtx.lwm2m.obsObj10250) { // 인증 후 Data 전송 가능 Event 미발생
					return NBIOT_ERROR_PF_CERITY_FAIL;
				} else if (!nbCtx.lwm2m.fotaFinish) {
					if (nbCtx.lwm2m.fotaDownReq) {
						return NBIOT_ERROR_PF_FOTA_FAIL;
					}
				}
			}

			return (nbCtx.status.error) ? NBIOT_ERROR_AT_CMD_FAIL :
						      NBIOT_ERROR_AT_CMD_NO_RESP;
		}
	} else {
		return NBIOT_ERROR_AT_CMD_NO_RESP;
	}
}

void NBIOT_finishProcess()
{
	// define error code and logging error
	// save & print last context
	RTC_read(&nbInfo.lastAccessTime);
	if (nbCtx.currentStep != NBIOT_STEP_FINISH && nbCtx.error == NBIOT_ERROR_NONE) {
		nbCtx.error = getErrorCode(ATCMD_pull());
	}
	NBIOT_DBG("stop working (step : %s, error : %02Xh)\n", NBIOT_STEP_STR(nbCtx.currentStep),
		  nbCtx.error);

	// set step FINISH_STEP after get error code.
	nbCtx.currentStep = NBIOT_STEP_FINISH;
	nbCtx.nextStep = NBIOT_STEP_FINISH;

	// logging NB-IoT working error
	if (ALARM_isTimeSync() && nbCtx.error != NBIOT_ERROR_NONE) {
		uchar *log = (uchar *)malloc(LEN_NBIOT_ERROR_LOG);
		uint32 temp = nbInfo.lastAccessTime.year - 2000;
		MISC_intToBCD(&temp, (log + 0), 1);
		temp = nbInfo.lastAccessTime.mon;
		MISC_intToBCD(&temp, (log + 1), 1);
		temp = nbInfo.lastAccessTime.day;
		MISC_intToBCD(&temp, (log + 2), 1);
		temp = nbInfo.lastAccessTime.hour;
		MISC_intToBCD(&temp, (log + 3), 1);
		temp = nbInfo.lastAccessTime.min;
		MISC_intToBCD(&temp, (log + 4), 1);
		temp = nbInfo.lastAccessTime.sec;
		MISC_intToBCD(&temp, (log + 5), 1);
		*(log + 6) = nbCtx.error;

		// If error log size is over, pop data.
		if (QUEUE_size(nbInfo.errorLog) >= NUM_OF_NBIOT_ERROR_LOG) {
			QUEUE_pop(nbInfo.errorLog);
		}
		QUEUE_push(nbInfo.errorLog, log);
	}

#if defined(DEBUG)
	printf("=============================================================\n");
	printf("[ MODEM STATUS ] CMD ERROR(%d) PSM(%d) SET MODEM(%d) WAIT DL(%d)\n",
	       nbCtx.status.error, nbCtx.status.psmOn, nbCtx.status.setCfg, nbCtx.status.waitDL);
	printf("[ PROCESS      ] QA REPORT(%d) UPDATE REG(%d) FOTA(%d)\n", nbCtx.proc.updateQa,
	       nbCtx.proc.updateReg, nbCtx.proc.runFota);
	printf("[ OPERATION    ] LAST(%s) NEXT(%s) ERROR(%02Xh) CEREG(%d)\n",
	       NBIOT_STEP_STR(nbCtx.currentStep), NBIOT_STEP_STR(nbCtx.nextStep), nbCtx.error,
	       nbCtx.cellreg);
	printf("[ LWM2M STATUS ] REG FIN/UD/DEL(%d/%d/%d) BS(%d) 16241(%d)\n",
	       nbCtx.lwm2m.regFinish, nbCtx.lwm2m.regUpdate, nbCtx.lwm2m.regDelete,
	       nbCtx.lwm2m.bsFinish, nbCtx.lwm2m.obsObj16241);
	printf("[ FOTA STATUS  ] DOWN REQ(%d) UPDATE REQ(%d) FINISH(%d)\n", nbCtx.lwm2m.fotaDownReq,
	       nbCtx.lwm2m.fotaUpdateReq, nbCtx.lwm2m.fotaFinish);
	printf("[ UL/DL COUNT  ] UDP : %d/%d, LWM2M : %d/%d\n", nbInfo.udpUlCnt, nbInfo.udpDlCnt,
	       nbInfo.pfUlCnt, nbInfo.pfDlCnt);
	printf("[ ATTACH TIME  ] %04d-%02d-%02d %02d:%02d:%02d\n", nbInfo.lastAttachTime.year,
	       nbInfo.lastAttachTime.mon, nbInfo.lastAttachTime.day, nbInfo.lastAttachTime.hour,
	       nbInfo.lastAttachTime.min, nbInfo.lastAttachTime.sec);
	printf("[ QA SEND TIME ] %04d-%02d-%02d %02d:%02d:%02d\n", nbInfo.lastQaReportTime.year,
	       nbInfo.lastQaReportTime.mon, nbInfo.lastQaReportTime.day,
	       nbInfo.lastQaReportTime.hour, nbInfo.lastQaReportTime.min,
	       nbInfo.lastQaReportTime.sec);
	printf("[ CERTIFY TIME ] %04d-%02d-%02d %02d:%02d:%02d\n", nbInfo.lastCertifyTime.year,
	       nbInfo.lastCertifyTime.mon, nbInfo.lastCertifyTime.day, nbInfo.lastCertifyTime.hour,
	       nbInfo.lastCertifyTime.min, nbInfo.lastCertifyTime.sec);
	printf("[ ACCESS TIME  ] %04d-%02d-%02d %02d:%02d:%02d\n", nbInfo.lastAccessTime.year,
	       nbInfo.lastAccessTime.mon, nbInfo.lastAccessTime.day, nbInfo.lastAccessTime.hour,
	       nbInfo.lastAccessTime.min, nbInfo.lastAccessTime.sec);
	for (int i = 0; i < NUM_OF_NBIOT_ERROR_LOG; i++) {
		char *log = QUEUE_get(nbInfo.errorLog, i);
		if (log == NULL) {
			break;
		}
		printf("[ LOG #%02d      ] 20%02X-%02X-%02X %02X:%02X:%02X ERR(%02X)\n", i,
		       *(log + 0), *(log + 1), *(log + 2), *(log + 3), *(log + 4), *(log + 5),
		       *(log + 6));
	}
	printf("=============================================================\n");
#endif
}

void NBIOT_release()
{
	nbCtx.status.psmOn = 0;
	nbCtx.lwm2m.value = 0;
	if (nbCtx.cellreg == NBIOT_CELLREG_NO_SERVICE) {
		nbCtx.cellreg = NBIOT_CELLREG_NOT_REG;
	}
}
