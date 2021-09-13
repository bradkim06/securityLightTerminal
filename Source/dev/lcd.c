#include "lcd.h"
#include "device.h"
#include "metering.h"

void initButton(void)
{
#define ALL_BUTTON 0xF0

	P2DIR &= ~ALL_BUTTON; // Set to Input
	P2REN &= ~ALL_BUTTON; // Input

	P2IES &= ALL_BUTTON; // Low ->Hi edge
	P2IFG &= ~ALL_BUTTON; // IFG cleared
	P2IE |= ALL_BUTTON; // Interrupt enabled
}

/* Port 2 interrupt service routine - Button */
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
	static uint32 ms = 1000;

	if (TIMER_getMsecDiff(ms) > 500) {
		ms = TIMER_getMsec();
	} else {
		P2IFG &= ~0xF0;
	}

	if (P2IFG & BM(BUTTON_ENTER)) {
		P2IFG &= ~BM(BUTTON_ENTER);
		LCD_sendReq(BUTTON_ENTER);
	}

	if (P2IFG & BM(BUTTON_STOP)) {
		P2IFG &= ~BM(BUTTON_STOP);
		LCD_sendReq(BUTTON_STOP);
	}

	if (P2IFG & BM(BUTTON_UP)) {
		P2IFG &= ~BM(BUTTON_UP);
		LCD_sendReq(BUTTON_UP);
	}

	if (P2IFG & BM(BUTTON_DOWN)) {
		P2IFG &= ~BM(BUTTON_DOWN);
		LCD_sendReq(BUTTON_DOWN);
	}
}
