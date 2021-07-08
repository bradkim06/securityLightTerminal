#ifndef __SUNRISE_H__
#define __SUNRISE_H__

typedef struct {
	int hour;
	int min;
} sunriseTime_t;

void calcSunrise(void);
#endif
