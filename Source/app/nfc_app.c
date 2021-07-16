#include "common.h"
#include "misc.h"

#include "alarm.h"
#include "app.h"
#include "meter.h"
#include "metering.h"
#include "modem.h"
#include "nfc_app.h"

#include "battery.h"
#include "flash.h"
#include "nfc.h"
#include "rtc.h"
#include "timer.h"
#include "uart.h"

#define TERMINAL_WORKING 0
#define TERMINAL_FINISH 1
#define TERMINAL_BUSY 2
#define TERMINAL_FAIL 3

// NFC 프로토콜 v2를 위한 시간 구조체.
typedef struct {
	union {
		uint32 dateTime;
		struct {
			uint32 year : 6, mon : 4, day : 5, hour : 5, min : 6, sec : 6;
		};
	};
} NfcDateV2_t;

// NFC 비동기 동작 시 사용되는 Command buffer
struct {
	int len;
	uint8 cmd[64];
} nfcAsyncRx;

void NFCAPP_clearAsyncCmd()
{
	nfcAsyncRx.len = 0;
	memset(nfcAsyncRx.cmd, 0, 64);
}

static int copyToAsyncCmd(uint8 *p, int len)
{
	if (len < 10 || len >= 64 ||
	    (nfcAsyncRx.len == len && memcmp(nfcAsyncRx.cmd, p, len) == 0)) {
		return 0;
	}

	memcpy(nfcAsyncRx.cmd, p, len);
	nfcAsyncRx.len = len;

	return 1;
}

uint8 getMyDeviceCode()
{
	return DEVICE_CODE_NB;
}

void send(byte *p, int len)
{
	NfcHeader_t *head = (NfcHeader_t *)p;

	head->deviceCode = getMyDeviceCode();
	memcpy(head->device_id, conf.imei, 8);
	head->len = len - MSG_OFFSET_LEN;

	NFC_sendMessage(p, len);

	// yikim 2018.01.30
	NFC_checkRead(100); // 데이터 전송 후 스마트폰에서 읽어가는 이벤트 체크.

	//다음 메세지 대기
	OSAL_startEventTimer(APP_getTaskId(), APP_EVENT_NFC_WAIT, (uint32)300);
	NFC_tagDetect();

	BATT_loadLastVoltage(); // 다음 보고시 사용할 목적으로 배터리 전압을 읽어 둠.
}

void continueSend(byte *p, int len)
{
	NfcHeader_t *head = (NfcHeader_t *)p;

	head->deviceCode = getMyDeviceCode();
	memcpy(head->device_id, conf.imei, 8);
	head->len = len - MSG_OFFSET_LEN;

	NFC_sendMessage(p, len);
}

void setRtcV1(uint8 *p)
{
	Date_t date;

	date.year = *(p + 0) + *(p + 1) * 0x100;
	date.mon = *(p + 2);
	date.day = *(p + 3);
	date.hour = *(p + 4);
	date.min = *(p + 5);
	date.sec = *(p + 6);

	// 실제 시간과 동기화여부가 명확하지 않으므로 'ALARM_writeTime' 사용 X
	RTC_set(&date);
}

void setRtcV2(uint8 *p)
{
	Date_t date;
	NfcDateV2_t tmp;

	memcpy((void *)&tmp.dateTime, p, sizeof(uint32));

	date.year = tmp.year + 2000;
	date.mon = tmp.mon;
	date.day = tmp.day;
	date.hour = tmp.hour;
	date.min = tmp.min;
	date.sec = tmp.sec;

	// 실제 시간과 동기화여부가 명확하지 않으므로 'ALARM_writeTime' 사용 X
	RTC_set(&date);
}

void readRtcV1(uint8 *p)
{
	Date_t date;
	RTC_read(&date);

	*(p + 0) = date.year % 0x100;
	*(p + 1) = date.year / 0x100;
	*(p + 2) = date.mon;
	*(p + 3) = date.day;
	*(p + 4) = date.hour;
	*(p + 5) = date.min;
	*(p + 6) = date.sec;
}

