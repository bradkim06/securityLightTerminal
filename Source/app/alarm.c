#include "common.h"
#include "device.h"
#include "misc.h"

#include "alarm.h"
#include "app.h"
#include "metering.h"
#include "modem.h"

#include "rtc.h"
#include "uart.h"
#include "sunrise.h"

// 정의된 RTC 타이머 이벤트 개수.
#define RTC_ALARM_COUNT 5

#define ALARM_TYPE_STR(x)                                                                          \
	(((x) == RTC_ALARM_LIGHT_ON) ?                                                             \
		 "Light on" :                                                                      \
		 ((x) == RTC_ALARM_LIGHT_OFF) ?                                                    \
		 "Light off" :                                                                     \
		 ((x) == RTC_ALARM_PSM_ON) ?                                                       \
		 "Psm on" :                                                                        \
		 ((x) == RTC_ALARM_PSM_OFF) ? "Psm off" :                                          \
					      ((x) == RTC_ALARM_REPORT) ? "Report" : "unknown")

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
		// CPU 내의 RTC register가 유효한 값.
		if (MISC_isValidDate(&date)) {
			break;
		}
		// default 시간 설정
		setDefaultRTC(&date);
	} while (0);

	RTC_set(&date);
	if (conf.isShortInterval) {
		AlarmList[0].type = RTC_ALARM_REPORT;
		AlarmList[0].minute = conf.reportMin;
	} else {
		AlarmList[0].type = RTC_ALARM_LIGHT_ON;
		AlarmList[0].minute = light.onTime[MINUTE];
		AlarmList[1].type = RTC_ALARM_LIGHT_OFF;
		AlarmList[1].minute = light.offTime[MINUTE];

		if (light.psm) {
			AlarmList[2].type = RTC_ALARM_PSM_ON;
			AlarmList[2].minute = light.psmOnTime[MINUTE];
			if (light.psm == PSM_RELIGHT) {
				AlarmList[3].type = RTC_ALARM_PSM_OFF;
				AlarmList[3].minute = light.psmOffTime[MINUTE];
			}
		}

		AlarmList[4].type = RTC_ALARM_REPORT;
		AlarmList[4].minute = conf.reportMin;
	}

	TimeSyncFlag = 0;
	memset(&TimeSyncDate, 0, sizeof(Date_t));
}

