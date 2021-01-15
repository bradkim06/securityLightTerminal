#ifndef __RTC_ALARM_H__
#define __RTC_ALARM_H__

#include "var_type.h"

// RTC 타이머 이벤트(= Alarm)
typedef enum {
	RTC_ALARM_METERING, // 매 시간 0분에 발생하는 검침 이벤트
	RTC_ALARM_REPORT, // 매 시간 정해진 시간(serial number에 따라 변경)에
	// 발생하는 보고 이벤트
	RTC_ALARM_METER_SAVE, // 매 시간 15분 or 45분에 발생하는 검침 데이터 저장
	// 이벤트 (REPORT 시간에따라 결정됨.)
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
int ALARM_writeTime(Date_t *date);
int ALARM_isTimeSync();
BOOL ALARM_needTimeSync();

#endif // __RTC_ALARM_H__
