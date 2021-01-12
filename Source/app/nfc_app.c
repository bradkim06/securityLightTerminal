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

#if REPEATER_DEVICE
#include "slave_access.h"
#endif

#define TERMINAL_WORKING 0
#define TERMINAL_FINISH 1
#define TERMINAL_BUSY 2
#define TERMINAL_FAIL 3

// NFC 프로토콜 v2를 위한 시간 구조체.
typedef struct {
	union {
		uint32 dateTime;
		struct {
			uint32 year : 6, mon : 4, day : 5, hour : 5, min : 6,
				sec : 6;
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
#if LORA_DEVICE
	return DEVICE_CODE_LORA;
#else
	return DEVICE_CODE_NB;
#endif
}

void send(byte *p, int len)
{
	NfcHeader_t *head = (NfcHeader_t *)p;

	head->deviceCode = getMyDeviceCode();
#if LORA_DEVICE
	memcpy(head->device_id, conf.devEui, 8);
#else
	memcpy(head->device_id, conf.imei, 8);
#endif
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
#if LORA_DEVICE
	memcpy(head->device_id, conf.devEui, 8);
#else
	memcpy(head->device_id, conf.imei, 8);
#endif
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
		if (secDiff < 0 ||
		    secDiff > 7) { // 모뎀 동작에 대해 Interval을 위해 추가.
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

#if LORA_DEVICE
	p->mtype = LORA_CONF_REPORT;
#else
	p->mtype = NB_CONF_REPORT;
#endif
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

	if (conf.sleepMode) {
		p->sleepMode = SLEEP_MODE_SLEEP;
	} else {
		p->sleepMode = SLEEP_MODE_NORMAL;
	}
#if LORA_DEVICE
	MODEM_getAppEui(p->appEui);
#else
	MODEM_getImsi(p->imsi);
	MISC_ipToHexArray(conf.serverIp, p->serverIp);

	uchar temp = 0;
	temp = p->serverIp[0];
	p->serverIp[0] = p->serverIp[3];
	p->serverIp[3] = temp;

	temp = p->serverIp[2];
	p->serverIp[2] = p->serverIp[1];
	p->serverIp[1] = temp;

	memcpy(p->serverPort, &conf.serverPort, 2);
#endif
	p->batt = BATT_getLastVoltage() & 0xff;
	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);
	p->meterInterval = conf.meterInterval;
	p->reportInterval = conf.reportInterval;

	readRtcV1(p->year);

#if 1
	p->nMeter = 1;
	p->meterInfo[0].meterPort = 1;
	p->meterInfo[0].meterType = conf.meterType;
#else
	p->nMeter = conf.meterNum;
	for (int i = 0; i < conf.meterNum; i++) {
		p->meterInfo[i].meterPort = conf.meterInfo[i].meterPort;
		p->meterInfo[i].meterType = conf.meterInfo[i].meterType;
	}
#endif

	// meter가 3개일 때를 기준으로 message format이 정의되어 있으므로 그 수가
	// 모자라면 그만큼 줄임
	int msgLen = sizeof(NfcConfReportV1_t) - (3 - p->nMeter) * 2;

	send(msg, msgLen);
}

#if REPEATER_DEVICE

void sendNodeConfReportV2(uint8 reportType, int error)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcConfReportV2_t *p = (NfcConfReportV2_t *)msg;

#if LORA_DEVICE
	p->mtype = LORA_CONF_REPORT;
#else
	p->mtype = NB_CONF_REPORT;
#endif
	p->mversion = NFC_PROTOCOL_VER_2;
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

	if (conf.sleepMode) {
		p->sleepMode = SLEEP_MODE_SLEEP;
	} else {
		p->sleepMode = SLEEP_MODE_NORMAL;
	}
#if LORA_DEVICE
	MODEM_getAppEui(p->appEui);
#else
	MODEM_getImsi(p->imsi);
	MISC_ipToHexArray(conf.serverIp, p->serverIp);

	uchar temp = 0;
	temp = p->serverIp[0];
	p->serverIp[0] = p->serverIp[3];
	p->serverIp[3] = temp;

	temp = p->serverIp[2];
	p->serverIp[2] = p->serverIp[1];
	p->serverIp[1] = temp;

	memcpy(p->serverPort, &conf.serverPort, 2);
#endif
	p->batt = BATT_getLastVoltage() & 0xff;
	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);
	p->meterInterval = conf.meterInterval;
	p->reportInterval = conf.reportInterval;

	readRtcV2(p->dateTime);

	memcpy(p->amiInfo.panId, &conf.pan_id, sizeof(uint16));
	memcpy(p->amiInfo.nwkAddr, &conf.nwk_addr, sizeof(uint32));

	int slaveIdPos = getFirstZeroPosition(conf.nwk_addr);
	uint8 *slaveNwk = (uint8 *)&conf.slaveNwk;
	p->amiInfo.slaveId = *(slaveNwk + slaveIdPos);

	send(msg, sizeof(NfcConfReportV2_t));
}

