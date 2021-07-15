#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "var_type.h"

// Light Termianl Mode
#define LMODE_INTERNAL 1
#define LMODE_SETTING 2

// Light Time Base
#define SUNRISE_BASE 0
#define CIVIL_BASE 1

// PSM Mode
#define PSM_NOT_USING 0
#define PSM_RELIGHT 1
#define PSM_NOT_RELIGHT 2

// Time
#define HOUR 0
#define MINUTE 1

#define MONTH 0
#define DAY 1

typedef struct {
	uint8 mode;
	uint8 timeBase;
	uint8 region;
	uint8 onTime[2];
	uint8 offTime[2];
	uint8 setOnTime[2];
	uint8 setOffTime[2];
	signed char onOfs;
	signed char offOfs;
	uint8 psm;
	uint8 psmOnTime[2]; // Hour, Min
	uint8 psmOffTime[2]; // Hour, Min
	uint8 psmOnDay[2]; // Month, Day
	uint8 psmOffDay[2]; // Month, Day
	uint8 dimmingMode;
	uint8 dimmingLv;
} light_t;

void initLightConfig(void);

#endif
