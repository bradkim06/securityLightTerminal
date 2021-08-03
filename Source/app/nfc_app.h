#ifndef _MESSAGE_HEADER_
#define _MESSAGE_HEADER_

#include "modem.h"
#include "var_type.h"

#define DEVICE_CODE_NB 0x01
#define DEVICE_CODE_LORA 0x02
#define DEVICE_CODE_SMART_PHONE 0xFA

//=======================================
// NFC Message Type
//=======================================
// SMART Phone -> Node
#define NODE_CONF_REQ 0x01
#define LORA_CONF_SET 0x02
#define NB_CONF_SET 0x03
#define SERVER_CONNECT_REQ 0x04
#define BD_CONTROL_REQ 0x05
#define NB_ID_REQ 0x06
#define NB_ID_SET 0x07
#define PSM_MODE_REQ 0x08
#define PSM_MODE_SET 0x09
#define LORA_APP_EUI_KEY_REQ 0x11
#define LORA_APP_EUI_KEY_SET 0x12
#define GPS_INFO_REQ 0x0A
#define GPS_INFO_SET 0x0B
#define MEASURE_REQ 0x0C

#define METER_BYPASS_REQ 0x41

// Node -> SMART Phone
#define LORA_CONF_REPORT 0x51
#define NB_CONF_REPORT 0x52
#define FW_VER_REPORT 0x53
#define SERVER_CONNECT_REPORT 0x54
#define BD_CONTROL_ACK 0x55
#define NB_ID_REPORT 0x56
#define PSM_MODE_REPORT 0x58
#define GPS_INFO_REPORT 0x5A
#define MEASURE_REPORT 0x5C
#define LORA_APP_EUI_KEY_REPORT 0x61

// TimeTable
#define SEOUL 1
#define BU_SAN 2
#define DAE_GU 3
#define IN_CHEON 4
#define GWANG_JU 5
#define DAE_JEON 6
#define UL_SAN 7
#define SU_WON 8
#define CHUN_CHEON 9
#define JIN_JU 10
#define CHEONG_JU 11
#define JEON_JU 12
#define JE_JU 13

#define METER_BYPASS_RESP 0x91
#define METER_BYPASS_ERROR 0x92

#define REPORT_TYPE_REQ 0
#define REPORT_TYPE_SET 1

#define NFC_PROTOCOL_VER_1 1
#define NFC_PROTOCOL_VER_2 2

#define MSG_OFFSET_LEN 2 // Device Code(1) + msgLen(1)
#define MSG_TYPE_POSITION 10

#define CONFIG_ERR_NONE 0
#define CONFIG_ERR_SERIAL_NUM 1
#define CONFIG_ERR_METERING_PARAM 2
#define CONFIG_ERR_METER_TYPE 3
#define CONFIG_ERR_METER_PORT 4
#define CONFIG_ERR_MODEM_FAIL 2

#define MAX_METER_DATA_PER_REPORT 4

#define REPORT_MODE_NOT_USE 0
#define REPORT_MODE_SERVER 1
#define REPORT_MODE_TERMINAL 2

#define SLEEP_MODE_NOT_USE 0
#define SLEEP_MODE_SLEEP 1
#define SLEEP_MODE_NORMAL 2

typedef struct {
	uint8 deviceCode;
	uint8 len;
	uint8 device_id[8];
} NfcHeader_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
} NfcMsg_t;

typedef struct {
	uint8 meterType;
	uint8 meterPort;
} NfcMeterInfo_t;

typedef struct {
	uint8 panId[2];
	uint8 nwkAddr[4];
	uint8 slaveId;
} NfcAmiInfo_t;

// Recv
// For Terminal message format (protocol version 1)
typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 serialNum[SERIAL_NUM_LEN];
	uint8 dateTime[4];
	uint8 serviceCode[4];
	uint8 serverIp[4];
	uint8 serverPort[2];
	uint8 lightMode;
	uint8 timeBase;
	uint8 reportInterval;
	uint8 timeTable;
	uint8 latitInt[2];
	uint8 latitDec[3];
	uint8 longitInt[2];
	uint8 longitDec[3];
	uint8 onOfs;
	uint8 offOfs;
} NfcConfSetV1_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 year[2];
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
} NfcMeterReq_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reqType;
} NfcServerConnReq_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reset;
	uint8 powerCtrl;
} NfcBdCtrlReq_t;

