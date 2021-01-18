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
	uint8 meterData[4];
	uint8 meterStatus;
	uint8 st[2];
	uint8 rt[2];
	uint8 flowData[4];
	LcdIcon_t icon;
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
