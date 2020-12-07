#ifndef _OSAL_TIMER_H_
#define _OSAL_TIMER_H_

#include "device.h"
#include "var_type.h"

#define TIMER_1SEC     32768
#define TIMER_INTERVAL 25  // msec

#define TIMER_ISR_HANDLER() OSAL_eventHandler()

void   TIMER_init(void);
void   TIMER_start(void);
void   TIMER_stop(void);
uint32 TIMER_getMsec(void);
uint32 TIMER_getMsecDiff(uint32 ms);

#endif  // _OSAL_TIMER_H_
