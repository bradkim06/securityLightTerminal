#ifndef __SUNRISE_H__
#define __SUNRISE_H__

#define LSTATUS_STR(x) (((x) == LMODE_INTERNAL) ? "ON" : ((x) == LMODE_SETTING) ? "OFF" : "unknown")

#define LMODE_STR(x)                                                                               \
	(((x) == LMODE_INTERNAL) ? "Terminal Internal Algorithm" :                                 \
				   ((x) == LMODE_SETTING) ? "Server Setting Time" : "unknown")

#define TIMEBASE_STR(x)                                                                            \
	(((x) == SUNRISE_BASE) ? "Sunrise Time" :                                                  \
				 ((x) == CIVIL_BASE) ? "Civil Twilight Time" : "unknown")

#define PSM_MODE_STR(x)                                                                            \
	(((x) == PSM_NOT_USING) ?                                                                  \
		 "Psm Off" :                                                                       \
		 ((x) == PSM_RELIGHT) ? "Psm Relight" :                                            \
					((x) == PSM_NOT_RELIGHT) ? "Psm not Relight" : "unknown")

typedef struct {
	signed char hour;
	signed char min;
} sunriseTime_t;

void calcSunrise(Date_t *date);
BOOL updateSunTime(Date_t *date);
#endif
