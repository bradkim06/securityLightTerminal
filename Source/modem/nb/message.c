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
#define NBMSG_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts("[NBMSG] ");                                                     \
			printf(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0);
#else
#define NBMSG_DBG(...)                                                                             \
	do {                                                                                       \
		if (TEST_isTestMode() == FALSE) {                                                  \
			printf_ts(__VA_ARGS__);                                                    \
		}                                                                                  \
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
		p->termSerial[i] = MISC_asciiToBCD(conf.serialNum[2 + i * 2 + 0],
						   conf.serialNum[2 + i * 2 + 1]);
	}

	// f/w version
	char fw_ver[0x10] = "";
	strcpy(fw_ver, APP_getFwVer());
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

static int insert_meteringTime(NbiotTimeStamp_t *p)
{
	Date_t now;
	RTC_read(&now);

	p->year = now.year - 2000;
	p->mon = now.mon;
	p->day = now.day;
	p->hour = now.hour;
	p->min = now.min;
	p->sec = now.sec;

	return sizeof(NbiotTimeStamp_t);
}

static int getErrorDataMessage(uchar *buf, int pfUlCnt, int pfDlCnt, int udpUlCnt, int udpDlCnt)
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

	int dataLen =
		sizeof(NbiotErrorReportToLg_t) + (logCnt * LEN_NBIOT_ERR_E_LOG) + 1; // +1 is EoF
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
	memcpy(&p->fwVer[1], APP_getFwVer(), p->fwVer[0]);

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

	p->alarm = pUnitData->meterStatus;
	p->lampStatus = getRelayStatus();
	memcpy(&p->volt, &pUnitData->volt, sizeof(pUnitData->volt));
	memcpy(&p->curr, &pUnitData->curr, sizeof(pUnitData->curr));
	memcpy(&p->shortCurr, &pUnitData->shortCurr, sizeof(pUnitData->shortCurr));
	memcpy(&p->power, &pUnitData->power, sizeof(pUnitData->power));
	memcpy(&p->cumPower, &pUnitData->cumPower, sizeof(pUnitData->cumPower));

	return sizeof(NbiotMeterData_t);
}

static int insert_params(NbiotMeterParams_t *p)
{
	p->rInterval = conf.reportInterval;
	p->mode = light.mode;
	p->timeBase = light.timeBase;
	p->timeTable = light.region;
	memcpy(&p->setOnTime, &light.setOnTime, sizeof(p->setOnTime));
	memcpy(&p->setOffTime, &light.setOffTime, sizeof(p->setOffTime));
	p->onOfs = light.onOfs;
	p->offOfs = light.offOfs;
	p->psmMode = light.psm;
	memcpy(&p->psmOnTime, &light.psmOnTime, sizeof(p->psmOnTime));
	memcpy(&p->psmOffTime, &light.psmOffTime, sizeof(p->psmOffTime));
	memcpy(&p->psmOnDay, &light.psmOnDay, sizeof(p->psmOnDay));
	memcpy(&p->psmOffDay, &light.psmOffDay, sizeof(p->psmOffDay));
	p->dimmingMode = light.dimmingMode;
	p->dimmingLv = light.dimmingLv;

	return sizeof(NbiotMeterParams_t); // 21
}

static int getCurrentDataMessage(uchar *buf)
{
	NbiotDataReport_t *p = (NbiotDataReport_t *)buf;

	p->protocol = PROTOCOL_VERSION;

	int len = 0;
	p->msgType = NBIOT_DATA_REPORT;
	p->reportType = PERIOD_MSG;
	p->lightReqType = AUTO_TYPE;
	len += 3;

	len += insert_id(&p->id);
	len += insert_radioQuality(&p->radio);
	len += insert_termInfo(&p->term);
	len += insert_meteringTime(&p->meterTime);
	len += insert_oneData(&p->data);
	len += insert_params(&p->params);

	p->len = len;
	p->checksum = cal_checksum(&p->msgType, len); // 2 - protocol, len

	return len + 3; // 3 = protocol + len + checksum
}

static BOOL checkValidAck(uchar *buf, int len)
{
	NbiotAck_t *p = (NbiotAck_t *)buf;
	uchar checksum = cal_checksum(&p->mtype, len - 3);

	if (p->checksum != checksum) {
		printf_ts("checksum not match calc(0x%02X), recv(0x%02X)\n", checksum, p->checksum);
		return FALSE;
	}

#define ACK_PROTOCOL 0xC1
	if (p->protocol != ACK_PROTOCOL) {
		printf_ts("protocol version not match need(0x%02X) recv(0x%02X)\n", ACK_PROTOCOL,
			  p->protocol);
		return FALSE;
	}

#define ACK_MTYPE 0x50
	if (p->mtype != ACK_MTYPE) {
		printf_ts("message type not match need(0x%02X) recv(0x%02X)\n", ACK_MTYPE,
			  p->mtype);
		return FALSE;
	}

	return TRUE;
}

int NBMSG_recvAckMsg(uchar *buf, int len)
{
	if (checkValidAck(buf, len)) {
		NbiotAck_t *p = (NbiotAck_t *)buf;

		return TRUE;
	}

	return FALSE;
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
	int len =
		sizeof(NbiotErrorReportToLg_t) + (NUM_OF_NBIOT_ERROR_LOG * LEN_NBIOT_ERROR_LOG) + 1;
	uchar *raw = (uchar *)malloc(len);
	len = getErrorDataMessage(raw, nbInfo.pfUlCnt, nbInfo.pfDlCnt, nbInfo.udpUlCnt,
				  nbInfo.udpDlCnt);

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
