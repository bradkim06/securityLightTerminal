#ifndef __METERING_H__
#define __METERING_H__

#include "meter.h"

#define LEN_MAX_METER_TX 0x40
#define LEN_MAX_METER_RX 0x60

//**************************************
// Electric Meter Message
//**************************************

/* Terminal -> Meter */
#define TERMINAL_STX 0x24
#define TERMINAL_ETX 0x16

#define REQ_READ_METER 0xA0
#define SET_METER_CONFIG 0xA1
#define SET_LIGHT_TIME 0xA2
#define SET_RTC_TIME 0xA3
#define SET_CALIB_FACTOR 0xA4

/* Meter -> Terminal */
#define METER_STX 0x5B
#define METER_ETX 0x8D

#define RESP_READ_METER 0xB0
#define RESP_METER_CONFIG 0xB1
#define RESP_LIGHT_TIME 0xB2
#define RESP_RTC_TIME 0xB3
#define RESP_CALIB_FACTOR 0xB4

typedef struct {
	uint8 stx;
	uint8 msgType;
	uint8 checkSum;
	uint8 etx;
} evMeterReq_t;

BOOL METER_sendRequest(uint8 meterType);
BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t *pUnit);

void METER_enable(uint8 meterType);
void METER_disable();

void METER_bypassReq(uint8 *body, int bodyLen);
BOOL METER_bypassResp();

#endif //__METERING_H__