#endif // #if REPEATER_DEVICE

void sendNodeConfReport(uint8 reportType, int error)
{
#if REPEATER_DEVICE
	sendNodeConfReportV2(reportType, error);
#else
	sendNodeConfReportV1(reportType, error);
#endif
}

void sendMeterReport()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcMeterReport_t *p = (NfcMeterReport_t *)msg;

	p->mtype = METER_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;

	p->batt = BATT_getLastVoltage() & 0xff;

	MeterUnitData_t *tmpUnit = METER_getTempData();
	int year = tmpUnit->year + 2000;

	p->year[0] = year % 0x100;
	p->year[1] = year / 0x100;
	p->mon = tmpUnit->mon;
	p->day = tmpUnit->day;
	p->hour = tmpUnit->hour;
	p->min = tmpUnit->min;
	p->sec = tmpUnit->sec;

	int needNewMetering = 0;

#if REPEATER_DEVICE
	SlaveAccessState_t state = SLAVE_getAccessState();
	if (state == SLAVE_ACCESS_IDLE) {
		if (SLAVE_needToUpdateMeterInfo()) {
			// Only check modem working. (not wait delay for MODEM)
			if (getModemAccessState() == ACCESS_STATE_IN_PROGRESS) {
				p->meterState = TERMINAL_BUSY;
			} else {
				needNewMetering = 1;
				p->meterState = TERMINAL_WORKING;
			}
		} else {
			p->meterState = TERMINAL_FINISH;
		}
	} else if (state == SLAVE_ACCESS_GET_METER_INFO) {
		p->meterState = TERMINAL_WORKING;
	} else {
		// Another working, will be report BUSY
		p->meterState = TERMINAL_BUSY;
	}
#else // #if REPEATER_DEVICE
	if (APP_isMetering()) { // 현재 검침중이면 재 검침하지 않음
		p->meterState = TERMINAL_WORKING; // 0: 검침중 1: 검침완료
	} else {
		if (METER_needNewMetering(10) ==
		    0) { // 즉시 검침의 경우에는 10초 이하이면 재 검침하지 않음.
			p->meterState = TERMINAL_FINISH;
		} else {
			needNewMetering = 1;
			p->meterState =
				TERMINAL_WORKING; // 0: 검침중 1: 검침완료
		}
	}
#endif // #if REPEATER_DEVICE

	p->nMeter = 1;

	p->oneMeter[0].type = conf.meterType;
	p->oneMeter[0].port = 1;

	if (tmpUnit->meterData[0] == 0xff && tmpUnit->meterData[1] == 0xff) {
		// previous metering failed
		p->oneMeter[0].flag = 1;
	} else {
		p->oneMeter[0].flag = 0;
		memcpy(p->oneMeter[0].serial, METER_getSerialNum(), 4);
		memcpy(p->oneMeter[0].value, tmpUnit->meterData, 4);
		if (conf.meterType == W_STANDARD_D) {
			p->oneMeter[0].status[0] = METER_getMeterCaliberDp();
		} else {
			p->oneMeter[0].status[0] = 0;
		}
		p->oneMeter[0].status[1] = tmpUnit->meterStatus;
	}

	int msgLen = sizeof(NfcMeterReport_t) -
		     (3 - p->nMeter) * sizeof(NfcOneMeterReport_t);
	send(msg, msgLen);

	if (needNewMetering) {
		// 현재 검침중이 아니고 이전 검침으로부터 재검침이 필요한 시간만큼 흘렀으면
		// 검침 시작
		OSAL_startEventTimer(APP_getTaskId(),
				     APP_EVENT_IMMEDIATE_METERING, (uint32)500);
	} else {
	}
}

