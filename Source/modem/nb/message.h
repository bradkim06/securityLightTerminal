#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "common.h"

#define PROTOCOL_VERSION 0xA1

#define NBIOT_JOIN 0x30
#define NBIOT_ACK 0x50
#define NBIOT_INTERVAL_REQ 0x50
#define NBIOT_CMD_CHG_SERVER 0x5A
#define NBIOT_DATA_REPORT 0x70

#define NBIOT_QA_MSG_VER 1
#define NBIOT_ERR_MSG_VER 1

#define LEN_NBIOT_CTN 6

#define LEN_NBIOT_QA_BATT 3
#define LEN_NBIOT_QA_CGI 4
#define LEN_NBIOT_QA_RSRP 2
#define LEN_NBIOT_QA_SINR 2
#define LEN_NBIOT_QA_MODEL TERM_MODEL_STRING_LEN + 1
#define LEN_NBIOT_QA_FW_VER FIRMWARE_VER_LEN + 1
#define LEN_NBIOT_QA_TX_POWER 2
// 본래 위치 좌표 및 Neighbor CELL ID 데이터를 넣어야 하지만
// NB-IoT 자사 제품에서는 지원하지 않는 기능이므로 reserved 처리.
#define LEN_NBIOT_QA_RESERVED 2

#define NBIOT_QA_BATT_PERCENT 0
#define NBIOT_QA_BATT_VOLTAGE 1
#define NBIOT_QA_BATT_NOTUSE 2

#define LEN_NBIOT_ERR_S_TIME 6
#define LEN_NBIOT_ERR_P_TIME 2
#define LEN_NBIOT_ERR_P_PAYLOAD 2
#define LEN_NBIOT_ERR_UL_DL_CNT 2
#define LEN_NBIOT_ERR_E_LENGTH 2
#define LEN_NBIOT_ERR_E_LOG 7
#define CNT_NBIOT_ERR_E_LOG 4

#define NBIOT_UE_TYPE_WORK_PERIOD 1
#define NBIOT_UE_TYPE_WORK_EVENT 2
#define NBIOT_UE_TYPE_WORK_MIXED 3
#define NBIOT_UE_TYPE_MODE_PF_FIXED 1
#define NBIOT_UE_TYPE_MODE_PF_MOVED 2
#define NBIOT_UE_TYPE_MODE_NPF_FIXED 3
#define NBIOT_UE_TYPE_MODE_NPF_MOVED 4

#define NBIOT_ERR_CURRENT_UE_TYPE                                              \
	((NBIOT_UE_TYPE_MODE_NPF_FIXED << 4) | NBIOT_UE_TYPE_WORK_MIXED)

#define LEN_MAX_NBIOT_DATA                                                     \
	160 // 최대 156 bytes, 계산 편이성 상 160(=0xA0)로 설정.
#define LEN_NBIOT_ACK_DATA 26
#define LEN_NBIOT_CMD_CHG_SERVER_DATA 29

typedef struct {
	uchar msgVer;
	uchar ctn[LEN_NBIOT_CTN];
	uchar sendTime[LEN_NBIOT_ERR_S_TIME];
	uchar ueType;
	uchar periodLength;
	uchar periodTime[LEN_NBIOT_ERR_P_TIME];
	uchar periodPayload[LEN_NBIOT_ERR_P_PAYLOAD];
	uchar pfUlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar pfDlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar udpUlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar udpDlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar tcpUlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar tcpDlCnt[LEN_NBIOT_ERR_UL_DL_CNT];
	uchar npFlag;
	uchar errLength[LEN_NBIOT_ERR_E_LENGTH];
} NbiotErrorReportToLg_t;

typedef struct {
	uchar msgVer;
	uchar ctn[LEN_NBIOT_CTN];
	uchar batt[LEN_NBIOT_QA_BATT];
	uchar cgi[LEN_NBIOT_QA_CGI];
	uchar rsrp[LEN_NBIOT_QA_RSRP];
	uchar sinr[LEN_NBIOT_QA_SINR];
	uchar model[LEN_NBIOT_QA_MODEL];
	uchar fwVer[LEN_NBIOT_QA_FW_VER];
	uchar txPower[LEN_NBIOT_QA_TX_POWER];
	uchar reserved[LEN_NBIOT_QA_RESERVED];
} NbiotQaReportToLg_t;

