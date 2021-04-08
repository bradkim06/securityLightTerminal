#include "message.h"
#include "modem.h"
#include "nbiot.h"

#include "alarm.h"
#include "app.h"
#include "battery.h"
#include "common.h"
#include "flash.h"
#include "meter.h"
#include "misc.h"
#include "rtc.h"
#include "test.h"
#include "uart.h"

#if defined(DEBUG)
#define NBMSG_DBG(...)                                                         \
	do {                                                                   \
		if (TEST_isTestMode() == FALSE) {                              \
			printf_ts("[NBMSG] ");                                 \
			printf(__VA_ARGS__);                                   \
		}                                                              \
	} while (0);
#else
#define NBMSG_DBG(...)                                                         \
	do {                                                                   \
		if (TEST_isTestMode() == FALSE) {                              \
			printf_ts(__VA_ARGS__);                                \
		}                                                              \
	} while (0);
#endif

static uchar cal_checksum(uchar *p, int len)
{
	uchar checksum = 0;

	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

static int insert_id(MobileId_t *p)
{
	MODEM_getImei(p->imei);
	MODEM_getImsi(p->imsi);

	return sizeof(MobileId_t);
}

static int insert_termInfo(NbiotDeviceInfo_t *p)
{
	for (int i = 0; i < SERIAL_NUM_LEN / 2; i++) {
		p->termSerial[i] =
			MISC_asciiToBCD(conf.serialNum[2 + i * 2 + 0],
					conf.serialNum[2 + i * 2 + 1]);
	}

	// f/w version
	char fw_ver[0x10] = "";
	strcpy(fw_ver, APP_getFwVer(conf.isUsingPF));
	fw_ver[0] = '0';

	p->fwVer[0] = MISC_asciiToBCD(fw_ver[0], fw_ver[1]);
	p->fwVer[1] = MISC_asciiToBCD(fw_ver[2], fw_ver[3]);

	p->termBatt = BATT_getVoltage();

	return sizeof(NbiotDeviceInfo_t);
}

static int insert_radioQuality(NbiotRadioQuality_t *p)
{
	int rssi = MODEM_getAbsRssi();
	int rsrp = MODEM_getAbsRsrp();
	int rsrq = MODEM_getAbsRsrq();
	int snr = MODEM_getSNR();
	int cid = MODEM_getCid();

	p->rssi = rssi;
	p->ber = 0; // BER - 사용하지 않음.
	memcpy(p->cid, &cid, 2);
	memcpy(p->rsrp, &rsrp, 2);
	memcpy(p->rsrq, &rsrq, 2);
	memcpy(p->snr, &snr, 2);

	return sizeof(NbiotRadioQuality_t);
}

static int insert_meteringParam(NbiotMeteringParam_t *p)
{
	p->ri = conf.reportInterval;

	return sizeof(NbiotMeteringParam_t);
}

static int insert_meteringTime(NbiotTimeStamp_t *p, BOOL isJoinMsg)
{
	MeterUnitData_t *pUnit;
	if (isJoinMsg) {
		pUnit = METER_getTempData();
	} else {
		pUnit = METER_getStoredData(0);
	}

	if (pUnit->isTimeSync) {
		p->year = pUnit->year;
		p->mon = pUnit->mon;
		p->day = pUnit->day;
		p->hour = pUnit->hour;
		p->min = pUnit->min;
		p->sec = pUnit->sec;
	} else {
		// 검침 데이터의 시간이 동기화된 것이 아닐 경우 전송시간에 대한 차이값으로
		// 설정.
		if (isJoinMsg) {
			// Join message 의경우 검침 직후 보고하므로 차이값을 0로 설정.
			p->year = 0xFF;
			p->mon = 0xFF;
			p->day = 0xFF;
			p->hour = 0;
			p->min = 0;
			p->sec = 0;
		} else {
			// Join message가 아닐 경우 전송시간에 대한 차이값을 계산하여 설정.
			p->year = 0xFF;
			p->mon = 0xFF;
			p->day = 0xFF;

			Date_t now;
			RTC_read(&now);
			uint8 diff = conf.reportInterval % conf.meterInterval;
			// 1. 주기보고의 경우 Base time 및 interval로 계산된 시간을 설정한다.
			// 1.1. 이때 분단위 주기 동작 여부에 따라 다르게 설정한다.
			// 2. 시간단위 주기동작 에서 검침시간 대비 보고시간은 report min만큼
			// 이후에 보고한다. 2.1. 위의 이유로 분단위는 report min 값을 설정한다.
			if (conf.isShortInterval) {
				p->hour = 0;
				p->min = 0;
			} else {
				p->hour = diff;
				p->min = conf.reportMin;
			}
			p->sec = 0;
		}
	}

	return sizeof(NbiotTimeStamp_t);
}

static int getErrorDataMessage(uchar *buf, int pfUlCnt, int pfDlCnt,
			       int udpUlCnt, int udpDlCnt)
{
	uint16 temp16 = 0;
	uint32 temp32 = 0;

	NbiotErrorReportToLg_t *p = (NbiotErrorReportToLg_t *)buf;
	memset(buf, 0, sizeof(NbiotErrorReportToLg_t));

	// Set message version
	p->msgVer = NBIOT_ERR_MSG_VER;

	// Set CTN
	MODEM_getCtn(p->ctn);

	Date_t date;
	RTC_read(&date);
	temp32 = date.year - 2000;
	MISC_intToBCD(&temp32, &p->sendTime[0], 1);
	temp32 = date.mon;
	MISC_intToBCD(&temp32, &p->sendTime[1], 1);
	temp32 = date.day;
	MISC_intToBCD(&temp32, &p->sendTime[2], 1);
	temp32 = date.hour;
	MISC_intToBCD(&temp32, &p->sendTime[3], 1);
	temp32 = date.min;
	MISC_intToBCD(&temp32, &p->sendTime[4], 1);
	temp32 = date.sec;
	MISC_intToBCD(&temp32, &p->sendTime[5], 1);

	p->ueType = NBIOT_ERR_CURRENT_UE_TYPE;

	p->periodLength = LEN_NBIOT_ERR_P_TIME + LEN_NBIOT_ERR_P_PAYLOAD;
	temp16 = SWAP16(conf.reportInterval * 60);
	memcpy(p->periodTime, &temp16, LEN_NBIOT_ERR_P_TIME);
	temp16 = SWAP16((uint16)LEN_MAX_NBIOT_DATA);
	memcpy(p->periodPayload, &temp16, LEN_NBIOT_ERR_P_PAYLOAD);

	temp16 = SWAP16(pfUlCnt);
	memcpy(p->pfUlCnt, &temp16, LEN_NBIOT_ERR_UL_DL_CNT);
	temp16 = SWAP16(pfDlCnt);
	memcpy(p->pfDlCnt, &temp16, LEN_NBIOT_ERR_UL_DL_CNT);
	temp16 = SWAP16(udpUlCnt);
	memcpy(p->udpUlCnt, &temp16, LEN_NBIOT_ERR_UL_DL_CNT);
	temp16 = SWAP16(udpDlCnt);
	memcpy(p->udpDlCnt, &temp16, LEN_NBIOT_ERR_UL_DL_CNT);

	p->npFlag = 0;

	int logCnt = 0;
	for (logCnt = 0; logCnt < CNT_NBIOT_ERR_E_LOG; logCnt++) {
		uchar *log = MODEM_getErrorLog(logCnt);
		if (log == NULL) {
			break;
		}
	}

	int dataLen = sizeof(NbiotErrorReportToLg_t) +
		      (logCnt * LEN_NBIOT_ERR_E_LOG) + 1; // +1 is EoF
	uchar *pLog = buf + sizeof(NbiotErrorReportToLg_t);
	if (logCnt > 0) {
		temp16 = SWAP16(logCnt * LEN_NBIOT_ERR_E_LOG);
		memcpy(p->errLength, &temp16, LEN_NBIOT_ERR_E_LENGTH);

		for (int i = 0; i < logCnt; i++) {
			memcpy(pLog, MODEM_getErrorLog(i), LEN_NBIOT_ERR_E_LOG);
			pLog += LEN_NBIOT_ERR_E_LOG;
		}
	}
	*pLog = 0xFF; // set EoF

	return dataLen;
}

static int getQaDataMessage(uchar *buf)
{
	NbiotQaReportToLg_t *p = (NbiotQaReportToLg_t *)buf;
	memset(buf, 0, sizeof(NbiotQaReportToLg_t));

	// Set message version
	p->msgVer = NBIOT_QA_MSG_VER;

	// Set CTN
	MODEM_getCtn(p->ctn);

	// Set battery voltage (2byte BCD)
	p->batt[0] = NBIOT_QA_BATT_VOLTAGE;
	p->batt[1] = (BATT_getVoltage() / 10);
	p->batt[2] = ((BATT_getVoltage() % 10) << 4);

	// Set serving CID
	uint32 cgi = SWAP32(MODEM_getCgi());
	memcpy(p->cgi, &cgi, LEN_NBIOT_QA_CGI);

	// Set RSRP (2byte BCD)
	uint32 rsrp = MODEM_getAbsRsrp();
	MISC_intToBCD(&rsrp, p->rsrp, LEN_NBIOT_QA_RSRP);

	// Set SNR (1byte 음수/양수 설정, 1byte BCD)
	int snr = MODEM_getSNR();
	uint32 absSnr = abs(snr);
	p->sinr[0] = (snr > 0) ? 0 : 1;
	MISC_intToBCD(&absSnr, &p->sinr[1], LEN_NBIOT_QA_SINR - 1);

	// Set model name
	char *model = TERM_MODEL_STRING(conf.termModel);
	p->model[0] = TERM_MODEL_STRING_LEN; // sizeof(model);
	memcpy(&p->model[1], model, p->model[0]);

	// Set firmware version
	p->fwVer[0] = FIRMWARE_VER_LEN;
	memcpy(&p->fwVer[1], APP_getFwVer(conf.isUsingPF), p->fwVer[0]);

	// Set TX power (1byte 음수/양수 설정, 1byte BCD)
	int txPower = MODEM_getTxPower();
	uint32 absTxPower = abs(txPower);
	p->txPower[0] = (txPower > 0) ? 0 : 1;
	MISC_intToBCD(&absTxPower, &p->txPower[1], LEN_NBIOT_QA_TX_POWER - 1);

	return sizeof(NbiotQaReportToLg_t);
}

static int insert_oneData(NbiotMeterData_t *p)
{
	MeterUnitData_t *pUnitData = METER_getTempData();

	/* p->status = pUnitData->meterStatus; */
	/* p->volt = pUnitData->volt; */
	/* p->curr = pUnitData->curr; */
	/* p->shortCurr = pUnitData->shortCurr; */
	/* p->power = pUnitData->power; */
	/* p->cumPower = pUnitData->cumPower; */
	pUnitData->volt = 0x12345678;
	pUnitData->cumPower = 0x87654321;

	p->status = 0xff;
	memcpy(&p->volt, &pUnitData->volt, sizeof(pUnitData->volt));
	memcpy(&p->curr, 0, sizeof(pUnitData->volt));
	memcpy(&p->shortCurr, 0, sizeof(pUnitData->volt));
	memcpy(&p->power, 0, sizeof(pUnitData->volt));
	memcpy(&p->cumPower, &pUnitData->cumPower, sizeof(pUnitData->cumPower));

	return sizeof(NbiotMeterData_t);
	// 21 = status(1), volt(4), curr(4), shortCurr(4), power(4), cumPower(4)
}

static int getCurrentDataMessage(uchar *buf)
{
	NbiotDataReport_t *p = (NbiotDataReport_t *)buf;

	p->protocol = PROTOCOL_VERSION;

	int len = 0;
	p->mtype = NBIOT_DATA_REPORT;
	len++;

	len += insert_id(&p->id);
	len += insert_radioQuality(&p->radio);
	len += insert_termInfo(&p->term);
	len += insert_meteringParam(&p->mp);
	// Join message는 아니지만 즉시검침/보고를 위한 message이므로
	// Join message와 동일한 방식의 metering time을 얻는다.
	len += insert_meteringTime(&p->meterTime, TRUE);
	len += insert_oneData(&p->data);

	p->len = len;
	p->checksum = cal_checksum(&p->mtype, len); // 2 - protocol, len

	return len + 3; // 3 = protocol + len + checksum
}

int NBMSG_recvAck(char *p, int len)
{
	int valid = 0;

	NbiotAck_t *pAck = (NbiotAck_t *)p;
	if (pAck->mtype == NBIOT_ACK) {
		uchar rChecksum = *(p + pAck->len + 2);
		uchar cChecksum = cal_checksum(&pAck->mtype, pAck->len);

		if (rChecksum != cChecksum) {
			NBMSG_DBG("checksum error (read : %x, calc : %x) \n",
				  rChecksum, cChecksum);
			return 0;
		}

		Date_t date;

		date.year = pAck->currTime.year + 2000;
		date.mon = pAck->currTime.mon;
		date.day = pAck->currTime.day;
		date.hour = pAck->currTime.hour;
		date.min = pAck->currTime.min;
		date.sec = pAck->currTime.sec;

		ALARM_writeTime(&date);

		int needReset = 0;
		int mpChanged = 0;
		uchar mi = 0;
		uchar ri = 0;

		if (pAck->len >= LEN_NBIOT_ACK_DATA) {
			if (pAck->resetCommand) {
				needReset = 1;
			}

			mi = pAck->mi;
			ri = pAck->ri;

			do {
				// 분단위 동작의 경우 주기변경 요청을 적용하지 않는다. (NB-IoT only)
				if (conf.isShortInterval) {
					break;
				}

				if (mi == conf.meterInterval &&
				    ri == conf.reportInterval) {
					// 기존 것과 같으면 무시
					break;
				}

				if (ri == 0 || mi == 0 || ri < mi) {
					// 둘 중 하나가 0이거나, 보고주기보다 검침주기보가 크면 무시
					break;
				}

				if ((24 % mi) || (24 % ri)) {
					// 검침주기나 보고주기가 24의 약수가 아니면 무시
					break;
				}

				conf.meterInterval = mi;
				conf.reportInterval = ri;

				FLASH_saveConfigInfo(&conf);
				FLASH_readConfigInfo(&conf);
				mpChanged = 1;

			} while (0);
		}

		// Convert IMEI data to string
		char imeiStr[19] = "";
		sprintf(imeiStr, "%02X-%02X%02X%02X-%02X%02X%02X-%X",
			pAck->id.imei[0], pAck->id.imei[1], pAck->id.imei[2],
			pAck->id.imei[3], pAck->id.imei[4], pAck->id.imei[5],
			pAck->id.imei[6], pAck->id.imei[7] / 0x10);

		// Convert IMSI data to string
		uint8 temp[16];
		for (int i = 0; i < 8; i++) {
			temp[i * 2 + 0] = (pAck->id.imsi[i] / 0x10) + '0';
			temp[i * 2 + 1] = (pAck->id.imsi[i] % 0x10) + '0';
		}

		char imsiStr[18] = "";
		sprintf(imsiStr, "%c%c%c-%c%c-%c%c%c%c%c%c%c%c%c%c", temp[0],
			temp[1], temp[2], temp[3], temp[4], temp[5], temp[6],
			temp[7], temp[8], temp[9], temp[10], temp[11], temp[12],
			temp[13], temp[14]);

		NBMSG_DBG(
			"IMEI(%s) IMSI(%s) RTC(%04d-%02d-%02d %02d:%02d:%02d)",
			imeiStr, imsiStr, date.year, date.mon, date.day,
			date.hour, date.min, date.sec);

		if (mpChanged) {
			printf(" RI(%d) MI(%d)", ri, mi);
		}

		if (needReset) {
			printf(" %s",
			       needReset ? "-- will be reset after 5 sec" : "");
		}

		printf("\n");

		if (needReset) {
			OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
		}

		valid = 1;
	}

	return valid;
}

int NBMSG_recvReqChgServer(char *p, int len, char *ip, uint16 *port)
{
	int valid = 0;

	NbiotCmdChgServer_t *pCmd = (NbiotCmdChgServer_t *)p;
	if (pCmd->mtype == NBIOT_CMD_CHG_SERVER) {
		uchar rChecksum = *(p + pCmd->len + 2);
		uchar cChecksum = cal_checksum(&pCmd->mtype, pCmd->len);

		if (rChecksum != cChecksum) {
			NBMSG_DBG("checksum error (read : %x, calc : %x) \n",
				  rChecksum, cChecksum);
			return 0;
		}

		if (pCmd->len >= LEN_NBIOT_CMD_CHG_SERVER_DATA) {
			Date_t date;

			date.year = pCmd->currTime.year + 2000;
			date.mon = pCmd->currTime.mon;
			date.day = pCmd->currTime.day;
			date.hour = pCmd->currTime.hour;
			date.min = pCmd->currTime.min;
			date.sec = pCmd->currTime.sec;

			ALARM_writeTime(&date);

			// Convert IMEI data to string
			char imeiStr[19] = "";
			sprintf(imeiStr, "%02X-%02X%02X%02X-%02X%02X%02X-%X",
				pCmd->id.imei[0], pCmd->id.imei[1],
				pCmd->id.imei[2], pCmd->id.imei[3],
				pCmd->id.imei[4], pCmd->id.imei[5],
				pCmd->id.imei[6], pCmd->id.imei[7] / 0x10);

			// Convert IMSI data to string
			uint8 temp[16];
			for (int i = 0; i < 8; i++) {
				temp[i * 2 + 0] =
					(pCmd->id.imsi[i] / 0x10) + '0';
				temp[i * 2 + 1] =
					(pCmd->id.imsi[i] % 0x10) + '0';
			}

			char imsiStr[18] = "";
			sprintf(imsiStr, "%c%c%c-%c%c-%c%c%c%c%c%c%c%c%c%c",
				temp[0], temp[1], temp[2], temp[3], temp[4],
				temp[5], temp[6], temp[7], temp[8], temp[9],
				temp[10], temp[11], temp[12], temp[13],
				temp[14]);

			NBMSG_DBG(
				"IMEI(%s) IMSI(%s) RTC(%04d-%02d-%02d %02d:%02d:%02d)",
				imeiStr, imsiStr, date.year, date.mon, date.day,
				date.hour, date.min, date.sec);

			// Copy requested IP & Port
			sprintf(ip, "%d.%d.%d.%d", pCmd->ip[3], pCmd->ip[2],
				pCmd->ip[1], pCmd->ip[0]);
			memcpy(port, pCmd->port, 2);

			if (strncmp(ip, conf.serverIp, SERVER_IP_STR_LEN) !=
				    0 ||
			    *port != conf.serverPort) {
				printf(" change server(%s:%d)\n", ip, *port);
				valid = 1;
			} else {
				printf(" same server(%s:%d)\n", ip, *port);
				valid = 0;
			}
		}
	}

	return valid;
}

int NBMSG_recvReqChgInterval(uchar *buf, int len)
{
	if (len <= 0)
		return 0;

	uchar *data = NULL;
	if (*buf != PROTOCOL_VERSION) {
		int rawDataLen = len;
		data = (uchar *)malloc(rawDataLen);
		for (int i = 0; i < rawDataLen; i++) {
			*(data + i) = MISC_asciiToBCD(*(buf + i * 2),
						      *(buf + 1 + i * 2));
		}
	} else {
		data = buf;
	}

	NbiotAckResp_t *pIntervalReq = (NbiotAckResp_t *)data;
	if (pIntervalReq->mtype == NBIOT_ACK_RESP) {
		/* uchar rChecksum = *(data + pIntervalReq->len + 2); */
		uchar rChecksum = pIntervalReq->checksum;
		uchar cChecksum =
			cal_checksum(&pIntervalReq->mtype, pIntervalReq->len);

		if (rChecksum != cChecksum) {
			NBMSG_DBG("checksum error (read : %x, calc : %x) \n",
				  rChecksum, cChecksum);
			return 0;
		}

		uchar ri = pIntervalReq->ri;

		// 둘 중 하나가 0이거나, 보고주기보다 검침주기보가 크면 무시

		// 기존 것과 같으면 무시
		if (ri == conf.reportInterval) {
			NBMSG_DBG("Report interval is same (%d)\n", ri);
			return TRUE;
		} else if (ri == 0) {
			NBMSG_DBG("Report interval is '0' not changed (%d)\n",
				  ri);
			return TRUE;
		} else if (24 % ri) {
			// 검침주기나 보고주기가 24의 약수가 아니면 무시
			NBMSG_DBG("Report interval can not over 24h (%d)\n",
				  ri);
			return 0;
		}

		conf.reportInterval = ri;

		FLASH_saveConfigInfo(&conf);
		FLASH_readConfigInfo(&conf);

		NBMSG_DBG("report interval(%d) -- changed\n", ri);

		if (pIntervalReq->isReset) {
			OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
		}

		return 1;
	}

	return 0;
}

int NBMSG_recvReqReset(uchar *buf, int len)
{
	NbiotResetReq_t *pResetReq = (NbiotResetReq_t *)buf;
	if (strstr(pResetReq->msg, "DevReset")) {
		NBMSG_DBG("reset request -- will be reset after detach N/W.\n");

		OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);

		return 1;
	}

	return 0;
}