void readRtcV2(uint8 *p)
{
	Date_t date;
	NfcDateV2_t tmp;

	RTC_read(&date);

	tmp.year = date.year - 2000;
	tmp.mon = date.mon;
	tmp.day = date.day;
	tmp.hour = date.hour;
	tmp.min = date.min;
	tmp.sec = date.sec;

	memcpy(p, (void *)&tmp.dateTime, sizeof(uint32));
}

static int getModemAccessState()
{
	int state = MODEM_getState();
	if (state != DEVICE_STATUS_IDLE) {
		// return ACCESS_STATE_IN_SETTING;
		return ACCESS_STATE_IN_PROGRESS;
	} else {
		Date_t now;
		Date_t workTime = MODEM_getLastAccessTime();
		RTC_read(&now);
		int32 secDiff = MISC_calcSecDiff(&workTime, &now);
		if (secDiff < 0 || secDiff > 7) { // 모뎀 동작에 대해 Interval을 위해 추가.
			return ACCESS_STATE_IDLE;
		} else {
			return ACCESS_STATE_WAIT_FEW_SEC;
		}
	}
}

///////////////////////////////
/////  Send Message
///////////////////////////////

void sendNodeConfReportV1(uint8 reportType, int error)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcConfReportV1_t *p = (NfcConfReportV1_t *)msg;

	p->mtype = NB_CONF_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;
	p->reportType = reportType;
	if (p->reportType == REPORT_TYPE_SET) {
		if (error) {
			p->result = 1;
		} else {
			p->result = 0;
		}
		p->errorCode = error;
	}

	memcpy(p->serialNum, conf.serialNum, SERIAL_NUM_LEN);
	MODEM_getImsi(p->imsi);
	memcpy(p->serviceCode, conf.serviceCode, 4);

	MISC_ipToHexArray(conf.serverIp, p->serverIp);
	uchar temp = 0;
	temp = p->serverIp[0];
	p->serverIp[0] = p->serverIp[3];
	p->serverIp[3] = temp;

	temp = p->serverIp[2];
	p->serverIp[2] = p->serverIp[1];
	p->serverIp[1] = temp;

	memcpy(p->serverPort, &conf.serverPort, 2);
	memcpy(p->fwVer, APP_getFwVer(), FIRMWARE_VER_LEN);
	p->batt = BATT_getLastVoltage() & 0xff;
	readRtcV2(p->dateTime);
	p->lightMode = light.mode;
	p->timeBase = light.timeBase;
	p->reportInterval = conf.reportInterval;
	p->timeTable = light.region;

	int msgLen = sizeof(NfcConfReportV1_t);

	send(msg, msgLen);
}

void sendNodeConfReport(uint8 reportType, int error)
{
	sendNodeConfReportV1(reportType, error);
}

static void sendMeterReport(void)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcMeasureResp_t *p = (NfcMeasureResp_t *)msg;
	MeterUnitData_t *pUnitData = METER_getTempData();

	readRtcV1(p->year);
	memcpy(&p->volt, &pUnitData->volt, sizeof(pUnitData->volt));
	memcpy(&p->curr, &pUnitData->curr, sizeof(pUnitData->curr));
	memcpy(&p->shortCurr, &pUnitData->shortCurr, sizeof(pUnitData->shortCurr));
	memcpy(&p->power, &pUnitData->power, sizeof(pUnitData->power));
	memcpy(&p->cumPower, &pUnitData->cumPower, sizeof(pUnitData->cumPower));
	p->status = getRelayStatus();
	p->alarm = pUnitData->meterStatus;
	memcpy(p->onTime, light.onTime, 2);
	memcpy(p->offTime, light.offTime, 2);

	int msgLen = sizeof(NfcMeasureResp_t);

	send(msg, msgLen);
}

void sendFwVerReport()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcFwVerReport_t *p = (NfcFwVerReport_t *)msg;

	p->mtype = FW_VER_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;

	memcpy(p->fwVer, APP_getFwVer(), FIRMWARE_VER_LEN);

	send(msg, sizeof(NfcFwVerReport_t));
}

