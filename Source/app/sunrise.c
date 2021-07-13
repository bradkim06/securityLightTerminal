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

// Display decimal hours in hours and minutes
static void showhrmn(double dhr)
{
	int hr, mn;
	hr = (int)dhr;
	mn = (dhr - (double)hr) * 60;

	printf("%0d:%0d\n", hr, mn);
};

static void gethrmn(double dhr, sunriseTime_t *p)
{
	int hr, mn;
	hr = (int)dhr;
	mn = (dhr - (double)hr) * 60;

	p->hour = hr;
	p->min = mn;
};

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
	float inlat, inlon, intz;
	double tzone, d, lambda;
	double obliq, alpha, delta, LL, equation, ha, hb, twx;
	double twam, noont, settm, riset, twpm;

	struct tm *p;

	rads = pi / 180.0;
	//  get the date and time from the user
	// read system date and extract the year

	// 초광산단서로 7
	// inlat = 37.529215;
	// inlon = 127.169043;
	// 대전시청
	inlat = 36.35061;
	inlon = 127.38477;
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
	noont = riset + 12.0 * ha / pi;

	twam = riset - twx; // morning twilight begin
	twpm = settm + twx; // evening twilight end

	if (riset > 24.0)
		riset -= 24.0;
	if (settm > 24.0)
		settm -= 24.0;

	printf("\n Sunrise and set\n");
	printf("=============================================\n");

	printf("  year  : %d", (int)y);
	printf("  month : %d", (int)m);
	printf("  day   : %d \n\n", (int)day);

	sunriseTime_t rise, set, twrise, twset;

	gethrmn(riset, &rise);
	gethrmn(settm, &set);
	gethrmn(twam, &twrise);
	gethrmn(twpm, &twset);

	printf("Latitude :  %3.1f, longitude: %3.1f, timezone: %3.1f \n", (float)latit,
	       (float)longit, (float)tzone);
	printf("Sunrise       : %0d:%0d\n", rise.hour, rise.min);
	printf("Civil twilight: %0d:%0d\n", twrise.hour, twrise.min);
	printf("Sunset        : %0d:%0d\n", set.hour, set.min);
	printf("Civil twilight: %0d:%0d\n", twset.hour, twset.min);
	printf("=============================================\n");

	conf.lightOnHour = (uint8)rise.hour;
	conf.lightOnMin = (uint8)rise.min;
	conf.lightOffHour = (uint8)set.hour;
	conf.lightOffMin = (uint8)set.min;
}

BOOL updateSunTime(Date_t *date)
{
	static uint8 prevDay = 0;
	BOOL retVal = FALSE;

	if (prevDay != date->day) {
		prevDay = date->day;
		calcSunrise(date);
		retVal = TRUE;
	}

	return retVal;
}
