#include "app.h"
#include "light.h"
#include "misc.h"
#include "uart.h"

light_t light;

void setGps(double src, uint8 dstInt[], uint8 dstDec[])
{
	uint32 integer = (uint32)src;
	MISC_intToBCD(&integer, dstInt, 2);

	src -= (uint32)src;
	uint32 decimal = (uint32)(src * 1000000);
	MISC_intToBCD(&decimal, dstDec, 3);
}

void getGps(double *dst, uint8 srcInt[], uint8 srcDec[])
{
	uint32 integer;
	MISC_bcdToInt(srcInt, &integer, 2);

	uint32 decimal;
	MISC_bcdToInt(srcDec, &decimal, 3);

	*dst = integer + ((double)decimal / 1000000);
}

void initLightConfig(void)
{
	memset(&light, 0, sizeof(light_t));
	light.mode = LMODE_INTERNAL;
	light.timeBase = SUNRISE_BASE;

	// 초광산단서로 7
	// inlat = 37.529215;
	// inlon = 127.169043;
	// 대전시청
	// inlat = 36.35061;
	// inlon = 127.38477;
	double latit = 36.35061;
	double longit = 127.38477;
	setGps(latit, conf.latitInt, conf.latitDec);
	setGps(longit, conf.longitInt, conf.longitDec);
}
