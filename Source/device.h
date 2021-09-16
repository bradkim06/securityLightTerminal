#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <msp430.h>
#include "timer.h"
#include "uart.h"
#include "var_type.h"

//============================================
// TERMINAL DEVICE COMMON PORT NUMBER
//============================================
#define COMM_STATUS_ON 2
#define COMM_STATUS_OFF 3

#define BUTTON_ENTER 4
#define BUTTON_STOP 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7

#define TEST_MODE_DETECT_P1_PIN 0 // P2.1
#define HTM115W_DETECT_P8_PIN 1 // P8.1
#define LORA_DETECT_P8_PIN 2 // P8.2
#define NBIOT_DETECT_P8_PIN 3 // P8.3
#define RF424_DETECT_P8_PIN 4 // P8.4

//============================================
// Terminal modem power control define
//============================================
#define DEV_REV_PWRCTRL_MOSFET_ONLY 17
#define DEV_REV_PWRCTRL_LDO_ONLY 18
#define DEV_REV_PWRCTRL_MOSFET_LDO 19
#define DEVICE_REVISION DEV_REV_PWRCTRL_MOSFET_LDO

//============================================
// Terminal LCD control define
//============================================
#define LCD_THROUGH_UART 1
#define LCD_THROUGH_GPIO (LCD_THROUGH_UART) ? 0 : 1

#define WDT_ARST_16SEC (WDTPW + WDTCNTCL + WDTSSEL0 + WDTIS1 + WDTIS0) // Use ACLK, 16sec
#define WDT_VRST_3SEC (WDTPW + WDTCNTCL + WDTSSEL1 + WDTIS2) // Use VLOCLK, abour 3~4sec

//============================================
// MCU power control (with LPM)
//============================================
#define POWER_MODE LPM3_bits // RTC 동작시 파워모드
#define POWER_ACTIVE_OFF LPM2_bits // ACTIVE_OFF시 파워모드

#define LPM2_ON()                                                                                  \
	do {                                                                                       \
		__bis_SR_register(POWER_ACTIVE_OFF + GIE);                                         \
	} while (0)
#define LPM2_OFF()                                                                                 \
	do {                                                                                       \
		__bic_SR_register_on_exit(POWER_ACTIVE_OFF);                                       \
	} while (0)

extern void LCD_disable();
extern void PRINT_disable();
extern void MODEM_disable();

extern BOOL DeviceSleepState;

#define WAKEUP_DEVICE()                                                                            \
	do {                                                                                       \
		WDTCTL = WDT_ARST_16SEC;                                                           \
		if (DeviceSleepState == TRUE) {                                                    \
			PRINT_enable();                                                            \
			TIMER_start();                                                             \
			DeviceSleepState = FALSE;                                                  \
			__bic_SR_register_on_exit(POWER_MODE);                                     \
		}                                                                                  \
		__no_operation();                                                                  \
	} while (0)

#define REBOOT_SYSTEM()                                                                            \
	do {                                                                                       \
		MISC_delayMs(100);                                                                 \
		GLOBAL_DISABLE_INT();                                                              \
		SYSTEM_RESET();                                                                    \
		MISC_delayMs(100);                                                                 \
	} while (0)

//============================================
// MCU interrupt macro & define
//============================================
typedef unsigned char halIntState_t;

#define NOP() __no_operation()
#define SYSTEM_RESET()                                                                             \
	do {                                                                                       \
		PMMCTL0 = (PMMPW + PMMSWPOR);                                                      \
	} while (0) /* force immediate system reset */

#define GLOBAL_ENABLE_INT() _EINT()
#define GLOBAL_DISABLE_INT() _DINT()
#define HAL_ENTER_CRITICAL_SECTION(x)                                                              \
	do {                                                                                       \
		x = __get_interrupt_state();                                                       \
		GLOBAL_DISABLE_INT();                                                              \
	} while (0)
#define HAL_EXIT_CRITICAL_SECTION(x)                                                               \
	do {                                                                                       \
		__set_interrupt_state(x);                                                          \
	} while (0)

#endif // #define __DEVICE_H__