void sendServerConnectReport(int reqType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcServerConnReportV2_t *p = (NfcServerConnReportV2_t *)msg;
	p->mtype = SERVER_CONNECT_REPORT;
	p->mversion = NFC_PROTOCOL_VER_2;

	readRtcV1(p->currentYear);

	Date_t workTime = MODEM_getLastAccessTime();
	p->lastAccessYear[0] = workTime.year % 0x100;
	p->lastAccessYear[1] = workTime.year / 0x100;
	p->lastAccessMon = workTime.mon;
	p->lastAccessDay = workTime.day;
	p->lastAccessHour = workTime.hour;
	p->lastAccessMin = workTime.min;
	p->lastAccessSec = workTime.sec;

	int accessState = getModemAccessState();
	p->serverAccessState = accessState;
	if (accessState == ACCESS_STATE_IN_SETTING || accessState == ACCESS_STATE_IN_PROGRESS) {
		p->accessStepOrResult = MODEM_getState();
	} else {
		p->accessStepOrResult = MODEM_getResult();
	}

	p->rssi = MODEM_getAbsRssi();
	p->snr = MODEM_getSNR();
#if NBIOT_DEVICE
	p->rsrp = MODEM_getAbsRsrp();
	p->rsrq = MODEM_getAbsRsrq();
#endif

	send(msg, sizeof(NfcServerConnReportV2_t));

	if (reqType == 1) { // 접속 시작 명령
		if (accessState == ACCESS_STATE_IDLE) {
			// 모뎀이 보고 중이면 새로운 보고를 지시하지 않음
			OSAL_stopEventTimer(APP_getTaskId(), APP_EVENT_IMMEDIATE_REPORT);
			OSAL_startEventTimer(APP_getTaskId(), APP_EVENT_IMMEDIATE_REPORT,
					     (uint32)500);
			//            printf("server access started\n");
		} else {
			//            printf("server access requested ... but should wait few
			//            second\n");
		}
	} else {
		//        printf("server access NOT requested\n");
	}
}

static void sendBdControlAck(uint8 reset, uint8 powerCtrl)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcBdCtrlAck_t *p = (NfcBdCtrlAck_t *)msg;

	p->mtype = BD_CONTROL_ACK;
	p->mversion = NFC_PROTOCOL_VER_1;
	p->reset = reset;
	p->powerCtrl = powerCtrl;

	int messageLen = sizeof(NfcBdCtrlAck_t);

	send(msg, messageLen);
}

static void sendPsmResp(uint8 reportType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcPsmResp_t *p = (NfcPsmResp_t *)msg;

	p->mtype = PSM_MODE_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;
	p->reportType = reportType;

	readRtcV1(p->year);

	p->psm = light.psm;
	memcpy(p->psmStartTime, light.psmOnTime, 2);
	memcpy(p->psmStartDate, light.psmOnDay, 2);
	p->onOfs = light.onOfs;
	p->offOfs = light.offOfs;
	p->dimmMode = light.dimmingMode;
	p->dimmLv = light.dimmingLv;

	int messageLen = sizeof(NfcPsmResp_t);

	send(msg, messageLen);
}

static void sendGpsResp(uint8 reportType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcGpsResp_t *p = (NfcGpsResp_t *)msg;

	p->mtype = GPS_INFO_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;
	p->reportType = reportType;
	p->timeTable = light.region;
	memcpy(p->latitInt, conf.latitInt, 2);
	memcpy(p->latitDec, conf.latitDec, 2);
	memcpy(p->longitInt, conf.longitInt, 2);
	memcpy(p->longitDec, conf.longitDec, 2);

	int messageLen = sizeof(NfcGpsResp_t);

	send(msg, messageLen);
}

///////////////////////////////
/////  Recv Message
///////////////////////////////

void Recv_Node_Conf_Req(byte *data, uint8 len)
{
	sendNodeConfReport(REPORT_TYPE_REQ, 0);
}

