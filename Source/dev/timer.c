#include "timer.h"

#include "common.h"
#include "device.h"
#include "misc.h"

static uint32 SysTimeMsec = 0;

uint32 TIMER_getMsec(void) {
  return SysTimeMsec;
}

uint32 TIMER_getMsecDiff(uint32 ms) {
  uint32 diff = 0;

  if (SysTimeMsec >= ms) {
    diff = SysTimeMsec - ms;
  } else {
    diff = 0xffffffff - ms + SysTimeMsec;
  }

  return diff;
}

void TIMER_init(void) {
  //	TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA1CCR0 = TIMER_1SEC / (1000 / TIMER_INTERVAL);
  TA1CTL  = TASSEL_1 + MC_1 + TACLR;  // ACLK(32Khaz), Up mode, clear TAR

  SysTimeMsec = 0;
#if 0
    TA0R = 0;
    TA0CCR0 = TIMER_1SEC;
    TA0CTL = TASSEL_1 + MC_1 + TACLR;         // ACLK(32Khaz), Up mode, clear TAR
#endif
}

void TIMER_start(void) {
  TA1CCTL0 = CCIE;  // CCR0 interrupt enabled
}

void TIMER_stop(void) {
  TA1CCTL0 &= ~CCIE;  // CCR0 interrupt disable
}

// Timer1 A0 interrupt service routine
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void) {
  SysTimeMsec += TIMER_INTERVAL;
  TA1R    = 0;
  TA1CCR0 = TIMER_1SEC / (1000 / TIMER_INTERVAL);

  halIntState_t intState;
  HAL_ENTER_CRITICAL_SECTION(intState);  // Hold off interrupts
  TIMER_ISR_HANDLER();
  HAL_EXIT_CRITICAL_SECTION(intState);  // Release interrupts
}