static void makeMeteringDateTime(uint8 *p, Date_t *date)
{
	*(p + 0) = date->year - 2000;
	*(p + 1) = (date->mon & 0x0f) << 4;
	*(p + 1) |= ((date->day & 0x1e) >> 1);
	*(p + 2) = (date->day & 0x01) << 7;
	*(p + 2) |= ((date->hour & 0x1f) << 2);
	*(p + 2) &= 0xfc; // clear unused bits
}

static void sendPeriodMeterReport(int numOfDatas, FlashMeterData_t *periodDatas)
{
	uint8 msg[64];
	memset(msg, 0, sizeof(msg));

	// Flash data 개수에 따른 총 Report Block 개수 계산.
	int totalBlock = numOfDatas / MAX_METER_DATA_PER_REPORT;
	if (numOfDatas && (numOfDatas % MAX_METER_DATA_PER_REPORT) > 0) {
		totalBlock++;
	}

	NfcPeriodMeterReport_t *p = (NfcPeriodMeterReport_t *)msg;
	p->mtype = PERIOD_METER_REPORT;
	p->mversion = 0;
	p->totalBlock = totalBlock; // 0 ~ 255, 0 is no data
	p->meterNum =
		1; // 현재 복수의 계량기에 대한 고려가 안되어 있음. 필요하다만 추후 구현.

	uint8 blkOffset = 0;
	p->meterInfo[blkOffset++] =
		conf.meterType; // 계량기 타입 (0인 경우 STANDARD로 설정)
	if (conf.meterType == W_STANDARD_D) {
		p->meterInfo[blkOffset++] =
			METER_getMeterCaliberDp(); // 구경(VIF, MSB 4bit), 소수점 위치(DIF, LSB
			// 4bit)
	}

	if (!totalBlock) {
		send(msg, sizeof(NfcPeriodMeterReport_t));
	} else {
		uint8 offset = 0;
		for (int block = 1; block <= totalBlock; block++) {
			offset = blkOffset;
			// 기간검침 데이터가 존재할 경우 Current Block은 1부터 시작, 마지막
			// Current Block 은 Total Block 값과 동일.
			p->currentBlock = block; // 0 ~ 255, 0 is no data

			// 각 4개의 Data 중 가장 마지막에 저장된 데이터의 hour interval을 입력.
			p->meterInfo[offset++] =
				METER_DATA_SAVE_INTERVAL; // Hour interval

			uint8 dataNum;
			if (numOfDatas < (block * MAX_METER_DATA_PER_REPORT)) {
				dataNum =
					numOfDatas % MAX_METER_DATA_PER_REPORT;
			} else {
				dataNum = MAX_METER_DATA_PER_REPORT;
			}
			p->meterInfo[offset++] =
				dataNum; // Total number of meter data in current block

			Date_t date;
			uint8 dateTime[3];
			for (int idx = 0; idx < dataNum; idx++) {
				FlashMeterData_t *pFlashData =
					(periodDatas + idx +
					 (block - 1) *
						 MAX_METER_DATA_PER_REPORT);
				date.year = pFlashData->year + 2000;
				date.mon = pFlashData->mon;
				date.day = pFlashData->day;
				date.hour = pFlashData->hour;
				date.min = date.sec = 0;

				makeMeteringDateTime(dateTime, &date);

				// Year 8bit, Mon 4bit, Day 5bit, Hour 5bit, Not userd 2bit
				int arrSize = sizeof(dateTime);
				memcpy(&(p->meterInfo[offset]), dateTime,
				       arrSize);
				offset += (arrSize / sizeof(dateTime[0]));

				arrSize = sizeof(pFlashData->firstValue);
				memcpy(&(p->meterInfo[offset]),
				       pFlashData->firstValue, arrSize);
				offset += (arrSize /
					   sizeof(pFlashData->firstValue[0]));

				// Diff data of "2nd data - 1st data", Hex value
				if (pFlashData->hourInterval !=
				    HOUR_INTERVAL_NO_DATA) {
					p->meterInfo[offset++] =
						pFlashData->diff[0];
					p->meterInfo[offset++] =
						pFlashData->diff[1];
				} else {
					// Diff 데이터가 없는 경우 표현할 방법이 없으므로 MT DOWN으로
					// 표시한다.
					p->meterInfo[offset++] =
						METER_DIFF_2ND_MT_DOWN & 0x00FF;
					p->meterInfo[offset++] =
						METER_DIFF_2ND_MT_DOWN &
						0xFF00 >> 8;
				}
				p->meterInfo[offset++] =
					pFlashData->batt; // Battery
			}

			continueSend(msg, sizeof(NfcPeriodMeterReport_t));
			if (NFC_checkRead(100) == FALSE)
				break;
		}
	}
}

