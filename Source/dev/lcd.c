#include "lcd.h"
#include "device.h"
#include "metering.h"

void initButton(void)
{
	// P2.2 DC_POWER_FAULT Input
#define DC_PWR_FLT_PIN 2
	P2DIR &= ~BM(DC_PWR_FLT_PIN);
	P2REN &= ~BM(DC_PWR_FLT_PIN);

	P2IES |= BM(DC_PWR_FLT_PIN); // Hi -> Low edge
	P2IFG &= ~BM(DC_PWR_FLT_PIN); // IFG cleared
	P2IE |= BM(DC_PWR_FLT_PIN); // Interrupt enabled

#define ALL_BUTTON 0xF0
	P2DIR &= ~ALL_BUTTON; // Set to Input
	P2REN &= ~ALL_BUTTON; // Input

	P2IES |= ALL_BUTTON; // Hi -> Low edge
	P2IFG &= ~ALL_BUTTON; // IFG cleared
	P2IE |= ALL_BUTTON; // Interrupt enabled
}

/* Port 2 interrupt service routine - Button */
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
// DC Fault Interrupt
#define DC_PWR_FLT_PIN 2
	if (P2IFG & BM(DC_PWR_FLT_PIN)) {
		keepReportType = PERIODIC_METERING;
		keepCauseType = AUTO;
		OSAL_setEvent(AppTaskId, APP_EVENT_LIGHT_REPORT);
	}

	static uint32 ms = 1000;
	// Button
	if (TIMER_getMsecDiff(ms) > 500) {
		ms = TIMER_getMsec();
	} else {
		P2IFG &= ~0xF0;
	}

	if (P2IFG & BM(BUTTON_ENTER)) {
		LCD_sendReq(BUTTON_ENTER);
	}

	if (P2IFG & BM(BUTTON_STOP)) {
		LCD_sendReq(BUTTON_STOP);
	}

	if (P2IFG & BM(BUTTON_UP)) {
		LCD_sendReq(BUTTON_UP);
	}

	if (P2IFG & BM(BUTTON_DOWN)) {
		LCD_sendReq(BUTTON_DOWN);
	}

	P2IFG = 0;
}
