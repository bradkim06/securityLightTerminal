#include "meter.h"
#include "common.h"
#include "misc.h"

#include "alarm.h"
#include "battery.h"
#include "flash.h"
#include "lcd.h"
#include "rtc.h"
#include "device.h"

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

void METER_clearTempData()
{
	memset(&LastMetering, 0, sizeof(LastMetering));
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

void METER_addTempData(Date_t *pDate, MeterUnitData_t *pUnit)
{
	insertDateToData(pDate, pUnit, FALSE);
	memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));
}

void Relay_Status(int on)
{
	static char prevStatus = RELAY_OFF;

	switch (on) {
	case RELAY_ON:
		P3OUT |= RELAY_PIN;
		break;
	case RELAY_OFF:
		P3OUT &= ~RELAY_PIN;
		break;
	case RELAY_TOGGLE:
		P3OUT ^= RELAY_PIN;
		break;
	case RELAY_NO_CHANGE:
	default:
		break;
	}

	if (prevStatus != on) {
		printf_ts("light status change\n");
		APP_runPeriodicReport();
	}
#define RELAY_STATUS_STR(x)                                                    \
	(((x) == RELAY_OFF) ? "OFF" :                                          \
			      ((x) == RELAY_NO_CHANGE) ?                       \
			      "NO CHANGE" :                                    \
			      ((x) == RELAY_ON) ?                              \
			      "ON" :                                           \
			      ((x) == RELAY_TOGGLE) ? "TOGGLE" : "UNKNOWN")

	printf_ts("Relay Status : %s[Command], %s[Status]\n",
		  RELAY_STATUS_STR(on), RELAY_STATUS_STR((P3OUT & RELAY_PIN)));
}
