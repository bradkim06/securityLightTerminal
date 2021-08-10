#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "var_type.h"

// Light Termianl Mode
#define LMODE_INTERNAL 1
#define LMODE_SETTING 2

// Light Time Base
#define SUNRISE_BASE 1
#define CIVIL_BASE 2

// PSM Mode
#define PSM_NOT_USING 0
#define PSM_RELIGHT 1
#define PSM_NOT_RELIGHT 2

// Dimming
#define DIMM_NOT_USE 0
#define DIMM_USE 1

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
} light_t; // 24

void setGps(double src, uint8 dstInt[], uint8 dstDec[]);
void getGps(double *dst, uint8 srcInt[], uint8 srcDec[]);
void BYTE_bcdToInt(uint8 *pBCD, uint8 *pInt);
void BYTE_intToBCD(uint8 *pInt, uint8 *pBCD);

#endif