void recvNodeConfSetV1(byte *data, uint8 len)
{
	NfcConfSetV1_t *p = (NfcConfSetV1_t *)data;

	setRtcV2(p->dateTime);

	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));

	light_t tlight;
	memcpy(&tlight, &light, sizeof(light_t));

	int error = 0;
	int changed = 0;
	do {
		// p->serialNum --> check하여 에러 처리할 필요가 있는가?
		if (memcmp(p->serialNum, tconfig.serialNum, SERIAL_NUM_LEN) != 0) {
			error = CONFIG_ERR_SERIAL_NUM;
			break;
		}

		uint8 ri = p->reportInterval;

		if ((24 % ri) != 0) {
			error = CONFIG_ERR_METERING_PARAM;
			break;
		}

		if (tconfig.reportInterval != ri) {
			tconfig.reportInterval = ri;
			changed = 1;
		}

		char newIP[SERVER_IP_STR_LEN];
		sprintf(newIP, "%d.%d.%d.%d", p->serverIp[3], p->serverIp[2], p->serverIp[1],
			p->serverIp[0]);

		if (strncmp(newIP, tconfig.serverIp, SERVER_IP_STR_LEN) != 0) {
			strcpy(tconfig.serverIp, newIP);
			changed = 1;
		}

		uint16 newPort;
		memcpy(&newPort, p->serverPort, 2);

		if (newPort != tconfig.serverPort) {
			tconfig.serverPort = newPort;
			changed = 1;
		}

		if (p->lightMode) {
			tlight.mode = p->lightMode;
			changed = 1;
		}

		if (p->timeBase) {
			tlight.timeBase = p->timeBase;
			changed = 1;
		}

		if (p->timeTable) {
			tlight.region = p->timeTable;
			changed = 1;
		}

		if (p->latitInt[0] || p->latitInt[1]) {
			memcpy(tconfig.latitInt, p->latitInt, sizeof(p->latitInt));
			changed = 1;
		}

		if (p->latitDec[0] || p->latitDec[1] || p->latitDec[2]) {
			memcpy(tconfig.latitDec, p->latitDec, sizeof(p->latitDec));
			changed = 1;
		}

		if (p->longitInt[0] || p->longitInt[1]) {
			memcpy(tconfig.longitInt, p->longitInt, sizeof(p->longitInt));
			changed = 1;
		}

		if (p->longitDec[0] || p->longitDec[1] || p->longitDec[2]) {
			memcpy(tconfig.longitDec, p->longitDec, sizeof(p->longitDec));
			changed = 1;
		}

		error = 0;
	} while (0);

	if (error == 0 && changed) {
		memcpy(&conf, &tconfig, sizeof(Config_t));
		memcpy(&light, &tlight, sizeof(light_t));
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
	}

	sendNodeConfReport(REPORT_TYPE_SET, error);
}

void recvNodeConfSet(byte *data, uint8 len)
{
	recvNodeConfSetV1(data, len);
}

void recvMeterReq(byte *data, uint8 len)
{
	NfcMeterReq_t *p = (NfcMeterReq_t *)data;

	setRtcV1(p->year);
	sendMeterReport();
}

void recvServerConnectReq(byte *data, uint8 len)
{
	NfcServerConnReq_t *p = (NfcServerConnReq_t *)data;
	sendServerConnectReport(p->reqType);
}

static void recvBdControlReq(byte *data, uint8 len)
{
	NfcBdCtrlReq_t *p = (NfcBdCtrlReq_t *)data;

	if (p->reset) {
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
	}

#define POWER_ON 1
#define POWER_OFF 2
	if (p->powerCtrl == POWER_ON) {
		Relay_Status(RELAY_ON, WORKER);
	} else if (p->powerCtrl == POWER_OFF) {
		Relay_Status(RELAY_OFF, WORKER);
	}

	sendBdControlAck(p->reset, p->powerCtrl);
}