void sendFwVerReport()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcFwVerReport_t *p = (NfcFwVerReport_t *)msg;

	p->mtype = FW_VER_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;

	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);

	send(msg, sizeof(NfcFwVerReport_t));
}

void sendDeviceInfo()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcDevInfoReport_t *p = (NfcDevInfoReport_t *)msg;

	p->mtype = DEVICE_INFO_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;

	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);
	memcpy(p->serialNum, conf.serialNum, SERIAL_NUM_LEN);
	p->deviceCode = getMyDeviceCode();

	p->deviceType = conf.termModel;

	send(msg, sizeof(NfcDevInfoReport_t));
}

void sendServerConnectReport(int reqType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

#if NBIOT_DEVICE
	NfcServerConnReportV2_t *p = (NfcServerConnReportV2_t *)msg;
	p->mtype = SERVER_CONNECT_REPORT;
	p->mversion = NFC_PROTOCOL_VER_2;
#else
	NfcServerConnReportV1_t *p = (NfcServerConnReportV1_t *)msg;
	p->mtype = SERVER_CONNECT_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;
#endif

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
	if (accessState == ACCESS_STATE_IN_SETTING ||
	    accessState == ACCESS_STATE_IN_PROGRESS) {
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

#if NBIOT_DEVICE
	send(msg, sizeof(NfcServerConnReportV2_t));
#else
	send(msg, sizeof(NfcServerConnReportV1_t));
#endif

	if (reqType == 1) { // 접속 시작 명령
		if (accessState == ACCESS_STATE_IDLE) {
			// 모뎀이 보고 중이면 새로운 보고를 지시하지 않음
			OSAL_stopEventTimer(APP_getTaskId(),
					    APP_EVENT_IMMEDIATE_REPORT);
			OSAL_startEventTimer(APP_getTaskId(),
					     APP_EVENT_IMMEDIATE_REPORT,
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

void sendPulseMeterValueAck()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcPulseMeterValueSetAck_t *p = (NfcPulseMeterValueSetAck_t *)msg;

	p->mtype = PULSE_METER_VALUE_ACK;
	p->mversion = NFC_PROTOCOL_VER_1;

	send(msg, sizeof(NfcPulseMeterValueSetAck_t));
}

void sendBdControlAck(uint8 mversion, uint8 reset, uint8 sleepMode,
		      uint8 reportMode)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	// Protocol version 관계없이 동일한 Data 먼저 처리.
	NfcBdCtrlAckV1_t *p = (NfcBdCtrlAckV1_t *)msg;

	p->mtype = BD_CONTROL_ACK;
	p->mversion = mversion;
	p->reset = reset;

	if (sleepMode) {
		p->sleepMode = SLEEP_MODE_SLEEP;
	} else {
		p->sleepMode = SLEEP_MODE_NORMAL;
	}

	// Protocol version에 따라 다른 부분 처리.
	int messageLen;
	if (mversion == NFC_PROTOCOL_VER_1) {
		messageLen = sizeof(NfcBdCtrlAckV1_t);
	} else {
		messageLen = sizeof(NfcBdCtrlAckV2_t);

		NfcBdCtrlAckV2_t *p = (NfcBdCtrlAckV2_t *)msg;
		if (reportMode) {
			p->reportMode = REPORT_MODE_TERMINAL;
		} else {
			p->reportMode = REPORT_MODE_SERVER;
		}
	}

	send(msg, messageLen);
}

#if REPEATER_DEVICE
void sendSlaveCheckReport()
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcSlaveCheckReport_t *p = (NfcSlaveCheckReport_t *)msg;

	p->mtype = SLAVE_CHECK_REPORT;
	p->mversion = NFC_PROTOCOL_VER_1;

	memcpy(p->nwkAddr, (void *)&conf.slaveNwk, sizeof(uint32));

	SlaveAccessState_t state = SLAVE_getAccessState();
	if (state == SLAVE_ACCESS_IDLE) {
		if (SLAVE_needToUpdateSlaveInfo()) {
			// Only check modem working. (not wait delay for MODEM)
			if (getModemAccessState() == ACCESS_STATE_IN_PROGRESS) {
				p->checkState = TERMINAL_BUSY;
			} else {
				SLAVE_updateSlaveInfo();
				p->checkState = TERMINAL_WORKING;
			}
		} else {
			// 정해진 Interval내 받은 Slave info가 있을 경우 해당 데이터 전송.
			SlaveAccessInfo_t pStatus;
			memset(&pStatus, 0, sizeof(SlaveAccessInfo_t));

			SLAVE_getSlaveInfo(&pStatus);

			p->checkState = (pStatus.lastError == SLAVE_SUCCESS) ?
						TERMINAL_FINISH :
						TERMINAL_FAIL;
			memcpy(p->slaveSerialNum, pStatus.slaveSerial,
			       LEN_SLAVE_SERIAL_NUM);
			p->slaveBatt = pStatus.slaveBatt;
			p->slaveRssi = pStatus.slaveRssi;
			p->masterRssi = pStatus.masterRssi;
		}
	} else if (state == SLAVE_ACCESS_GET_SLAVE_INFO) {
		p->checkState = TERMINAL_WORKING;
	} else {
		p->checkState = TERMINAL_BUSY;
	}

	send(msg, sizeof(NfcSlaveCheckReport_t));
}
#endif // #if REPEATER_DEVICE

///////////////////////////////
/////  Recv Message
///////////////////////////////

// void Recv_Node_Conf_Req(byte* data, uint8 len)
// {
//     sendNodeConfReport(REPORT_TYPE_REQ, 0);
// }

void recvNodeConfSetV1(byte *data, uint8 len)
{
	NfcConfSetV1_t *p = (NfcConfSetV1_t *)data;

	setRtcV1(p->year);

	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));

	int error = 0;
	int changed = 0;
	do {
		// p->serialNum --> check하여 에러 처리할 필요가 있는가?
		if (memcmp(p->serialNum, tconfig.serialNum, SERIAL_NUM_LEN) !=
		    0) {
			error = CONFIG_ERR_SERIAL_NUM;
			break;
		}

		if (p->sleepMode == SLEEP_MODE_SLEEP) {
			if (!tconfig.sleepMode) {
				tconfig.sleepMode = 1;
				changed = 1;
			}
		} else if (p->sleepMode == SLEEP_MODE_NORMAL) {
			if (tconfig.sleepMode) {
				tconfig.sleepMode = 0;
				changed = 1;
			}
		}

		uint8 mi = p->meterInterval;
		uint8 ri = p->reportInterval;

		if ((24 % mi) != 0 || (24 % ri) != 0 || mi > ri) {
			error = CONFIG_ERR_METERING_PARAM;
			break;
		}

		if (tconfig.meterInterval != mi) {
			tconfig.meterInterval = mi;
			changed = 1;
		}

		if (tconfig.reportInterval != ri) {
			tconfig.reportInterval = ri;
			changed = 1;
		}

#if NBIOT_DEVICE
		char newIP[SERVER_IP_STR_LEN];
		sprintf(newIP, "%d.%d.%d.%d", p->serverIp[3], p->serverIp[2],
			p->serverIp[1], p->serverIp[0]);

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
#endif
		uint8 mt = p->meterInfo[0].meterType;
		if (mt != W_STANDARD_D && mt != W_SHINHAN_D &&
		    mt != W_SHINHAN_D_BIG && mt != W_MNS_D) {
			error = CONFIG_ERR_METER_TYPE;
			break;
		}

		if (tconfig.meterType != mt) {
			tconfig.meterType = mt;
			changed = 1;
		}

		// TODO : Check meter port (ref. Need to implementation dynamic UART port
		// assign) error =  CONFIG_ERR_METER_PORT;

		error = 0;
	} while (0);

	if (error == 0 && changed) {
		memcpy(&conf, &tconfig, sizeof(Config_t));
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
	}

	sendNodeConfReport(REPORT_TYPE_SET, error);
}

