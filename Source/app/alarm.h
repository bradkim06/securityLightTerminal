#ifndef __RTC_ALARM_H__
#define __RTC_ALARM_H__

#include "var_type.h"

// RTC 타이머 이벤트(= Alarm)
typedef enum {
	// 매 시간 정해진 시간(serial number에 따라 변경)에 발생하는 보고 이벤트
	RTC_ALARM_REPORT,
	RTC_ALARM_LIGHT,
	RTC_ALARM_UNKNOWN, // 미사용 알람
} RtcAlarmType_t;

typedef struct {
	uint8 minute;
	RtcAlarmType_t type;
} RtcAlarm_t;

void ALARM_isr();
void ALARM_init();
void ALARM_schedule(void);
void ALARM_run();
int ALARM_status();
int ALARM_writeTime(Date_t *date);
int ALARM_isTimeSync();
BOOL ALARM_needTimeSync();

#endif // __RTC_ALARM_H__
