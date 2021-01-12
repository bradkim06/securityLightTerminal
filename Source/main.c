#include "common.h"
#include "device.h"
#include "misc.h"

#include "battery.h"
#include "lcd.h"
#include "metering.h"
#include "modem.h"
#include "osal.h"
#include "shell.h"
#include "task_mgr.h"
#include "test.h"
#include "timer.h"
#include "uart.h"

#if REPEATER_DEVICE
#include "cc1200.h"
#endif

BOOL DeviceSleepState;

static BOOL IsTestModeOn = FALSE;
void initClock(void)
{
	UCSCTL3 |= SELREF_2; // Set DCO FLL reference = REFO
	P7SEL |= 0x03; // Set XIN/XOUT(P7.0, P7.1)
	UCSCTL6 &= ~(XT1OFF); // XT1 On
	UCSCTL6 |= XCAP_3; // Internal load cap

	UCSCTL4 &= 0xF8FF;
	UCSCTL4 |= SELA_0; // Set ACLK = XTCLK

	__bis_SR_register(SCG0); // Disable the FLL control loop
	UCSCTL0 = 0x0000; // Set lowest possible DCOx, MODx
	UCSCTL1 = DCORSEL_6; // Select DCO range 8MHz operation
	UCSCTL2 = FLLD_1 + 249; // Set DCO Multiplier for 8MHz
		// (N + 1) * FLLRef = Fdco
		// (249 + 1) * 32768 = 8MHz
		// Set FLL Div = fDCOCLK/2
	__bic_SR_register(SCG0); // Enable the FLL control loop

	// Worst-case settling time for the DCO when the DCO range bits have been
	// changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
	// UG for optimization.
	// 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
	__delay_cycles(250000);

	// Loop until XT1,XT2 & DCO fault flag is cleared

	BOOL IsSetInternalClock = TRUE;
	for (int loop = 0; loop < 10000; loop++) {
		__delay_cycles(2500);
		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
		SFRIFG1 &= ~OFIFG; // Clear fault flags
		if ((SFRIFG1 & OFIFG) == 0) {
			IsSetInternalClock = FALSE;
			break;
		}
	}

	// If "ACLK = XTCLK" setting failed, set "ACLK = REFO"
	// XTCLK(= External OSC 32KHz), REFO(=Internal OSC 32KHz)
	if (IsSetInternalClock) {
		UCSCTL4 |= SELA_2; // Set ACLK = REFO
		P7SEL &= ~(0x03); // Clear XIN/XOUT(P7.0, P7.1)
	} else {
		// OSC fault, Vacant memory access, NMI pin interrupt,
		SFRIE1 |= (OFIE + VMAIE + NMIIE);
	}

	RTCCTL2 |= 0x80;
	RTCCTL01 |= RTCMODE;
}