#if REPEATER_DEVICE

void recvNodeConfSetV2(byte *data, uint8 len)
{
	NfcConfSetV2_t *p = (NfcConfSetV2_t *)data;

	setRtcV2(p->dateTime);

	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));

	int error = 0;
	int changed = 0;
	do {
		// p->serialNum --> check하여 에러 처리할 필요가 있는가?
		if (memcmp(p->serialNum, tconfig.serialNum, SERIAL_NUM_LEN) !=
		    0) {
			error = CONFIG_ERR_SERIAL_NUM;
			break;
		}

		if (p->sleepMode == SLEEP_MODE_SLEEP) {
			if (!tconfig.sleepMode) {
				tconfig.sleepMode = 1;
				changed = 1;
			}
		} else if (p->sleepMode == SLEEP_MODE_NORMAL) {
			if (tconfig.sleepMode) {
				tconfig.sleepMode = 0;
				changed = 1;
			}
		}

		uint8 mi = p->meterInterval;
		uint8 ri = p->reportInterval;

		if ((24 % mi) != 0 || (24 % ri) != 0 || mi > ri) {
			error = CONFIG_ERR_METERING_PARAM;
			break;
		}

		if (tconfig.meterInterval != mi) {
			tconfig.meterInterval = mi;
			changed = 1;
		}

		if (tconfig.reportInterval != ri) {
			tconfig.reportInterval = ri;
			changed = 1;
		}

#if NBIOT_DEVICE
		char newIP[SERVER_IP_STR_LEN];
		sprintf(newIP, "%d.%d.%d.%d", p->serverIp[3], p->serverIp[2],
			p->serverIp[1], p->serverIp[0]);

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
#endif
		uint16 panId;
		uint32 nwkAddr;
		memcpy((void *)&panId, p->amiInfo.panId, sizeof(uint16));
		memcpy((void *)&nwkAddr, p->amiInfo.nwkAddr, sizeof(uint32));

		if (tconfig.pan_id != panId) {
			tconfig.pan_id = panId;
			changed = 1;
		}

		if (tconfig.nwk_addr != nwkAddr) {
			tconfig.nwk_addr = nwkAddr;
			// Master NWK변경 시 Slave도 변경되야 하므로 새로운 NWK로 변경.
			tconfig.slaveNwk = tconfig.nwk_addr;
			changed = 1;
		}

		int slaveIdPos = getFirstZeroPosition(tconfig.nwk_addr);
		uint8 *slaveNwk = (uint8 *)&tconfig.slaveNwk;
		if (*(slaveNwk + slaveIdPos) != p->amiInfo.slaveId) {
			*(slaveNwk + slaveIdPos) = p->amiInfo.slaveId;
			changed = 1;
		}

		error = 0;
	} while (0);

	if (error == 0 && changed) {
		memcpy(&conf, &tconfig, sizeof(Config_t));
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
	}

	sendNodeConfReport(REPORT_TYPE_SET, error);
}

