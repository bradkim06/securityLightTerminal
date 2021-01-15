#include "rtc.h"
#include "device.h"
#include "misc.h"

static int setYEAR(int year)
{
	RTCYEAR = year;
	return RTCYEAR;
}

static int setMON(int month)
{
	RTCMON = month;
	return RTCMON;
}

static int setDAY(int day)
{
	RTCDAY = day;
	return RTCDAY;
}

// static int setDOW(int dow)
// {
//     RTCDOW = dow;
//     return RTCDOW;
// }

static int setHOUR(int hour)
{
	RTCHOUR = hour;
	return RTCHOUR;
}

static int setMIN(int min)
{
	RTCMIN = min;
	return RTCMIN;
}

static int setSEC(int sec)
{
	RTCSEC = sec;
	return RTCSEC;
}

// static int getTIM0(void)
// {
//     return RTCTIM0;
// }

// static int getTIM1(void)
// {
//     return RTCTIM1;
// }

// static int getDATE(void)
// {
//     return RTCDATE;
// }

static int getYEAR(void)
{
	return RTCYEAR;
}

static int getMON(void)
{
	return RTCMON;
}

// static int getDOW(void)
// {
//     return RTCDOW;
// }

static int getDAY(void)
{
	return RTCDAY;
}

static int getHOUR(void)
{
	return RTCHOUR;
}

static int getMIN(void)
{
	return RTCMIN;
}

static int getSEC(void)
{
	return RTCSEC;
}

void RTC_read(struct Time *date)
{
	date->year = getYEAR();
	date->mon = getMON();
	date->day = getDAY();
	date->hour = getHOUR();
	date->min = getMIN();
	date->sec = getSEC();
}

void RTC_set(struct Time *date)
{
	if (MISC_isValidDate(date)) {
		RTCCTL01 |= RTCMODE;
		RTCCTL01 &= ~(RTCHOLD);

		setYEAR(date->year);
		setMON(date->mon);
		setDAY(date->day);
		setHOUR(date->hour);
		setMIN(date->min);
		setSEC(date->sec);
	}
}

/**
 * @brief Set RTC alarm
 *
 * @param day   alarm day, '0xFF' is each day
 * @param hour  alarm hour, '0xFF' is each hour
 * @param min   alarm min, '0xFF' is 00
 * @example
 *      every day, 12:00 alarm : RTC_setAlarm(0xFF, 12, 0xFF)
 *      every month, 1st 15:30 alarm : RTC_setAlarm(1, 15, 30)
 */
void RTC_alarm(uint8 day, uint8 hour, uint8 min)
{
	uint16 minhr = 0;
	uint16 dowday = 0;

	// clearing and disable alarm
	RTCCTL01 &= ~(RTCAIE + RTCAIFG);
	RTCAMINHR = 0x0000;
	RTCADOWDAY = 0x0000;

	if (hour != 0xFF) {
		minhr |= (0x8000 | hour << 8); // alarm enable bit ON
	}

	if (min != 0xFF) {
		minhr |= (0x0080 | min);
	}

	if (day != 0xFF) {
		dowday |= (0x8000 | day << 8);
	}

	RTCCTL01 |= RTCAIE;
	RTCAMINHR = minhr;
	RTCADOWDAY = dowday;
}

// RTC interrupt service routine
#pragma vector = RTC_VECTOR
__interrupt void RTC_ISR(void)
{
	if (RTCIV & RTCAIFG) {
		RTCIV &= ~(RTCAIFG);
		RTC_ISR_HANDLER();
		WAKEUP_DEVICE();
	}
}
