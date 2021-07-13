#ifndef __SUNRISE_H__
#define __SUNRISE_H__

typedef struct {
	int hour;
	int min;
} sunriseTime_t;

void calcSunrise(Date_t *date);
BOOL updateSunTime(Date_t *date);
#endif