///////////////////////////////
/////  Send Recv SMW Message
///////////////////////////////

void sendMeterBypassError()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcMeterBypassError_t *p = (NfcMeterBypassError_t *)msg;

	p->mtype = METER_BYPASS_ERROR;
	p->mversion = 0;

	memcpy(p->serialNum, conf.serialNum, SERIAL_NUM_LEN);
	memcpy(p->fwVer, APP_getFwVer(), FIRMWARE_VER_LEN);

	p->result = 1; // SN mismatch

	send(msg, sizeof(NfcMeterBypassError_t));
}

void NFCAPP_meterBypassReq()
{
	NfcMeterBypassReq_t *p = (NfcMeterBypassReq_t *)nfcAsyncRx.cmd;

	if (p->whatWithSN != IGNORE_SERIAL_NUM) {
		if (memcmp(conf.serialNum, p->serialNum, SERIAL_NUM_LEN) != 0) {
			if (p->whatWithSN == CHANGE_SERIAL_NUM) {
				memcpy(conf.serialNum, p->serialNum, SERIAL_NUM_LEN);
				FLASH_saveConfigInfo(&conf);
			} else {
				sendMeterBypassError();
				return;
			}
		}
	}

	APP_bypassMetering(p->body, p->bodyLen);
}

void NFCAPP_meterBypassResp(uint8 *body, int bodyLen)
{
	if (SMART_WATER_METER(conf.termModel) == 0) {
		// 스마트미터가 아니면 무시
		return;
	}

	OSAL_stopEventTimer(APP_getTaskId(), APP_EVENT_NFC_WAIT);
	OSAL_startEventTimer(APP_getTaskId(), APP_EVENT_NFC_WAIT, (uint32)700);

	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcMeterBypassResp_t *p = (NfcMeterBypassResp_t *)msg;

	p->mtype = METER_BYPASS_RESP;
	p->mversion = 0;

	memcpy(p->serialNum, conf.serialNum, SERIAL_NUM_LEN);
	memcpy(p->fwVer, APP_getFwVer(), FIRMWARE_VER_LEN);

	if (bodyLen >= 35) {
		bodyLen = 35;
	}

	p->bodyLen = bodyLen;
	memcpy(p->body, body, bodyLen);

	send(msg, sizeof(NfcMeterBypassResp_t) - 35 + bodyLen);
}

void recvMeterBypassRequest(byte *data, uint8 len)
{
	if (SMART_WATER_METER(conf.termModel) == 0) {
		// 스마트미터가 아니면 무시
		return;
	}

	if (copyToAsyncCmd(data, len) == 0) {
		return;
	}

	OSAL_setEvent(APP_getTaskId(), APP_EVENT_METER_BYPASS);
}

///////////////////////////////
/////  Send Recv NB-IoT for U+ Message
///////////////////////////////

static void sendNbiotIdReport(uint8 reportType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcNbiotIdReport_t *p = (NfcNbiotIdReport_t *)msg;

	p->mtype = NB_ID_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;
	p->reportType = reportType;

	memcpy(p->serviceCode, conf.serviceCode, 4);
	MODEM_getCseId(p->cseid);
	MODEM_getIccid(p->iccid);

	send(msg, sizeof(NfcNbiotIdReport_t));
}

static void recvNbiotIdSet(byte *data, uint8 len)
{
	NfcNbiotIdSet_t *p = (NfcNbiotIdSet_t *)data;

	BOOL isChange = FALSE;
	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));
	if (memcmp(tconfig.serviceCode, p->serviceCode, 4) != 0) {
		memcpy(tconfig.serviceCode, p->serviceCode, 4);
		if (MODEM_makeCseID(tconfig.serviceCode, NULL, NULL, NULL, 0)) {
			// 설정 변경 시 Global context에 복사 후 이벤트 등록
			isChange = TRUE;
		}
	}

	if (p->serverIp[3]) {
		char newIP[SERVER_IP_STR_LEN];
		sprintf(newIP, "%d.%d.%d.%d", p->serverIp[3], p->serverIp[2], p->serverIp[1],
			p->serverIp[0]);

		if (strncmp(newIP, tconfig.serverIp, SERVER_IP_STR_LEN) != 0) {
			strcpy(tconfig.serverIp, newIP);
			isChange = TRUE;
		}
	}

	if (p->serverPort[1] || p->serverPort[0]) {
		uint16 newPort;
		memcpy(&newPort, p->serverPort, 2);

		if (newPort != tconfig.serverPort) {
			tconfig.serverPort = newPort;
			isChange = TRUE;
		}
	}

	if (isChange) {
		memcpy(&conf, &tconfig, sizeof(Config_t));
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
	}

	sendNbiotIdReport(REPORT_TYPE_SET);
}

