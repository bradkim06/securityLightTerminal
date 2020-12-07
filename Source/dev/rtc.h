#ifndef RTC_H_
#define RTC_H_

#include "alarm.h"
#include "var_type.h"

#define YEAR_MIN 2017
#define YEAR_MAX 2100

struct Time {
  uint16 year;  // yearH, yearL
  uint8  mon;
  uint8  day;
  uint8  hour;
  uint8  min;
  uint8  sec;
  uint8  reserved;
};

#define RTC_ISR_HANDLER() ALARM_isr()
// struct Time { int Year, Month, DayOfWeek, Day, Hour, Minute, Second; };

void RTC_read(struct Time* date);
void RTC_set(struct Time* date);
void RTC_alarm(uint8 day, uint8 hour, uint8 min);

#endif /*RTC_H_*/
