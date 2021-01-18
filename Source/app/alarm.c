#include "common.h"
#include "device.h"
#include "misc.h"

#include "alarm.h"
#include "app.h"
#include "metering.h"
#include "modem.h"

#include "rtc.h"
#include "uart.h"

// 정의된 RTC 타이머 이벤트 개수.
#if REPEATER_DEVICE
#define RTC_ALARM_COUNT 1
#else
#define RTC_ALARM_COUNT 3
#endif

#define ALARM_TYPE_STR(x)                                                      \
	(((x) == RTC_ALARM_METERING) ?                                         \
		 "metering" :                                                  \
		 ((x) == RTC_ALARM_REPORT) ?                                   \
		 "report" :                                                    \
		 ((x) == RTC_ALARM_METER_SAVE) ? "meter save" : "unknown")

static int TimeSyncFlag = 0;
static Date_t TimeSyncDate;

static RtcAlarm_t AlarmList[RTC_ALARM_COUNT];
static RtcAlarm_t *NextAlarm = NULL;

static void setDefaultRTC(Date_t *date)
{
	date->year = YEAR_MIN;
	date->mon = 1;
	date->day = 1;
	date->hour = 0;
	date->min = 0;
	date->sec = 0;
}

void ALARM_init()
{
	Date_t date;
	RTC_read(&date);

	do {
		if (MISC_isValidDate(
			    &date)) { // CPU 내의 RTC register가 유효한 값.
			break;
		}

		setDefaultRTC(&date); // default 시간 설정
	} while (0);

	RTC_set(&date);

	// 보조중계기 및 분단위 검침 시 alaram은 1개만 metering type 및 report min으로
	// 등록. report min에 맞춰 검침 및 보고를 수행. (분단위 동작은
	// shortScheduler() 참조)
	if (conf.isShortInterval) {
		AlarmList[0].type = RTC_ALARM_METERING;
		AlarmList[0].minute = conf.reportMin;
#if (REPEATER_DEVICE == 0)
		AlarmList[1].type = RTC_ALARM_UNKNOWN;
		AlarmList[1].minute = 0;
		AlarmList[2].type = RTC_ALARM_UNKNOWN;
		AlarmList[2].minute = 0;
#endif
	} else {
#if REPEATER_DEVICE
		AlarmList[0].type = RTC_ALARM_METERING;
		AlarmList[0].minute = conf.reportMin;
#else
		AlarmList[0].type = RTC_ALARM_METERING;
		AlarmList[0].minute = 0;
		if (conf.reportMin > 30) {
			AlarmList[1].type = RTC_ALARM_METER_SAVE;
			AlarmList[1].minute = 15;
			AlarmList[2].type = RTC_ALARM_REPORT;
			AlarmList[2].minute = conf.reportMin;
		} else {
			AlarmList[1].type = RTC_ALARM_REPORT;
			AlarmList[1].minute = conf.reportMin;
			AlarmList[2].type = RTC_ALARM_METER_SAVE;
			AlarmList[2].minute = 45;
		}
#endif
	}

	TimeSyncFlag = 0;
	memset(&TimeSyncDate, 0, sizeof(Date_t));
}

int ALARM_isTimeSync()
{
	return TimeSyncFlag;
}

BOOL ALARM_needTimeSync()
{
	// LORA에서는 날자가 바뀌면 새로운 time sync를 실행함.

	Date_t date;
	RTC_read(&date);

	if (TimeSyncFlag && (TimeSyncDate.day == date.day)) {
		return FALSE;
	}

	return TRUE;
}

int ALARM_writeTime(Date_t *date)
{
	if (MISC_isValidDate(date)) {
		TimeSyncFlag = 1;
		RTC_set(date);
		memcpy(&TimeSyncDate, date, sizeof(Date_t));
		return 1;
	}
	return 0;
}

void shortScheduler()
{
	Date_t date;
	RTC_read(&date);

	// 현재시간이 55초를 초과할 경우 분으로 올림하여 계산.
	if (date.sec > 55) {
		date.min++;
	}
	// 분단위 검침 시 Alarm list[0]만 사용하며, minute을 기반으로 고정된 시간에
	// 수행하도록 schduling한다. ex) AlarmList[0].minute = 8, interval = 3 >>
	// alarm set = 8, 11, 14, 17, ..., 59 int ri   = conf.reportInterval;
	int ri = conf.rtCtrlInterval;
	int hour = date.hour;
	int min = date.min + ri - ((60 + date.min - AlarmList[0].minute) % ri);
	if (min >= 60) {
		min -= 60;
		if (++hour >= 24) {
			hour = 0;
		}
	}

	RTC_alarm(0xFF, hour, min);
	NextAlarm = &AlarmList[0];

	printf_ts("set next alarm [%02d:%02d:00] - %s\n\n", hour, min,
		  ALARM_TYPE_STR(NextAlarm->type));
}

