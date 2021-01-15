#ifndef _LCD_DRIVER_H_
#define _LCD_DRIVER_H_

#include "uart.h"
#include "var_type.h"
// MSP430F412를 사용하여 LCD를 Driving 하기 위한 코드

#define LCD_DETECT_P1_PIN 4 // P1.4
#define LCD_GPIO_INT_P4_PIN 0 // P4.0
#define LCD_GPIO_DATA_P3_PIN 1 // P3.1
#define LCD_UART_INT_P3_PIN USCI_A0_RX_P3_PIN
#define LCD_UART_DATA_P3_PIN USCI_A0_TX_P3_PIN

#define LCD_STX 0x02
#define LCD_ETX 0x03

#define LCD_ERROR_NONE 0
#define LCD_ERROR_REPORT 1
#define LCD_ERROR_METERING 2

#define LEN_MAX_LCD_TX 0x40
#define LEN_MAX_LCD_RX 0
#define LEN_LCD_MSG_BODY 11

#define MSG_LCD_CLEAR 0x10
#define MSG_LCD_DISPLAY_ALL 0x11
#define MSG_LCD_WAIT 0x20
#define MSG_LCD_DISPLAY_METER 0x30
#define MSG_LCD_TEST 0x40
#define MSG_LCD_DISPLAY_ERR 0x50
#define MSG_LCD_DISPLAY_FW_VER 0x60
#define MSG_LCD_DISPLAY_STRING 0x70

typedef struct {
	uint8 stx;
	uint8 mtype;
	uint8 body[LEN_LCD_MSG_BODY];
	uint8 checksum;
	uint8 etx;
} LcdMsg_t;

typedef struct {
	uint8 battery : 2, // terminal battery
		lowBatt : 1, // meter battery
		fArrow : 1, rArrow : 1, m3 : 1, notUsed : 1, leak : 1;
} LcdIcon_t;

typedef struct {
	uint8 stx;
	uint8 mtype;
	uint8 digit[9];
	LcdIcon_t icon;
	uint8 flow;
	uint8 checksum;
	uint8 etx;
} LcdMeterMsg_t;

BOOL LCD_Exist();
void LCD_init();
void LCD_enable();
void LCD_disable();
void LCD_clear();
void LCD_displayAll();
void LCD_wait();
void LCD_displayFirmwareVersion();
void LCD_displayCurrentValue();
void LCD_accessModem();
void LCD_notUsed();
void LCD_updateMeterValue(uint8 *unit);
void LCD_redraw();
void LCD_displayString(char *str);
void LCD_displayError(int errCode);
#endif // _LCD_DRIVER_H_
