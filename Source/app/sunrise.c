// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "rtc.h"
#include "sunrise.h"
#include "common.h"
#include "uart.h"

#define LMODE_STR(x)                                                                               \
	(((x) == LMODE_INTERNAL) ? "Terminal Internal Algorithm" :                                 \
				   ((x) == LMODE_SETTING) ? "Server Setting Time" : "unknown")
#define TIMEBASE_STR(x)                                                                            \
	(((x) == SUNRISE_BASE) ? "Sunrise Time" :                                                  \
				 ((x) == CIVIL_BASE) ? "Civil Twilight Time" : "unknown")

const double pi = 3.14159;

double rads;
double L, g;

//   Get the days to J2000
//   h is UT in decimal hours
//   FNday only works between 1901 to 2099 - see Meeus chapter 7

static double FNday(int y, int m, int d, float h)
{
	long int luku = -7 * (y + (m + 9) / 12) / 4 + 275 * m / 9 + d;
	// type casting necessary on PC DOS and TClite to avoid overflow
	luku += (long int)y * 367;
	return (double)luku - 730531.5 + h / 24.0;
};

//   the function below returns an angle in the range
//   0 to 2*pi

static double FNrange(double x)
{
	double b = 0.5 * x / pi;
	double a = 2.0 * pi * (b - (long)(b));
	if (a < 0)
		a = 2.0 * pi + a;
	return a;
};

// Calculating the hourangle
//
static double f0(double lat, double declin)
{
	// const double SunDia = 0.53; // Sunradius degrees
	// const double AirRefr = 34.0 / 60.0; // athmospheric refraction degrees //
	double fo, dfo;
#define SUNDIA 0.53
#define AirRefr 34.0 / 60.0
	// Correction: different sign at S HS
	dfo = rads * (0.5 * SUNDIA + AirRefr);
	if (lat < 0.0)
		dfo = -dfo;
	fo = tan(declin + dfo) * tan(lat * rads);
	if (fo > 0.99999)
		fo = 1.0; // to avoid overflow //
	fo = asin(fo) + pi / 2.0 + 0.01;
	// fo = asin(fo) + pi / 2.0;
	return fo;
};

// Calculating the hourangle for twilight times
//
static double f1(double lat, double declin)
{
	double fi, df1;
	// Correction: different sign at S HS
	df1 = rads * 6.0;
	if (lat < 0.0)
		df1 = -df1;
	fi = tan(declin + df1) * tan(lat * rads);
	if (fi > 0.99999)
		fi = 1.0; // to avoid overflow //
	fi = asin(fi) + pi / 2.0 + 0.053;
	return fi;
};

//   Find the ecliptic longitude of the Sun

static double FNsun(double d)
{
	//   mean longitude of the Sun

	L = FNrange(280.461 * rads + .9856474 * rads * d);

	//   mean anomaly of the Sun

	g = FNrange(357.528 * rads + .9856003 * rads * d);

	//   Ecliptic longitude of the Sun

	return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
};

// decimal hours to hours and minutes
static void gethrmn(double dhr, sunriseTime_t *p)
{
	int hr, mn;
	hr = (int)dhr;
	mn = (dhr - (double)hr) * 60;

	p->hour = hr;
	p->min = mn;
};

static void addOffset(sunriseTime_t *rise, sunriseTime_t *set)
{
	rise->min += light.onOfs;
	set->min += light.offOfs;

	if (rise->min >= 60) {
		rise->hour++;
		rise->min -= 60;
		if (rise->hour > 23) {
			rise->hour = 0;
		}
	} else if (rise->min < 0) {
		rise->hour--;
		rise->min += 60;
		if (rise->hour < 0) {
			rise->hour = 23;
		}
	}

	if (set->min >= 60) {
		set->hour++;
		set->min -= 60;
		if (set->hour > 23) {
			set->hour = 0;
		}
	} else if (set->min < 0) {
		set->hour--;
		set->min += 60;
		if (set->hour < 0) {
			set->hour = 23;
		}
	}
}