#endif // #if REPEATER_DEVICE

void recvNodeConfSet(byte *data, uint8 len)
{
#if REPEATER_DEVICE
	recvNodeConfSetV2(data, len);
#else
	recvNodeConfSetV1(data, len);
#endif
}

void recvMeterReq(byte *data, uint8 len)
{
	NfcMeterReq_t *p = (NfcMeterReq_t *)data;

	setRtcV1(p->year);
	sendMeterReport();
}

void recvPeriodMeterReq(byte *data, uint8 len)
{
	NfcPeriodMeterReq_t *p = (NfcPeriodMeterReq_t *)data;

	// Day 단위 request 이므로 From Date는 시분초를 최소로 설정.
	Date_t fromDate = { .year = ((p->from_year[1] << 8) | p->from_year[0]),
			    .mon = p->from_mon,
			    .day = p->from_day,
			    .hour = 0,
			    .min = 0,
			    .sec = 0 };

	// Day 단위 request 이므로 To Date는 시분초를 최대로 설정.
	Date_t toDate = { .year = ((p->to_year[1] << 8) | p->to_year[0]),
			  .mon = p->to_mon,
			  .day = p->to_day,
			  .hour = 23,
			  .min = 59,
			  .sec = 59 };

	int numOfDatas;
	FlashMeterData_t *periodDatas =
		FLASH_readPeriodData(&fromDate, &toDate, &numOfDatas);
	sendPeriodMeterReport(numOfDatas, periodDatas);
	free(periodDatas);
}

void recvServerConnectReq(byte *data, uint8 len)
{
	NfcServerConnReq_t *p = (NfcServerConnReq_t *)data;
	sendServerConnectReport(p->reqType);
}

void recvPulseMeterValueSet(byte *data, uint8 len)
{
	// NfcPulseMeterValueSet_t *p = (NfcPulseMeterValueSet_t *)data;
	sendPulseMeterValueAck();
}

