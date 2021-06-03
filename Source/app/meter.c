#include "meter.h"
#include "common.h"
#include "misc.h"

#include "alarm.h"
#include "battery.h"
#include "flash.h"
#include "lcd.h"
#include "rtc.h"

typedef struct {
	uint8 meterSerial[4];
	MeterUnitData_t unit;
} MeteringData_t;

typedef struct {
	int nData;
	int storedInterval;
	MeterUnitData_t unit[MAX_NUM_STORED_DATA];
	MeterUnitData_t reportedUnit;
} MeterStoredData_t;

// 마지막 검침 데이터 저장 영역
MeteringData_t LastMetering;

//주기 검침 데이터 저장 영역
MeterStoredData_t StoredMeterData;

static void insertDateToData(Date_t *pDate, MeterUnitData_t *pData,
			     BOOL isIgnoreSec)
{
	// Data의 year는 RTC의 year에서 2000을 빼서 사용
	pData->year = pDate->year - 2000;
	pData->mon = pDate->mon;
	pData->day = pDate->day;
	pData->hour = pDate->hour;
	pData->min = pDate->min;
	pData->sec = (isIgnoreSec == TRUE) ? 0 : pDate->sec;
	pData->isTimeSync = ALARM_isTimeSync();
}

static void copyDateFromData(MeterUnitData_t *pData, Date_t *pDate,
			     BOOL isIgnoreSec)
{
	// RTC의 year는 Data의 year에서 2000을 더해야 함.
	pDate->year = pData->year + 2000;
	pDate->mon = pData->mon;
	pDate->day = pData->day;
	pDate->hour = pData->hour;
	pDate->min = pData->min;
	pDate->sec = (isIgnoreSec == TRUE) ? 0 : pData->sec;
}

char *METER_getMeterName(int meterType)
{
	char *name = "Unknown";
	switch (meterType) {
	case E_METER:
		name = "E_STD";
		break;
	}
	return name;
}

void METER_displayStoredData()
{
	//TODO METER_displayStoredData
	/* char termSerial[0x10] = ""; */
	/* strncpy(termSerial, (char *)conf.serialNum, SERIAL_NUM_LEN); */
	/* termSerial[SERIAL_NUM_LEN] = '\0'; */

	/* MeteringData_t *q = &LastMetering; */
	/* MeterStoredData_t *p = &StoredMeterData; */

	/* printf("# %d data, serial no: %02x%02x%02x%02x(%02x) T: %s\n", p->nData, */
	/*        q->meterSerial[0], q->meterSerial[1], q->meterSerial[2], */
	/*        q->meterSerial[3], q->caliberDp, termSerial); */

	/* for (int i = 0; i < p->nData; i++) { */
	/* 	MeterUnitData_t *pUnit = &p->unit[i]; */
	/* 	printf("   %2d)  %2d-%02d-%02d %02d:%02d:%02d   %02x%02x%02x%02x(%02x)\n", */
	/* 	       i + 1, pUnit->year, pUnit->mon, pUnit->day, pUnit->hour, */
	/* 	       pUnit->min, pUnit->sec, pUnit->meterData[0], */
	/* 	       pUnit->meterData[1], pUnit->meterData[2], */
	/* 	       pUnit->meterData[3], pUnit->meterStatus); */
	/* } */
}

void METER_displayTempData()
{
	//TODO METER_displayTempData
	/* char termSerial[0x10] = ""; */
	/* strncpy(termSerial, (char *)conf.serialNum, SERIAL_NUM_LEN); */
	/* termSerial[SERIAL_NUM_LEN] = '\0'; */

	/* MeteringData_t *p = &LastMetering; */

	/* printf("# serial no: %02x%02x%02x%02x(%02x) T: %s\n", p->meterSerial[0], */
	/*        p->meterSerial[1], p->meterSerial[2], p->meterSerial[3], */
	/*        p->caliberDp, termSerial); */

	/* MeterUnitData_t *pUnit = &LastMetering.unit; */
	/* printf("         %2d-%02d-%02d %02d:%02d:%02d   %02x%02x%02x%02x(%02x)\n", */
	/*        pUnit->year, pUnit->mon, pUnit->day, pUnit->hour, pUnit->min, */
	/*        pUnit->sec, pUnit->meterData[0], pUnit->meterData[1], */
	/*        pUnit->meterData[2], pUnit->meterData[3], pUnit->meterStatus); */
}

BOOL METER_isAllFF(uchar *p, int len)
{
	BOOL result = TRUE;

	for (int i = 0; i < len; i++) {
		if (*(p + i) != 0xFF) {
			result = FALSE;
			break;
		}
	}
	return result;
}

void METER_clearTempData()
{
	memset(&LastMetering, 0, sizeof(LastMetering));
}

int METER_getNumberOfStoredData()
{
	return StoredMeterData.nData;
}

int METER_getStoredInterval()
{
	return StoredMeterData.storedInterval;
}

uint8 *METER_getSerialNum()
{
	return LastMetering.meterSerial;
}

MeterUnitData_t *METER_getTempData()
{
	return &LastMetering.unit;
}

BOOL METER_needNewMetering(int secLimit)
{
	Date_t prev;
	copyDateFromData(&LastMetering.unit, &prev, FALSE);

	Date_t now;
	RTC_read(&now);

	int32 secDiff = MISC_calcSecDiff(&prev, &now);

	return (secDiff < 0 || secDiff > secLimit) ? TRUE : FALSE;
}

MeterUnitData_t *METER_getStoredData(int pos)
{
	int nData = METER_getNumberOfStoredData();
	if (pos >= 0 && pos < nData) {
		return &StoredMeterData.unit[pos];
	}
	return NULL;
}

void METER_addTempData(Date_t *pDate, MeterUnitData_t *pUnit)
{
	insertDateToData(pDate, pUnit, FALSE);
	memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));
}

int METER_getFirstValidPos()
{
	int pos = 0xFF;

	/* for (int i = 0; i < StoredMeterData.nData; i++) { */
	/* 	if (METER_isAllFF(StoredMeterData.unit[i].meterData, 4) == */
	/* 	    FALSE) { */
	/* 		pos = i; */
	/* 		break; */
	/* 	} */
	/* } */

	return pos;
}
