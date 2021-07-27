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
	MeterUnitData_t unit;
} MeteringData_t;

// 마지막 검침 데이터 저장 영역
MeteringData_t LastMetering;
uint8 relayStatus = RELAY_OFF;

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

MeterUnitData_t *METER_getTempData()
{
	return &LastMetering.unit;
}

void METER_addTempData(MeterUnitData_t *pUnit)
{
	memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));
}

void Relay_Status(int on, Cause_t cause)
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

#define RELAY_STATUS_STR(x)                                                                        \
	(((x) == RELAY_OFF) ?                                                                      \
		 "OFF" :                                                                           \
		 ((x) == RELAY_NO_CHANGE) ?                                                        \
		 "NO CHANGE" :                                                                     \
		 ((x) == RELAY_ON) ? "ON" : ((x) == RELAY_TOGGLE) ? "TOGGLE" : "UNKNOWN")

	printf_ts("Relay Status : %s[Command], %s[Status]\n", RELAY_STATUS_STR(on),
		  RELAY_STATUS_STR((P3OUT & RELAY_PIN)));

	relayStatus = P3OUT & RELAY_PIN;

	if (on == RELAY_TOGGLE) {
		return;
	}

	if (prevStatus != on) {
		prevStatus = on;
		if (on == RELAY_ON) {
			printf_ts("light on\n");
			APP_runMetering(LIGHT_ON, cause);
		} else if (on == RELAY_OFF) {
			printf_ts("light off\n");
			APP_runMetering(LIGHT_OFF, cause);
		}
	}
}

uchar getRelayStatus(void)
{
#define LAMP_ON 1
#define LAMP_OFF 2
#define LAMP_PSM 3
	uchar retVal = LAMP_OFF;

	if (P3OUT & RELAY_PIN) {
		retVal = LAMP_ON;
	}

	if (light.psm) {
		retVal = LAMP_PSM;
	}

	return retVal;
}