void recvBdControlReq(byte *data, uint8 len)
{
	NfcBdCtrlReqV1_t *p = (NfcBdCtrlReqV1_t *)data;

	int sleepModeChanged = 0;
	int reportModeChanged = 0;

	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));

	// Protocol version 관계없는 공통부분 설정 확인.
	if (p->sleepMode == SLEEP_MODE_SLEEP) {
		if (tconfig.sleepMode == 0) {
			tconfig.sleepMode = 1;
			sleepModeChanged = 1;
		}
	} else if (p->sleepMode == SLEEP_MODE_NORMAL) {
		if (tconfig.sleepMode) {
			tconfig.sleepMode = 0;
			sleepModeChanged = 1;
		}
	}

	int mVersion = p->mversion;
	// Protocol version 2에 대한 설정 확인.
	if (mVersion == NFC_PROTOCOL_VER_2) {
		NfcBdCtrlReqV2_t *pMsg = (NfcBdCtrlReqV2_t *)data;
		if (pMsg->reportMode == REPORT_MODE_SERVER) {
			if (tconfig.rtCtrlMode) {
				tconfig.rtCtrlMode = 0;
				tconfig.rtCtrlThreshold = 0;
				tconfig.rtCtrlInterval = tconfig.reportInterval;
				reportModeChanged = 1;
			}
		} else if (pMsg->reportMode == REPORT_MODE_TERMINAL) {
			if (tconfig.rtCtrlMode == 0) {
				tconfig.rtCtrlMode = 1;
				tconfig.rtCtrlThreshold = 0;
				tconfig.rtCtrlInterval = tconfig.reportInterval;
				reportModeChanged = 1;
			}
		}
	}

	if (sleepModeChanged || reportModeChanged) {
		// 설정 변경 시 Global context에 복사 후 이벤트 등록
		memcpy(&conf, &tconfig, sizeof(Config_t));
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
	} else {
		if (p->reset) {
			OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
		}
	}

	if (mVersion == NFC_PROTOCOL_VER_1) {
		sendBdControlAck(mVersion, p->reset, conf.sleepMode,
				 REPORT_MODE_NOT_USE);
	} else {
		sendBdControlAck(mVersion, p->reset, conf.sleepMode,
				 conf.rtCtrlMode);
	}
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
	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);

	p->result = 1; // SN mismatch

	send(msg, sizeof(NfcMeterBypassError_t));
}

