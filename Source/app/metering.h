#ifndef __METERING_H__
#define __METERING_H__

#include "meter.h"

#define LEN_MAX_METER_TX 0x40
#define LEN_MAX_METER_RX 0x60

//**************************************
// MNS METER MSG 정의
//**************************************
typedef struct {
  uint8 startByte1;  // fixed value - 0x2a('*')
  uint8 userID[10];
  uint8 meterValue[8];
  uint8 status[2];
  uint8 carriageReturn1;
  uint8 startByte2;  // fixed value - 0x26('&')
  uint8 version[2];
  uint8 etc[30];
  uint8 checksum[2];
  uint8 carriageReturn2;
} MnsMeter_t;

#define MNS_CHECKSUM_START1 2
#define MNS_CHECKSUM_STOP1  11
#define MNS_CHECKSUM_START2 12
#define MNS_CHECKSUM_STOP2  21
#define MNS_CHECKSUM_START3 24
#define MNS_CHECKSUM_STOP3  55

//**************************************
// SHINHAN METER MSG 정의
//**************************************
#define SHINHAN_METER_FRAME_LEN  15
#define SHINHAN_BIG_METER_OFFSET 2

#define SHINHAN_METER_FRAME_LEN 15
#define SHINHAN_BIG_METER_FRAME_LEN \
  (SHINHAN_METER_FRAME_LEN + SHINHAN_BIG_METER_OFFSET)

#define SHINHAN_METER_DATA_LEN 11
#define SHINHAN_BIG_METER_DATA_LEN \
  (SHINHAN_METER_DATA_LEN + SHINHAN_BIG_METER_OFFSET)

#define SHINHAN_METER_SERIAL_LEN     4
#define SHINHAN_BIG_METER_SERIAL_LEN 6

typedef struct {
  uint8 irq_ack;  // always 0x06
  uint8 stx;      // always 0x02
  uint8 len;      // always 0x0d
  uint8 serial[6];
  uint8 value[4];
  uint8 battery;
  uint8 status;
  uint8 etx;  // always 0x03
  uint8 checksum;
} shBig_t;

typedef struct {
  uint8 irq_ack;  // always 0x06
  uint8 stx;      // always 0x02
  uint8 len;      // always 0x0b
  uint8 serial[4];
  uint8 value[4];
  uint8 battery;
  uint8 status;
  uint8 etx;  // always 0x03
  uint8 checksum;
} sh_t;

//**************************************
// STANDARD METER MSG 정의
//**************************************
#define STD_METER_DATA_REQ    0x5B  // terminal --> meter
#define STD_METER_DATA_RESP   0x08  // meter --> terminal
#define STD_METER_CONFIG_REQ  0xA2  // terminal --> meter
#define STD_METER_ADJUST_REQ  0xA0  // terminal --> meter
#define STD_METER_ADJUST_RESP 0xB2  // meter --> terminal
#define STD_LCD_TEST_REQ      0xA6  // terminal --> meter
#define STD_LCD_MARK_REQ      0xA7  // terminal --> meter

// adjust frame result code
#define STD_METER_ADJUST_SUCCESS 0  // Others value is fail.

// frame delimiter (terminal --> meter)
#define STD_REQ_START    0x10
#define STD_REQ_ADDR     0x01
#define STD_REQ_CHECKSUM (STD_METER_DATA_REQ + STD_REQ_ADDR)
#define STD_REQ_STOP     0x16

// frame delimiter (meter --> terminal)
#define STD_RESP_START 0x68
#define STD_RESP_STOP  0x16

typedef struct {
  uint8 stx;
  uint8 cfield;  // STD_METER_DATA_REQ
  uint8 afield;
  uint8 checksum;
  uint8 etx;
} StdMeterReq_t;

typedef struct {
  uint8 stx;
  uint8 cfield;  // STD_METER_DISPLAY_LCD
  uint8 afield;
  uint8 pattern;  // 1 - 전체 표시
  uint8 checksum;
  uint8 etx;
} StdMeterLcd_t;

typedef struct {
  uint8 stx1;
  uint8 l1field;  // always 0xff
  uint8 l2field;  // always 0xff
  uint8 stx2;
  uint8 cfield;  // STD_METER_DATA_RESP
  uint8 afield;
  uint8 cifield;
  uint8 mdh;
  uint8 serial[4];
  uint8 status;
  uint8 dif;
  uint8 vif;
  uint8 value[4];
  uint8 checksum;
  uint8 etx;
} StdMeterDataResp_t;