typedef struct {
	uchar imei[8];
	uchar imsi[8];
} MobileId_t;

typedef struct {
	uchar rssi;
	uchar ber;
	uchar cid[2];
	uchar rsrp[2];
	uchar rsrq[2];
	uchar snr[2];
} NbiotRadioQuality_t;

typedef struct {
	uchar termSerial[5];
	uchar fwVer[2];
	uchar termBatt;
} NbiotDeviceInfo_t;

typedef struct {
	uchar meterSerial[4];
	uchar meterType;
	uchar meterCaliber_dp;
	uchar meterStatus;
} NbiotMeterInfo_t;

typedef struct {
	uchar mi;
	uchar ri;
} NbiotMeteringParam_t;

typedef struct {
	uchar year;
	uchar mon;
	uchar day;
	uchar hour;
	uchar min;
	uchar sec;
} NbiotTimeStamp_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	uchar refValue[4];
	uchar valueDiff[MAX_NUM_STORED_DATA][2];
} NbiotMeterData_t;

typedef struct {
	uchar resetCause;
	uchar resetCount[2];
} NbiotReset_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotRadioQuality_t radio; // 10 - 29
	NbiotDeviceInfo_t term; // 8  - 37
	NbiotMeterInfo_t meter; // 7  - 44
	NbiotMeteringParam_t mp; // 2  - 46
	NbiotTimeStamp_t meterTime; // 6  - 52
	uchar data[4]; // 4  - 56
	NbiotReset_t reset; // 3  - 59
	uchar checksum; // 1  - 60
} NbiotJoin_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotTimeStamp_t currTime; // 6  - 25
	uchar resetCommand; // 1  - 26
	uchar mi; // 1  - 27
	uchar ri; // 1  - 28
	uchar checksum; // 1  - 29
} NbiotAck_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotTimeStamp_t currTime; // 6  - 25
	uchar ip[4]; // 4  - 29
	uchar port[2]; // 2  - 31
	uchar checksum; // 1  - 32
} NbiotCmdChgServer_t;

typedef struct {
	char msg[8];
} NbiotResetReq_t;

typedef struct {
	char msg[4];
} NbiotSimpleAck_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	uchar mi; // 1  - 27
	uchar ri; // 1  - 28
	uchar checksum; // 1  - 29
} NbiotIntervalReq_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotRadioQuality_t radio; // 10 - 29
	NbiotDeviceInfo_t term; // 8  - 37
	NbiotMeterInfo_t meter; // 7  - 44
	NbiotMeteringParam_t mp; // 2  - 46
	NbiotTimeStamp_t meterTime; // 6  - 52
	NbiotMeterData_t data; // 7 + 2*nData --> 61 ~ 155
	uchar checksum; // 1  - 156
} NbiotDataReport_t;

int NBMSG_recvAck(char *p, int len);
int NBMSG_recvReqChgServer(char *p, int len, char *ip, uint16 *port);
int NBMSG_recvReqReset(uchar *buf, int len);
int NBMSG_recvSimpleAck(uchar *buf, int len);
int NBMSG_recvReqChgInterval(uchar *buf, int len);

/**
 * @brief Get the Qa Report Message object
 *        Return message is allocated on the heap. so, have to free
 *
 * @param messageLen QA report message length
 * @return char*     QA report message (converted to string)
 */
char *NBMSG_getQaReport(int *messageLen);

/**
 * @brief Get the Error Report Message object
 *        Return message is allocated on the heap. so, have to free
 *
 * @param messageLen Error report message length
 * @return char*     Error report message (converted to stringe)
 */
char *NBMSG_getErrorReport(int *messageLen);

/**
 * @brief Get the Meter Data Message object
 *        Return message is allocated on the heap. so, have to free
 *
 * @param isJoin     Flag specifying whether the message is a JOIN message or
 * not.
 * @param messageLen Meter data report message length
 * @return char*     Meter data report message (converted to string)
 */
char *NBMSG_getMeterReport(BOOL isJoin, int *messageLen);

#endif
