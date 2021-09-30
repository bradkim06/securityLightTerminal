#ifndef __METER_H__
#define __METER_H__

#include "lcd.h"

#define RELAY_STATUS_STR(x)                                                                        \
	(((x) == RELAY_OFF) ?                                                                      \
		 "OFF" :                                                                           \
		 ((x) == RELAY_NO_CHANGE) ?                                                        \
		 "NO CHANGE" :                                                                     \
		 ((x) == RELAY_ON) ? "ON" : ((x) == RELAY_TOGGLE) ? "TOGGLE" : "UNKNOWN")

//=======================================
// Electric Meter Type
//=======================================
#define E_METER 0x01

#define RELAY_PIN 0x02

#define RELAY_OFF 0
#define RELAY_NO_CHANGE 1
#define RELAY_ON 2
#define RELAY_TOGGLE 3

extern int lightStatus;

//**************************************
// Meter data unit structure
//**************************************
typedef struct {
	uchar meterStatus;
	uint32 volt;
	uint32 curr;
	uint32 shortCurr;
	uint32 power;
	uint32 cumPower;
} MeterUnitData_t;

char *METER_getMeterName(int meterType);
void METER_clearTempData();
MeterUnitData_t *METER_getTempData();
void METER_addTempData(MeterUnitData_t *pUnit);

// 점멸기용 추가
void Relay_Status(int on, MeteringType_t type, Cause_t cause);
uchar getRelayStatus(void);

#endif // __METER_H__
