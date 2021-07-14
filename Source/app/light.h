#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "var_type.h"

typedef struct {
	uint8 mode;
	uint8 timeBase;
	uint8 region;
	uint8 onTime[2];
	uint8 offTime[2];
	uint8 setOnTime[2];
	uint8 setOffTime[2];
	uint8 onOfs;
	uint8 offOfs;
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
