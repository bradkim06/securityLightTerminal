#ifndef __METER_H__
#define __METER_H__

#include "lcd.h"

//=======================================
// Electric Meter Type
//=======================================
#define E_METER 0x01

//**************************************
// Meter data unit structure
//**************************************
typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 isTimeSync;
	uchar meterStatus;
	uint32 volt;
	uint32 curr;
	uint32 shortCurr;
	int32 power;
	int32 cumPower;
} MeterUnitData_t;

char *METER_getMeterName(int meterType);

BOOL METER_isAllFF(uchar *p, int len);
void METER_displayTempData();
void METER_displayStoredData();
void METER_clearTempData();
void METER_clearStoredData(BOOL isClearAll);
MeterUnitData_t *METER_getTempData();
MeterUnitData_t *METER_getStoredData(int pos);
void METER_addTempData(Date_t *pDate, MeterUnitData_t *pUnit);
void METER_addStoredData(Date_t *pDate, MeterUnitData_t *pUnit);
void METER_collectStoredData();

void METER_saveInFlash(Date_t *pDate, MeterUnitData_t *pUnit);
BOOL METER_needNewMetering(int secLimit);
int METER_getNumberOfStoredData();
int METER_getStoredInterval();
uint8 *METER_getSerialNum();
int METER_getFirstValidPos();

#endif // __METER_H__