int ALARM_isTimeSync()
{
	return TimeSyncFlag;
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
	int ri = conf.reportInterval;
	int hour = date.hour;
	// int min = date.min + ri - ((60 + date.min - AlarmList[0].minute) % ri);
	int min = date.min + ri;
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

static BOOL checkLightAlarm(Date_t *date, uint8 time[], int idx)
{
	/* int hourdiff, mindiff; */
	BOOL retVal = FALSE;

	uint8 hour = time[HOUR];

	printf_ts("%s - [%02d:%02d:00] ", ALARM_TYPE_STR(AlarmList[idx].type), hour, time[MINUTE]);

	if (hour == 0) {
		hour = 24;
	}

	int diffHour = hour - date->hour;

	if (diffHour < 0) {
		diffHour += 24;
	}

	printf("diffhour : %d\n", diffHour);

	if (diffHour == 0) {
		/* mindiff = time[MINUTE] - date->min; */

		if ((time[MINUTE] - date->min) > 0) {
			retVal = TRUE;
		}
	} else if (diffHour == 1) {
		retVal = TRUE;
	} else {
		retVal = FALSE;
	}

	return retVal;
}

static BOOL checkPsmDate(Date_t *date)
{
	BOOL retVal = FALSE;

	if ((date->mon >= light.psmOnDay[MONTH]) && (date->mon <= light.psmOffDay[MONTH])) {
		if ((date->day >= light.psmOnDay[DAY]) && (date->day <= light.psmOffDay[DAY])) {
			retVal = TRUE;
		}
	}

	return retVal;
}

void normalScheduler()
{
	Date_t date;
	RTC_read(&date);

	if (TimeSyncFlag) {
		updateSunTime(&date);

		AlarmList[0].minute = light.onTime[MINUTE];
		AlarmList[1].minute = light.offTime[MINUTE];

		if (light.psm) {
			AlarmList[2].type = RTC_ALARM_PSM_ON;
			AlarmList[2].minute = light.psmOnTime[MINUTE];
			if (light.psm == PSM_RELIGHT) {
				AlarmList[3].type = RTC_ALARM_PSM_OFF;
				AlarmList[3].minute = light.psmOffTime[MINUTE];
			}
		}
	}

	// 시간 동기화에 의한 알람 중복 등록을 피하기 위해 이전 알람 이벤트에 의한 순차 등록으로 수정.
	// 단, 시간 동기화 후 발생 오차에 의한 문제를 방지하기 위해 아래와 같은 기준으로 알람을 설정.
	// 1. 알람 발생 후 완료까지 시간 5분.
	// 2. 알람 동작 완료 후 Idle 시간 5분.
	// 3. 시간 오차를 고려하여 실제 알람 시간의 +-1분 오차를 고려.

	int diff, lastDiff = 60;
	BOOL isLight = 0;
	RtcAlarm_t *pNext = NULL;
	for (int idx = 0; idx < RTC_ALARM_COUNT; idx++) {
		if (TimeSyncFlag) {
			if (idx == RTC_ALARM_LIGHT_ON) {
				if (checkLightAlarm(&date, light.onTime, idx)) {
					isLight = TRUE;
				}
			} else if (idx == RTC_ALARM_LIGHT_OFF) {
				if (checkLightAlarm(&date, light.offTime, idx)) {
					isLight = TRUE;
				}
			}

			if (light.psm) {
				if (checkPsmDate(&date)) {
					if (idx == RTC_ALARM_PSM_ON) {
						if (checkLightAlarm(&date, light.psmOnTime, idx)) {
							isLight = TRUE;
						}
					}

					if (light.psm == PSM_RELIGHT) {
						if (idx == RTC_ALARM_PSM_OFF) {
							if (checkLightAlarm(&date, light.psmOffTime,
									    idx)) {
								isLight = TRUE;
							}
						}
					}
				}
			}

			if (isLight) {
				pNext = &AlarmList[idx];
				break;
			} else if (idx != RTC_ALARM_REPORT) {
				continue;
			}

			diff = AlarmList[idx].minute - date.min;

			if (diff < 1) {
				// diff 값이 음수이거나 차이가 없는 경우 다음 시간에 event를 발생시키기 위해 1시간을 더한다.
				diff += 60;
			}

			// diff 값이 0이거나 1인 알람을 설정할 경우 알람이 발생하지 않을 수 있으므로 제외한다.
			if (diff > 1) {
				if (diff <= lastDiff) {
					pNext = &AlarmList[idx];
					lastDiff = diff;
				}
			}
		}
	}

	// 검색된 next alarm이 없는 경우 첫번째 알람을 설정한다.
	if (pNext) {
		NextAlarm = pNext;
	} else {
		printf_ts("set next alarm - set fail, set first alarm \n");
		NextAlarm = &AlarmList[RTC_ALARM_REPORT];
	}

	int hour = NextAlarm->minute <= date.min ? ((date.hour + 1) % 24) : date.hour;
	int min = NextAlarm->minute;
	RTC_alarm(0xFF, hour, min);

	printf_ts("set next alarm [%02d:%02d:00] - %s\n", hour, min,
		  ALARM_TYPE_STR(NextAlarm->type));
}

int ALARM_status()
{
	if (NextAlarm == NULL) {
		return 0;
	}
	return 1;
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
	printf_ts("alarm occurred - '%s'\n", ALARM_TYPE_STR(NextAlarm->type));

	if (conf.isShortInterval) {
		APP_runPeriodicReport();
		return;
	}

	if (MODEM_getState() != DEVICE_STATUS_IDLE) {
		printf_ts("modem status : %d\n", MODEM_getState());
		ALARM_schedule();
		return;
	}

	Date_t date;
	RTC_read(&date);

	RtcAlarmType_t alarmType = NextAlarm->type;
	if (alarmType == RTC_ALARM_REPORT) {
		if (MISC_checkInterval(date.hour, conf.reportHour, conf.reportInterval)) {
			APP_runPeriodicReport();
		}
	} else if (alarmType == RTC_ALARM_LIGHT_ON) {
		if (light.onTime[HOUR] == date.hour) {
			Relay_Status(RELAY_ON, LIGHT_ON, AUTO);
		}
	} else if (alarmType == RTC_ALARM_LIGHT_OFF) {
		if (light.offTime[HOUR] == date.hour) {
			Relay_Status(RELAY_OFF, LIGHT_OFF, AUTO);
		}
	} else if (alarmType == RTC_ALARM_PSM_ON) {
		if (light.psmOnTime[HOUR] == date.hour) {
			Relay_Status(RELAY_OFF, LIGHT_OFF, AUTO);
		}
	} else if (alarmType == RTC_ALARM_PSM_OFF) {
		if (light.psmOffTime[HOUR] == date.hour) {
			Relay_Status(RELAY_ON, LIGHT_ON, AUTO);
		}
	} else {
		printf_ts("unknow alarm type(%d)\n", alarmType);
	}

	ALARM_schedule();
}

void ALARM_isr()
{
	OSAL_setEvent(APP_getTaskId(), APP_EVENT_RTC_ALARM);
}