void NFCAPP_meterBypassReq()
{
	NfcMeterBypassReq_t *p = (NfcMeterBypassReq_t *)nfcAsyncRx.cmd;

	if (p->whatWithSN != IGNORE_SERIAL_NUM) {
		if (memcmp(conf.serialNum, p->serialNum, SERIAL_NUM_LEN) != 0) {
			if (p->whatWithSN == CHANGE_SERIAL_NUM) {
				memcpy(conf.serialNum, p->serialNum,
				       SERIAL_NUM_LEN);
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
	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);

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
#if NBIOT_DEVICE

void sendNbiotIdReport(uint8 reportType)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcNbiotIdReport_t *p = (NfcNbiotIdReport_t *)msg;

	p->mtype = NB_ID_REPORT;
	p->mversion = 0;
	p->reportType = reportType;

	MODEM_getCseId(p->cseid);
	MODEM_getIccid(p->iccid);

	send(msg, sizeof(NfcNbiotIdReport_t));
}

void recvNbiotIdSet(byte *data, uint8 len)
{
	NfcNbiotIdSet_t *p = (NfcNbiotIdSet_t *)data;

	Config_t tconfig;
	memcpy(&tconfig, &conf, sizeof(Config_t));
	if (memcmp(tconfig.serviceCode, p->serviceCode, 4) != 0) {
		memcpy(tconfig.serviceCode, p->serviceCode, 4);
		if (MODEM_makeCseID(tconfig.serviceCode, NULL, NULL, NULL, 0)) {
			// 설정 변경 시 Global context에 복사 후 이벤트 등록
			memcpy(&conf, &tconfig, sizeof(Config_t));
			OSAL_setEvent(APP_getTaskId(), APP_EVENT_CHANGE_CONFIG);
		}
	}

	sendNbiotIdReport(REPORT_TYPE_SET);
}

#endif

#if LORA_DEVICE

void sendLoRaAppEuiKeyReport(uint8 reportType, int result, int error)
{
	byte msg[64];
	memset(msg, 0, sizeof(msg));

	NfcLoRaAppEuiKeyReport_t *p = (NfcLoRaAppEuiKeyReport_t *)msg;

	p->mtype = LORA_APP_EUI_KEY_REPORT;
	p->mversion = 0;
	p->reportType = reportType;
	p->result = result;
	p->errorCode = error;
	if (result == TERMINAL_FINISH && error == CONFIG_ERR_NONE) {
		// 정상완료인 경우에만 설정
		MODEM_getAppEui(p->appEui);
		MODEM_getAppKey(p->appKey);
	}

	memcpy(p->fwVer, APP_getFwVer(conf.isUsingPF), FIRMWARE_VER_LEN);
	memcpy(p->serialNum, conf.serialNum, SERIAL_NUM_LEN);

	send(msg, sizeof(NfcLoRaAppEuiKeyReport_t));
}

void recvLoRaAppEuiKeySet(byte *data, uint8 len)
{
	NfcLoRaAppEuiKeySet_t *p = (NfcLoRaAppEuiKeySet_t *)data;

	int result, error;
	if (memcmp(p->serialNum, conf.serialNum, SERIAL_NUM_LEN) != 0) {
		//  Serial number 불일치 애러
		result = TERMINAL_FAIL;
		error = CONFIG_ERR_SERIAL_NUM;
	} else {
		if (getModemAccessState() == ACCESS_STATE_IN_SETTING) {
			// 모뎀 설정 중
			result = TERMINAL_WORKING;
			error = CONFIG_ERR_NONE;
		} else {
			JoinMode_t joinMode =
				(p->joinMode == JOIN_MODE_SEUDO_JOIN) ?
					LORA_SEUDO_JOIN :
					LORA_REAL_JOIN;
			if (MODEM_setUserNwk(p->appEui, p->appKey, joinMode)) {
				// 현재 AppKey or AppEUI 이 불일치 할 경우 TRUE로 설정 시작.
				result = TERMINAL_WORKING;
				error = CONFIG_ERR_NONE;
			} else {
				// 모뎀 AppKey and AppEUI 이 요청받은 값과 동일할 경우 완료 및 보고.
				result = TERMINAL_FINISH;
				error = CONFIG_ERR_NONE;
			}
		}
	}

	sendLoRaAppEuiKeyReport(REPORT_TYPE_SET, result, error);
}

#endif

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
		// Recv_Node_Conf_Req(data, len);
		sendNodeConfReport(REPORT_TYPE_REQ, CONFIG_ERR_NONE);
		break;

	case LORA_CONF_SET:
	case NB_CONF_SET:
		recvNodeConfSet(data, len);
		break;

	case METER_REQ:
		recvMeterReq(data, len);
		break;

	case SERVER_CONNECT_REQ:
		recvServerConnectReq(data, len);
		break;

	case BD_CONTROL_REQ:
		recvBdControlReq(data, len);
		break;

	case PERIOD_METER_REQ:
		recvPeriodMeterReq(data, len);
		break;

	case PULSE_METER_VALUE_SET:
		// Not support this command
		sendFwVerReport();
		break;

	case DEVICE_INFO_REQ:
		sendDeviceInfo();
		break;

	case SLAVE_CHECK_REQ:
#if REPEATER_DEVICE
		sendSlaveCheckReport();
#else
		sendFwVerReport();
#endif
		break;

#if NBIOT_DEVICE
	case NB_ID_REQ:
		sendNbiotIdReport(REPORT_TYPE_REQ);
		break;

	case NB_ID_SET:
		recvNbiotIdSet(data, len);
		break;
#else
	case NB_ID_REQ:
	case NB_ID_SET:
		sendFwVerReport();
		break;
#endif

#if LORA_DEVICE
	case LORA_APP_EUI_KEY_REQ:
		sendLoRaAppEuiKeyReport(REPORT_TYPE_REQ, TERMINAL_FINISH,
					CONFIG_ERR_NONE);
		break;

	case LORA_APP_EUI_KEY_SET:
		recvLoRaAppEuiKeySet(data, len);
		break;
#else
	case LORA_APP_EUI_KEY_REQ:
	case LORA_APP_EUI_KEY_SET:
		sendFwVerReport();
		break;
#endif
	case METER_BYPASS_REQ:
		recvMeterBypassRequest(data, len);
		break;
	}
}