void calcSunrise(Date_t *date)
{
	int y = date->year;
	int m = date->mon;
	int day = date->day;

	if (y < 2021 || y > 2100) {
		printf("wrong year %d", y);
		return;
	}

	if (m > 12 || m < 1) {
		printf("wrong month %d", m);
		return;
	}

	if (day > 31 || m < 1) {
		printf("wrong day %d", day);
		return;
	}

	double h, latit, longit;
	double inlat, inlon, intz;
	double tzone, d, lambda;
	double obliq, alpha, delta, LL, equation, ha, hb, twx;
	double twam, settm, riset, twpm;

	rads = pi / 180.0;
	//  get the date and time from the user
	// read system date and extract the year

	getGps(&inlat, conf.latitInt, conf.latitDec);
	getGps(&inlon, conf.longitInt, conf.longitDec);
#define KST_ZONE 9
	intz = KST_ZONE;
	latit = (double)inlat;
	longit = (double)inlon;
	tzone = (double)intz;

	// testing
	// m=6; day=10;

	h = 12;
	d = FNday(y, m, day, h);

	//   Use FNsun to find the ecliptic longitude of the
	//   Sun

	lambda = FNsun(d);

	//   Obliquity of the ecliptic

	obliq = 23.439 * rads - .0000004 * rads * d;

	//   Find the RA and DEC of the Sun

	alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
	delta = asin(sin(obliq) * sin(lambda));

	// Find the Equation of Time
	// in minutes
	// Correction suggested by David Smith
	LL = L - alpha;
	if (L < pi)
		LL += 2.0 * pi;
	equation = 1440.0 * (1.0 - LL / pi / 2.0);
	ha = f0(latit, delta);
	hb = f1(latit, delta);
	twx = hb - ha; // length of twilight in radians
	twx = 12.0 * twx / pi; // length of twilight in hours
	riset = 12.0 - 12.0 * ha / pi + tzone - longit / 15.0 + equation / 60.0;
	settm = 12.0 + 12.0 * ha / pi + tzone - longit / 15.0 + equation / 60.0;

	twam = riset - twx; // morning twilight begin
	twpm = settm + twx; // evening twilight end

	if (riset > 24.0)
		riset -= 24.0;
	if (settm > 24.0)
		settm -= 24.0;

	printf("=============================================\n");
	printf("[ Current Day ] : %04d-%02d-%02d\n", (int)y, (int)m, (int)day);
	printf("[ Light Mode  ] : %s\n", LMODE_STR(light.mode));
	printf("[ Rise Offset ] : %02dmin\n", light.onOfs);
	printf("[ Set Offset  ] : %02dmin\n", light.offOfs);

	sunriseTime_t rise, set, twrise, twset;

	gethrmn(riset, &rise);
	gethrmn(settm, &set);
	gethrmn(twam, &twrise);
	gethrmn(twpm, &twset);

	if (light.mode == LMODE_INTERNAL) {
		printf("[ Latitude    ] : %3.1f\n", (float)latit);
		printf("[ longitude   ] : %3.1f\n", (float)longit);
		printf("[ TimeBase    ] : %s\n", TIMEBASE_STR(light.timeBase));
		if (light.timeBase == SUNRISE_BASE) {
			addOffset(&rise, &set);
			light.onTime[HOUR] = (uint8)set.hour;
			light.onTime[MINUTE] = (uint8)set.min;
			light.offTime[HOUR] = (uint8)rise.hour;
			light.offTime[MINUTE] = (uint8)rise.min;
		} else if (light.timeBase == CIVIL_BASE) {
			addOffset(&twrise, &twset);
			light.onTime[HOUR] = (uint8)twset.hour;
			light.onTime[MINUTE] = (uint8)twset.min;
			light.offTime[HOUR] = (uint8)twrise.hour;
			light.offTime[MINUTE] = (uint8)twrise.min;
		}
		printf("[ Rise Time   ] : %02d:%02d\n", light.offTime[HOUR], light.offTime[MINUTE]);
		printf("[ Set Time    ] : %02d:%02d\n", light.onTime[HOUR], light.onTime[MINUTE]);
	} else if (light.mode == LMODE_SETTING) {
		addOffset((sunriseTime_t *)light.setOnTime, (sunriseTime_t *)light.setOffTime);
		printf("[ Rise Time   ] : %02d:%02d\n", light.setOnTime[HOUR],
		       light.setOnTime[MINUTE]);
		printf("[ Set Time    ] : %02d:%02d\n", light.setOffTime[HOUR],
		       light.setOffTime[MINUTE]);
	}

	printf("=============================================\n");
}

BOOL updateSunTime(Date_t *date)
{
	static uint8 prevDay = 0;
	static light_t prevSetting;
	BOOL retVal = FALSE;
	BOOL needChange = TRUE;

	if (prevDay != date->day) {
		printf_ts("Day Changed prev(%d) current(%d)\n", prevDay, date->day);
	} else if (memcmp(&prevSetting, &light, sizeof(light_t))) {
		printf_ts("Light Setting Changed\n");
	} else {
		needChange = FALSE;
	}

	if (needChange) {
		calcSunrise(date);
		prevDay = date->day;
		memcpy(&prevSetting, &light, sizeof(light_t));
		retVal = TRUE;
	}

	return retVal;
}
