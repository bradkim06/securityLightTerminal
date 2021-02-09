#include "metering.h"
#include "common.h"
#include "device.h"
#include "meter.h"
#include "misc.h"

#include "alarm.h"
#include "nfc_app.h"

#include "battery.h"
#include "flash.h"
#include "lcd.h"
#include "rtc.h"
#include "uart.h"

#define METER_INTERRUPT_HIGH()                                                 \
	do {                                                                   \
		P10SEL &= ~0x10, P10DIR |= 0x10, P10OUT |= 0x10;               \
	} while (0);
#define METER_INTERRUPT_LOW()                                                  \
	do {                                                                   \
		P10SEL &= ~0x10;                                               \
		P10DIR |= 0x10;                                                \
		P10OUT &= ~0x10;                                               \
	} while (0);
#define METER_RXD_PULL_DOWN()                                                  \
	do {                                                                   \
		P10REN |= 0x20;                                                \
		P10OUT &= ~0x20;                                               \
	} while (0);
#define METER_RXD_PULL_UP()                                                    \
	do {                                                                   \
		P10REN |= 0x20;                                                \
		P10OUT |= 0x20;                                                \
	} while (0);

static uint8 std_checksum(uint8 *from, int size)
{
	uint8 checksum = 0;
	// size - 2 -> Not Calculate ETX, Checksum
	for (int i = 0; i < size - 2; i++) {
		checksum += *(from + i);
	}

	return checksum;
}

static BOOL read_electric_meter(uint8 *rxBuf, MeterUnitData_t *pUnit)
{
	elecMeterResp_t *rx = (elecMeterResp_t *)rxBuf;

	for (int i = 0; i < sizeof(elecMeterResp_t); i++) {
		printf("rxBuf[%d] = 0x%02X\n", i, rxBuf[i]);
	}

	if (rx->stx != METER_STX) {
		printf("Invalid Meter STX\n");
		return FAIL;
	}

	if (rx->etx != METER_ETX) {
		printf("Invalid Meter ETX\n");
		return FAIL;
	}

	if (rx->checkSum != std_checksum(rxBuf, sizeof(elecMeterResp_t))) {
		printf("Invalid Checksum\n");
		return FAIL;
	}

	printf("E_meter\n");

	return SUCCESS;
}

static BOOL send_meteringEV(void)
{
	evMeterReq_t msg;
	evMeterReq_t *pMsg = &msg;

	pMsg->stx = TERMINAL_STX;
	pMsg->msgType = REQ_READ_METER;
	pMsg->checkSum =
		std_checksum((uint8 *)&pMsg->stx, sizeof(evMeterReq_t));
	pMsg->etx = TERMINAL_ETX;

	UART_send(UART_A0, &msg, sizeof(evMeterReq_t), FALSE);
	return TRUE;
}

/**
 * @brief Send STD request from NFC to meter(STD)
 *
 * @param body      request data
 * @param bodyLen   request data len
 */
void METER_bypassReq(uint8 *body, int bodyLen)
{
	//TODO METER_bypassReq
}

BOOL METER_sendRequest(uint8 meterType)
{
	BOOL result = FALSE;

	if (meterType == E_METER) {
		send_meteringEV();
	}

	return result;
}

void METER_enable(uint8 meterType)
{
#if (REPEATER_DEVICE == 0)
	UartInitParam_t param = {
		.isRxEnable = TRUE,
		.isTxEnable = TRUE,
		.baudrate = UART_BAUDRATE_9600,
		.parity = UART_PARITY_NONE,
		.stopbit = UART_STOPBIT_ONE,
		.charLen = UART_8BITS_CHAR,
	};

	UART_open(UART_A0, &param);
#endif
}

BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t *pUnit)
{
	uint8 buf[LEN_MAX_METER_RX];
	memset(buf, 0, LEN_MAX_METER_RX);
	int len = UART_receive(UART_A0, buf, LEN_MAX_METER_RX);

	BOOL result = FALSE;

	if (meterType == E_METER) {
		result = read_electric_meter(buf, pUnit);
	}

	return result;
}

BOOL METER_bypassResp()
{
	//TODO METER_bypassResp
	/* Comment METER_bypassResp() */
	/* uint8 buf[LEN_MAX_METER_RX]; */
	/* memset(buf, 0, LEN_MAX_METER_RX); */
	/* int len = UART_receive(UART_A3, buf, LEN_MAX_METER_RX); */

	/* if (buf[0] != STD_RESP_START || buf[3] != STD_RESP_START) { */
	/* 	printf("invalid message\n"); */
	/* 	return FAIL; */
	/* } */

	/* // PRINT_hexBuffer(rxMsg, len); */
	/* // 4 - stx1,l1field, l2field, stx2 */
	/* // 5 - stx1,l1field, l2field, stx2, etx */
	/* NFCAPP_meterBypassResp(buf + 4, len - 5); */
	/* return SUCCESS; */
}
