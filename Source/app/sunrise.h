#ifndef __SUNRISE_H__
#define __SUNRISE_H__

typedef struct {
	signed char hour;
	signed char min;
} sunriseTime_t;

void calcSunrise(Date_t *date);
BOOL updateSunTime(Date_t *date);
#endif