#define IGNORE_SERIAL_NUM 0
#define CHANGE_SERIAL_NUM 1
#define CHECK_SERIAL_NUM 2

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 serialNum[SERIAL_NUM_LEN]; // 0 - no change
	uint8 whatWithSN;
	uint8 bodyLen;
	uint8 body[39]; // 39 = 64-10-1-1-12-1
} NfcMeterBypassReq_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 serviceCode[4];
	uint8 serverIp[4];
	uint8 serverPort[2];
} NfcNbiotIdSet_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 psm;
	uint8 psmStartTime[2];
	uint8 psmEndTime[2];
	uint8 psmStartDate[2];
	uint8 psmEndDate[2];
	uint8 dimmMode;
	uint8 dimmLv;
} NfcPsmModeSet_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 timeTable;
	uint8 latitInt[2];
	uint8 latitDec[3];
	uint8 longitInt[2];
	uint8 longitDec[3];
} NfcGpsSet_t;

//////////////////////
///// Send
//////////////////////

// For Terminal message format (protocol version 1)
typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reportType;
	uint8 result;
	uint8 errorCode;
	uint8 serialNum[SERIAL_NUM_LEN];
	uint8 imsi[8];
	uint8 serviceCode[4];
	uint8 serverIp[4];
	uint8 serverPort[2];
	uint8 fwVer[4];
	uint8 batt;
	uint8 dateTime[4];
	uint8 lightMode;
	uint8 timeBase;
	uint8 reportInterval;
	uint8 timeTable;
	uint8 onOfs;
	uint8 offOfs;
} NfcConfReportV1_t;

// For Repeator message format (protocol version 2)
typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reportType;
	uint8 result;
	uint8 errorCode;
	uint8 serialNum[SERIAL_NUM_LEN];
	uint8 sleepMode;
	uint8 reserved;
	uint8 imsi[8];
	uint8 serverIp[4];
	uint8 serverPort[2];
	uint8 batt;
	uint8 fwVer[4];
	uint8 reportInterval;
	NfcAmiInfo_t amiInfo;
	uint8 dateTime[4];
} NfcConfReportV2_t;

typedef struct {
	uint8 type;
	uint8 port;
	uint8 flag;
	uint8 serial[4];
	uint8 value[4];
	uint8 status[2];
} NfcOneMeterReport_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 batt;
	uint8 year[2];
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 meterState;
	uint8 nMeter;
	NfcOneMeterReport_t oneMeter[3];
} NfcMeterReport_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 currentYear[2];
	uint8 currentMon;
	uint8 currentDay;
	uint8 currentHour;
	uint8 currentMin;
	uint8 currentSec;
	uint8 lastAccessYear[2];
	uint8 lastAccessMon;
	uint8 lastAccessDay;
	uint8 lastAccessHour;
	uint8 lastAccessMin;
	uint8 lastAccessSec;
	uint8 serverAccessState;
	uint8 accessStepOrResult;
	uint8 rssi;
	uint8 snr;
	uint8 rsrp;
	uint8 rsrq;
} NfcServerConnReportV2_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reset;
	uint8 powerCtrl;
} NfcBdCtrlAck_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 batt;
	uint8 year[2];
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 volt[4];
	uint8 curr[4];
	uint8 shortCurr[4];
	uint8 power[4];
	uint8 cumPower[4];
	uint8 status;
	uint8 alarm;
	uint8 onTime[2];
	uint8 offTime[2];
} NfcMeasureResp_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 fwVer[4];
} NfcFwVerReport_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 fwVer[4];
	uint8 serialNum[SERIAL_NUM_LEN]; // 0 - no change
	uint8 deviceCode;
	uint8 deviceType;
} NfcDevInfoReport_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 serialNum[SERIAL_NUM_LEN]; // 0 - no change
	uint8 fwVer[4];
	uint8 bodyLen;
	uint8 body[35]; // 35 = 64-10-1-1-12-4-1
} NfcMeterBypassResp_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 serialNum[SERIAL_NUM_LEN]; // 0 - no change
	uint8 fwVer[4];
	uint8 result;
} NfcMeterBypassError_t;

#if NBIOT_DEVICE
typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reportType;
	uint8 serviceCode[4];
	uint8 cseid[LEN_MODEM_EP_NAME];
	uint8 iccid[10];
} NfcNbiotIdReport_t;
#endif

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reportType;
	uint8 year[2];
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 psm;
	uint8 psmStartTime[2];
	uint8 psmEndTime[2];
	uint8 psmStartDate[2];
	uint8 psmEndDate[2];
	uint8 dimmMode;
	uint8 dimmLv;
} NfcPsmResp_t;

typedef struct {
	NfcHeader_t header;
	uint8 mtype;
	uint8 mversion;
	uint8 reportType;
	uint8 timeTable;
	uint8 latitInt[2];
	uint8 latitDec[3];
	uint8 longitInt[2];
	uint8 longitDec[3];
} NfcGpsResp_t;
// Functio Prototype

void NFCAPP_runMessage(byte *data, uint8 len);
void NFCAPP_meterBypassResp(uint8 *body, int bodyLen);
void NFCAPP_meterBypassReq();
void NFCAPP_clearAsyncCmd();

#endif // _MESSAGE_HEADER_