static void recvPsmModeSet(byte *data, uint8 len)
{
	NfcPsmModeSet_t *p = (NfcPsmModeSet_t *)data;

	if (p->psm) {
		light.psm = p->psm;
		memcpy(light.psmOnTime, p->psmStartTime, 2);
		memcpy(light.psmOffTime, p->psmEndTime, 2);
		memcpy(light.psmOnDay, p->psmStartDate, 2);
		memcpy(light.psmOffDay, p->psmEndDate, 2);
	}

	if (p->dimmMode) {
		light.dimmingMode = p->dimmMode;
	}

	if (p->dimmLv) {
		light.dimmingLv = p->dimmLv;
	}

	sendPsmResp(REPORT_TYPE_SET);
}

static void recvGpsSet(byte *data, uint8 len)
{
	NfcGpsSet_t *p = (NfcGpsSet_t *)data;

	if (p->timeTable) {
		light.region = p->timeTable;

		memcpy(conf.latitInt, p->latitInt, 2);
		memcpy(conf.latitDec, p->latitDec, 2);
		memcpy(conf.longitInt, p->longitInt, 2);
		memcpy(conf.longitDec, p->longitDec, 2);
	}

	sendGpsResp(REPORT_TYPE_SET);
}

///////////////////////////////
/////  NFC process and print
///////////////////////////////

// void printNfcMsg(int dir, byte *msg, uint8 len)
// {
// #if 1
//     printf("\n[%s] MSG_TYPE(%02X) (%02X)",
//         dir?"TX":"RX", *(msg + 10), *(msg + 10));

//     char *p = (char *)(msg+10);

//     printf("\n");

//     for(int i = 0; i < len-10; i++) {
//         if(!(i%0x16)) {
//             printf("\n    ");
//         }
//         printf("%02x ", *(p + i));
//     }
//     printf("\n");
// #endif
// }

void NFCAPP_runMessage(byte *data, uint8 len)
{
	switch (data[MSG_TYPE_POSITION]) {
	case NODE_CONF_REQ:
		Recv_Node_Conf_Req(data, len);
		break;

	case NB_CONF_SET:
		recvNodeConfSet(data, len);
		break;

	case SERVER_CONNECT_REQ:
		recvServerConnectReq(data, len);
		break;

	case BD_CONTROL_REQ:
		recvBdControlReq(data, len);
		break;

	case NB_ID_REQ:
		sendNbiotIdReport(REPORT_TYPE_REQ);
		break;

	case NB_ID_SET:
		recvNbiotIdSet(data, len);
		break;

	case PSM_MODE_REQ:
		sendPsmResp(REPORT_TYPE_REQ);
		break;

	case PSM_MODE_SET:
		recvPsmModeSet(data, len);
		break;

	case GPS_INFO_REQ:
		sendGpsResp(REPORT_TYPE_REQ);
		break;

	case GPS_INFO_SET:
		recvGpsSet(data, len);
		break;

	case MEASURE_REQ:
		recvMeterReq(data, len);
		break;

	case LORA_APP_EUI_KEY_REQ:
	case LORA_APP_EUI_KEY_SET:
		sendFwVerReport();
		break;
	case METER_BYPASS_REQ:
		recvMeterBypassRequest(data, len);
		break;
	}
}