int NBMSG_recvSimpleAck(uchar *buf, int len)
{
	NbiotSimpleAck_t *pAck = (NbiotSimpleAck_t *)buf;
	if (strstr(pAck->msg, "ACK")) {
		return 1;
	}

	return 0;
}

char *NBMSG_getQaReport(int *messageLen)
{
	uchar *raw = (uchar *)malloc(sizeof(NbiotQaReportToLg_t) + 1);
	int len = getQaDataMessage(raw);

	// Hex data를 ASCII로 변환하여 저장할 버퍼의 길이이므로
	// 본래 데이터 길이의 x2에 '\0'를 위한 +1로 정의.
	int dataLen = (len * 2) + 1;
	char *data = (char *)malloc(dataLen);
	memset(data, 0, dataLen);

	for (int i = 0; i < len; i++) {
		snprintf(data, dataLen, "%s%02X", data, *(raw + i));
	}
	free(raw);

	*messageLen = len;
	return data;
}

char *NBMSG_getErrorReport(int *messageLen)
{
	// Uplink/Downlink information + error log data
	int len = sizeof(NbiotErrorReportToLg_t) +
		  (NUM_OF_NBIOT_ERROR_LOG * LEN_NBIOT_ERROR_LOG) + 1;
	uchar *raw = (uchar *)malloc(len);
	len = getErrorDataMessage(raw, nbInfo.pfUlCnt, nbInfo.pfDlCnt,
				  nbInfo.udpUlCnt, nbInfo.udpDlCnt);

	// Hex data를 ASCII로 변환하여 저장할 버퍼의 길이이므로
	// 본래 데이터 길이의 x2에 '\0'를 위한 +1로 정의.
	int dataLen = (len * 2) + 1;
	char *data = (char *)malloc(dataLen);
	memset(data, 0, dataLen);

	for (int i = 0; i < len; i++) {
		snprintf(data, dataLen, "%s%02X", data, *(raw + i));
	}
	free(raw);

	*messageLen = len;
	return data;
}

char *NBMSG_getMeterReport(BOOL isJoin, int *messageLen)
{
	uchar *raw = (uchar *)malloc(LEN_MAX_NBIOT_DATA);
	int len;
	len = getCurrentDataMessage(raw);

	// Hex data를 ASCII로 변환하여 저장할 버퍼의 길이이므로
	// 본래 데이터 길이의 x2에 '\0'를 위한 +1로 정의.
	int dataLen = (len * 2) + 1;
	char *data = (char *)malloc(dataLen);
	memset(data, 0, dataLen);

	for (int i = 0; i < len; i++) {
		snprintf(data, dataLen, "%s%02X", data, *(raw + i));
	}
	free(raw);

	*messageLen = len;
	return data;
}