void initPort()
{
#if LORA_DEVICE
	P1SEL = 0x00;
	P1DIR = 0xFF;
	P1OUT = 0x00;

	// P2.5 (BOOT0)  : Module BOOT0 pin, Input
	// P2.3 (NRESET) : Module nReset pin, output high
	// P2.0 (VCC)    : change power on
	P2SEL = 0x00;
	P2DIR = (0xFF & ~BM(MODEM_LORA_BOOT0_P2_PIN));
	P2OUT = BM(MODEM_NRST_P2_PIN);
#else
	P1SEL = 0x00;
	P1DIR = 0xFF;
#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_LDO)
	P1OUT = BM(MODEM_VCC2_P1_PIN); // P1.1 (MOSFET_EN) : power off Modem
#else
	P1OUT = 0x00; // All set output low
#endif

	P2SEL = 0x00;
	P2DIR = 0xFF;
#if (DEVICE_REVISION == DEV_REV_PWRCTRL_MOSFET_ONLY)
	// P2.5 (MCU_RESET) : NC 되었으므로 ouput low
	// P2.3 (NRESET)    : NC 되었으므로 ouput low
	// P2.0 (LDO_EN)    : power off Modem (output high)
	P2OUT = BM(MODEM_VCC1_P2_PIN);
#else
	// P2.5 (MCU_RESET) : NC 되었으므로 ouput low
	// P2.3 (NRESET)    : NC 되었으므로 ouput low
	// P2.0 (LDO_EN)    : power off Modem (output low)
	P2OUT = 0x00;
#endif
#endif
	P3SEL = 0x00;
	P3DIR = 0xFF;
	P3OUT = 0x00;

	P4SEL = 0x00;
	P4DIR = 0xFF;
	P4OUT = 0x00;

#if REPEATER_DEVICE
	// 초기 CC1200 power off
	P4SEL &= ~BM(CC1200_VCC_P4_PIN);
	P4DIR |= BM(CC1200_VCC_P4_PIN);
	P4OUT |= BM(CC1200_VCC_P4_PIN);
#endif

	P5SEL = 0x00;
	P5DIR = 0xFF;
	P5OUT = 0x00;

	P6SEL = 0x00;
	P6DIR = 0xFF;
	P6OUT = 0x00;

	P7SEL = 0x00; // XIN, XOUT will be set in initClock
	P7DIR = 0xFF;
	P7OUT = 0x00;

	P8SEL = 0x00;
	P8DIR = 0xFF;
	P8OUT = 0x00;

	P9SEL = 0x00;
	P9DIR = 0xFF;
	P9OUT = 0x00;

	P10SEL = 0x00;
	P10DIR = 0xFF;
	P10OUT = 0x00;

	// P11SEL = 0x00;
	// P11DIR = 0xFF;
	// P11OUT = 0x00;
	// P11 = PF_L 이므로 PF로 초기화하여 PF_L과 PF_H를 함께 초기화.
	PFSEL = 0x00;
	PFDIR = 0xFF;
	PFOUT = 0x00;

	// PJSEL = 0x00;
	PJDIR = 0xFF;
	PJOUT = 0x00;

	// Interrut disable
	P1IE = 0x00;
	P2IE = 0x00;

	P1IFG = 0x00;
	P2IFG = 0x00;

	if (LCD_Exist()) {
		P2DIR &= ~BM(REED_SENSOR_P2_PIN); // Set to Input
		P2IE |= BM(REED_SENSOR_P2_PIN); // Interrupt enabled
		P2IES &= ~BM(REED_SENSOR_P2_PIN); // Low/Hi edge
		P2IFG &= ~BM(REED_SENSOR_P2_PIN); // IFG cleared
	}

	P6DIR |= BM(BATT_ADC_IN_P6_PIN);
	P6REN |= BM(BATT_ADC_IN_P6_PIN);
}

void initSystem()
{
	initPort();
	initClock();

	GLOBAL_ENABLE_INT();

	UART_init(UART_A0, LEN_MAX_LCD_TX, LEN_MAX_LCD_RX);
	UART_init(UART_A1, LEN_MAX_DEBUG_TX, LEN_MAX_DEBUG_RX);
	UART_init(UART_A2, LEN_MAX_MODEM_TX, LEN_MAX_MODEM_RX);
	UART_init(UART_A3, LEN_MAX_METER_TX, LEN_MAX_METER_RX);

	OSAL_init();
	TIMER_init();
	TIMER_start();

#define TEST_WAIT_DELAY (100)
#define TEST_CHECK_COUNT (500 / TEST_WAIT_DELAY)
	for (int i = 0; i < TEST_CHECK_COUNT; i++) {
		IsTestModeOn = TEST_checkTestMode();
		if (IsTestModeOn == FALSE) {
			break;
		}
		MISC_delayMs(TEST_WAIT_DELAY);
	}
}

main()
{
	WDTCTL = WDTPW | WDTCNTCL; // Clear watchdog timer
	WDTCTL = WDT_VRST_3SEC; // Start watchdog timer(3.2768 sec)
	initSystem();

	// 시스템 초기화 및 테스트모드 여부 확인 후 WDT는 16초로 재설정
	// 1. App level의 코드 구동 시 Console 출력이 포함되어 동작 시간이 유동적으로
	// 변함.
	// 2. 시스템 초기화 이후에는 Clock 설정이 완료된 상태이므로 WDT를 재설정해도
	// 무방.

	WDTCTL = WDTPW | WDTCNTCL; // Clear watchdog timer
	WDTCTL = WDT_ARST_16SEC; // Start watchdog timer(16 sec)
	if (IsTestModeOn) {
		TEST_run();
	} else {
		TASKMGR_init();
		TASKMGR_run();
	}
}