typedef struct {
  uint8 stx;
  uint8 cfield;  // STD_METER_ADJUST_REQ
  uint8 afield;
  uint8 caliber;
  uint8 serial[4];
  uint8 q3[2];
  uint8 qt[2];
  uint8 q2[2];
  uint8 q1[2];
  uint8 maker;
  uint8 checksum;
  uint8 etx;
} StdMeterAdjustReq_t;

typedef struct {
  uint8 stx1;
  uint8 l1field;  // always 0x0f
  uint8 l2field;  // always 0x0f
  uint8 stx2;
  uint8 cfield;  // STD_METER_ADJUST_RESP
  uint8 caliber;
  uint8 serial[4];
  uint8 q3[2];
  uint8 qt[2];
  uint8 q2[2];
  uint8 q1[2];
  uint8 maker;
  uint8 fwVersion;
  uint8 errorCode;
  uint8 checksum;
  uint8 etx;
} StdMeterAdjustResp_t;

// configuration result structure parameter
typedef struct {
  uint8 caliber;
  uint8 serial[4];
  uint8 q3[2];
  uint8 qt[2];
  uint8 q2[2];
  uint8 q1[2];
  uint8 maker;
  uint8 fwVersion;
  uint8 errorCode;
} StdAdjustResult_t;

typedef struct {
  uint8 stx;
  uint8 cfield;  // STD_LCD_TEST_REQ
  uint8 afield;
  uint8 data;
  uint8 time;
  uint8 checksum;
  uint8 etx;
} StdLcdTestReq_t;

typedef struct {
  uint8 stx;
  uint8 cfield;  // STD_LCD_MARK_REQ
  uint8 afield;
  uint8 data;
  uint8 checksum;
  uint8 etx;
} StdLcdMarkReq_t;

//=======================================
// ONETL METER MSG 정의
//=======================================
// Meter message ID
#define ONETL_ID_READ_METER 0xA5  // Only for Calorie Meter
#define ONETL_ID_READ_ST_RT 0x5F  // Only for Calorie Meter
#define ONETL_ID_READ_FLOW  0x5D  // Common request message
#define ONETL_ID_READ_TOTAL 0xAA  // Only for Calorie Meter

// Meter address
#define ONETL_W_METER_ADDR 0x02  // Water Meter address
#define ONETL_H_METER_ADDR 0x03  // Hot water Meter address
#define ONETL_G_METER_ADDR 0x04  // GAS Meter address
#define ONETL_C_METER_ADDR 0x05  // Calorie Meter address

// frame delimiter (terminal --> meter)
#define ONETL_REQ_STX 0xE1
#define ONETL_REQ_ETX 0xF0

// frame delimiter (meter --> terminal)
#define ONETL_RESP_STX 0xE9
#define ONETL_RESP_ETX 0xF0

// Battery status
#define ONETL_STATUS_BATT_EMPTY 0x00
#define ONETL_STATUS_BATT_LOW   0x01
#define ONETL_STATUS_BATT_FULL  0x02

// BCC AND operatin constant
#define ONETL_BCC_AND_CONSTANT 0x7F

typedef struct {
  uint8 stx;
  uint8 id;
  uint8 len;
  uint8 address;
} OntelHeader_t;

typedef struct {
  uint8 stx;
  uint8 id;  // STD_METER_DATA_REQ
  uint8 len;
  uint8 address;
  uint8 bcc;
  uint8 etx;
} OntelReq_t;

typedef struct {
  uint8 stx;
  uint8 id;
  uint8 len;
  uint8 address;
  uint8 serial[4];
  uint8 meterValue[5];
  uint8 status;
  uint8 bcc;
  uint8 etx;
} OntelFlowResp_t;

typedef struct {
  uint8 stx;
  uint8 id;
  uint8 len;
  uint8 address;
  uint8 serial[4];
  uint8 meterValue[5];
  uint8 st[2];
  uint8 rt[2];
  uint8 flowValue[5];
  uint8 status;
  uint8 bcc;
  uint8 etx;
} OntelTotalResp_t;

BOOL METER_sendRequest(uint8 meterType);
BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t* pUnit);
BOOL METER_std_configRequest();
BOOL METER_std_adjustReq(char*  sn,
                         uint8  caliber,
                         uint8  maker,
                         uint16 q3,
                         uint16 qt,
                         uint16 q2,
                         uint16 q1);
BOOL METER_std_adjustResp(StdAdjustResult_t* result);
BOOL METER_std_lcdTestReq();
BOOL METER_std_lcdMarkReq(BOOL isMarkOn);

void METER_enable(uint8 meterType);
void METER_disable();

void METER_bypassReq(uint8* body, int bodyLen);
BOOL METER_bypassResp();

#endif  //__METERING_H__