void normalScheduler()
{
	Date_t date;
	RTC_read(&date);

	// 시간 동기화에 의한 알람 중복 등록을 피하기 위해 이전 알람 이벤트에 의한
	// 순차 등록으로 수정. 단, 시간 동기화 후 발생 오차에 의한 문제를 방지하기
	// 위해 아래와 같은 기준으로 알람을 설정.
	// 1. 알람 발생 후 완료까지 시간 5분.
	// 2. 알람 동작 완료 후 Idle 시간 5분.
	// 3. 시간 오차를 고려하여 실제 알람 시간의 +-1분 오차를 고려.

	int diff, lastDiff = 60;
	RtcAlarm_t *pNext = NULL;
	for (int idx = 0; idx < RTC_ALARM_COUNT; idx++) {
		diff = AlarmList[idx].minute - date.min;
		if (diff <= 0) {
			// diff 값이 음수이거나 차이가 없는 경우 다음 시간에 event를 발생시키기
			// 위해 1시간을 더한다.
			diff += 60;
		}

		printf_ts("set next alarm [%10s:%02dm diff=%02dm] - ",
			  ALARM_TYPE_STR(AlarmList[idx].type),
			  AlarmList[idx].minute, diff);

		// diff 값이 0이거나 1인 알람을 설정할 경우 알람이 발생하지 않을 수 있으므로
		// 제외한다.
		if (diff > 1) {
			if (diff <= lastDiff) {
				pNext = &AlarmList[idx];
				lastDiff = diff;

				printf("change\n");
			} else {
				printf("skip\n");
			}
		} else {
			printf("diff is too small \n");
		}
	}

	// 검색된 next alarm이 없는 경우 첫번째 알람을 설정한다.
	if (pNext) {
		NextAlarm = pNext;
	} else {
		printf_ts("set next alarm - set fail, set first alarm \n");
		NextAlarm = &AlarmList[0];
	}

	int hour = NextAlarm->minute <= date.min ? ((date.hour + 1) % 24) :
						   date.hour;
	int min = NextAlarm->minute;
	RTC_alarm(0xFF, hour, min);

	printf_ts("set next alarm [%02d:%02d:00] - %s\n", hour, min,
		  ALARM_TYPE_STR(NextAlarm->type));
}

void ALARM_schedule()
{
	if (conf.isShortInterval) {
		shortScheduler();
	} else {
		normalScheduler();
	}
}

void ALARM_run()
{
	/* Comment check sleepMode ALARM_run */
	/* if (conf.sleepMode || NextAlarm == NULL) { */
	/* 	APP_prepareToSleep(); */
	/* 	return; */
	/* } */

	printf_ts("alarm occurred - '%s'\n", ALARM_TYPE_STR(NextAlarm->type));
	if (conf.isShortInterval) {
		APP_runMetering(PERIODIC_METERING);
		return;
	}

	Date_t date;
	RTC_read(&date);

	RtcAlarmType_t alarmType = NextAlarm->type;
#if REPEATER_DEVICE
	if (alarmType == RTC_ALARM_METERING) {
		if (MISC_checkInterval(date.hour, conf.reportHour,
				       conf.rtCtrlInterval)) {
			APP_runMetering(PERIODIC_METERING);
		}
		/* Comment APP_prepareToSleep */
		/* else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	}
#else
	if (alarmType == RTC_ALARM_REPORT) {
		if (MISC_checkInterval(date.hour, conf.reportHour,
				       conf.rtCtrlInterval)) {
			APP_runPeriodicReport();
		}
		/* Comment APP_prepareToSleep */
		/* else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	} else if (alarmType == RTC_ALARM_METERING) {
		if (MISC_checkInterval(date.hour, conf.reportHour,
				       conf.meterInterval)) {
			APP_runMetering(PERIODIC_METERING);
		}
		/* Comment APP_prepareToSleep */
		/* } else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	} else if (alarmType == RTC_ALARM_METER_SAVE) {
		if (ALARM_isTimeSync()) {
			// 기간검침 데이터 저장 Base Time을 "통신 Base Time + 1시간" 으로 설정.
			// 해당 이유는 LoRa 통신 문제 발생 시 Join시도를 1시간동안 하므로
			// 경우에 따라 0시에 기간검침 데이터를 저장하지 않는 문제가 발생하므로
			// 수정.
			if ((date.hour % METER_DATA_SAVE_INTERVAL) ==
			    (conf.reportHour + 1)) {
				APP_runMetering(PERIODIC_METERING_FOR_SAVE);
			}
			/* Comment APP_prepareToSleep */
			/* else { */
			/* 	APP_prepareToSleep(); */
			/* } */
		}
		/* Comment APP_prepareToSleep */
		/* } else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	}
#endif
	else {
		printf_ts("unknow alarm type(%d)\n", alarmType);
		/* Comment APP_prepareToSleep */
		/* APP_prepareToSleep(); */
	}
}

void ALARM_isr()
{
	OSAL_setEvent(APP_getTaskId(), APP_EVENT_RTC_ALARM);
}
