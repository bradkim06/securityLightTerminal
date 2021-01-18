/**
 * @file uart.h
 * @author ytkim (ytkim@hitecepc.com)
 * @brief
 *  [UART 0]
 *   - TxD (P3.4) : 5438(5419)에서 413으로 LCD 제어 메시지를 전달
 *   - RxD (P3.5) : GPIO mode(5438(5419)이 UART0 TxD pin으로 메시지를 곧 보낼
 * 거라는 점을 413에 알림
 *   - txBuf만 필요
 *  [UART 1]
 *   - TxD (P5.6) : console TX
 *   - RxD (P5.7) : console RX
 *   - txBuf, rxBuf 모두 필요
 *  [UART 2]
 *   - TxD (P9.4) : TxD to SMT RF Module
 *   - RxD (P9.5) : RxD from SMT RF Module
 *   - txBuf 만 필요(rx의 경우 shell에 정의된 commandBuffer를 직접 이용
 *  [UART 3]
 *   - TxD (P10.4) : 신한/MNS 계량기의 경우 GPIO, 다른 계량기는 UART Mode
 *   - RxD (P10.5) : RxD from meter
 *   - 신한/MNS의 경우 rxBuf만 필요, 다른 계량기는 TxBuf/RxBuf 모두 필요
 *
 * @version 0.1
 * @date 2020-04-02
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef __UART_H__
#define __UART_H__

#include "app.h"
#include "var_type.h"

// UART pin information
#define USCI_A0_TX_P3_PIN 4 // P3.4
#define USCI_A0_RX_P3_PIN 5 // P3.5
#define USCI_A1_TX_P5_PIN 6 // P5.6
#define USCI_A1_RX_P5_PIN 7 // P5.7
#define USCI_A2_TX_P9_PIN 4 // P9.4
#define USCI_A2_RX_P9_PIN 5 // P9.5
#define USCI_A3_TX_P10_PIN 4 // P10.4
#define USCI_A3_RX_P10_PIN 5 // P10.5

// UART console print color
#define TP_ANSI_RESET "\x1b[0m" // Foreground color table
#define TP_ANSI_BOLD_ON "\x1b[1m"
#define TP_ANSI_INVERSE_ON "\x1b[7m"
#define TP_ANSI_BOLD_OFF "\x1b[22m"
#define TP_ANSI_FG_BLACK "\x1b[30m"
#define TP_ANSI_FG_RED "\x1b[31m"
#define TP_ANSI_FG_GREEN "\x1b[32m"
#define TP_ANSI_FG_YELLOW "\x1b[33m"
#define TP_ANSI_FG_BLUE "\x1b[34m"
#define TP_ANSI_FG_MAGENTA "\x1b[35m"
#define TP_ANSI_FG_CYAN "\x1b[36m"
#define TP_ANSI_FG_WHITE "\x1b[37m"
#define TP_ANSI_BG_RED "\x1b[41m" // Background color table
#define TP_ANSI_BG_GREEN "\x1b[42m"
#define TP_ANSI_BG_YELLOW "\x1b[43m"
#define TP_ANSI_BG_BLUE "\x1b[44m"
#define TP_ANSI_BG_MAGENTA "\x1b[45m"
#define TP_ANSI_BG_CYAN "\x1b[46m"
#define TP_ANSI_BG_WHITE "\x1b[47m"

//=========================================================
// NFC ISR macro interface for external access
//=========================================================
#define UART_A0_RX_ISR_HANDLER() APP_meterRxIsr()
#define UART_A1_RX_ISR_HANDLER()
#define UART_A2_RX_ISR_HANDLER() APP_modemRxIsr()
#define UART_A3_RX_ISR_HANDLER()

//=========================================================
// TYPES
//=========================================================
typedef enum {
	GPIO_INPUT = 0,
	GPIO_LOW = 1,
} UartPinMode_t;

typedef enum { UART_A0 = 0, UART_A1 = 1, UART_A2 = 2, UART_A3 = 3 } UartNum_t;

typedef enum {
	UART_BAUDRATE_600 = 0,
	UART_BAUDRATE_1200 = 1,
	UART_BAUDRATE_2400 = 2,
	// UART_BAUDRATE_4800   = 3,
	UART_BAUDRATE_9600 = 4,
	// UART_BAUDRATE_14400  = 5,
	// UART_BAUDRATE_19200  = 6,
	UART_BAUDRATE_38400 = 7,
	// UART_BAUDRATE_57600  = 8,
	UART_BAUDRATE_115200 = 9
} UartBaudrate_t;

typedef enum {
	UART_PARITY_NONE = 0,
	UART_PARITY_ODD = 1,
	UART_PARITY_EVEN = 2
} UartParity_t;

typedef enum { UART_STOPBIT_ONE = 0, UART_STOPBIT_TWO = 1 } UartStopbit_t;

typedef enum { UART_8BITS_CHAR = 0, UART_7BITS_CHAR = 1 } UartCharLen_t;

typedef struct {
	BOOL isTxEnable;
	BOOL isRxEnable;
	UartBaudrate_t baudrate;
	UartParity_t parity;
	UartStopbit_t stopbit;
	UartCharLen_t charLen;
} UartInitParam_t;

// UART library
void UART_init(UartNum_t num, int txBufLen, int rxBufLen);
void UART_open(UartNum_t num, UartInitParam_t *param);
void UART_close(UartNum_t num, UartPinMode_t mode);
void UART_send(UartNum_t num, void *buffer, int len, BOOL isAppendCR);
int UART_receive(UartNum_t num, void *buffer, int limit);
BOOL UART_getChar(UartNum_t num, char *ch);
void UART_putChar(UartNum_t num, char ch);

// Default print functions
int printf(const char *format, ...);
int printf_ts(const char *format, ...);

// Additional function print functions
void PRINT_enable();
void PRINT_disable();
void PRINT_stop();
void PRINT_resume();
void PRINT_hexBuffer(uint8 *p, int len);
void PRINT_string(char *p, int len);
#endif
