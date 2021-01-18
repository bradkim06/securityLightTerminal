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

uint8 std_checksum(uint8 *from, int size)
{
	uint8 checksum = 0;
	for (int i = 0; i < size; i++) {
		checksum += *(from + i);
	}

	return checksum;
}

/* Remove read_shinhan_d_meter */

/* Remove read_mns_d_meter */

/* Remove read_onetl_meter */

/* Comment METER_std_lcdMarkReq */
/* BOOL METER_std_lcdMarkReq(BOOL isMarkOn) */
/* { */
/* 	LCD_wait(); */

/* 	StdLcdMarkReq_t msg; */

/* 	msg.stx = STD_REQ_START; */
/* 	msg.cfield = STD_LCD_MARK_REQ; */
/* 	msg.afield = STD_REQ_ADDR; */
/* 	msg.data = */
/* 		(isMarkOn == TRUE) ? 1 : 0; // 1 is on H Mark, 0 is off H Mark */
/* 	msg.checksum = std_checksum((uint8 *)&msg.cfield, */
/* 				    (int)(&msg.checksum - &msg.cfield)); */
/* 	msg.etx = STD_REQ_STOP; */

/* 	METER_enable(W_STANDARD_D); */
/* 	MISC_delayMs(20); // STD spec delay for waitting slave prepare */

/* 	UART_send(UART_A3, &msg, sizeof(StdLcdTestReq_t), FALSE); */
/* 	return TRUE; */
/* } */

/* Comment METER_std_configRequest */
/* BOOL METER_std_configRequest() */
/* { */
/* 	LCD_wait(); */

/* 	StdMeterReq_t msg; */
/* 	StdMeterReq_t *pMsg = &msg; */

/* 	pMsg->stx = STD_REQ_START; */
/* 	pMsg->cfield = STD_METER_CONFIG_REQ; */
/* 	pMsg->afield = STD_REQ_ADDR; */
/* 	pMsg->checksum = std_checksum((uint8 *)&pMsg->cfield, */
/* 				      (int)(&pMsg->checksum - &pMsg->cfield)); */
/* 	pMsg->etx = STD_REQ_STOP; */

/* 	METER_enable(W_STANDARD_D); */
/* 	MISC_delayMs(20); // STD spec delay for waitting slave prepare */

/* 	UART_send(UART_A3, &msg, sizeof(StdMeterReq_t), FALSE); */
/* 	return TRUE; */
/* } */

/* Comment METER_std_adjustReq */
// Only STD, It is request to adjust Q3 ~ Q1 and Qt
/* BOOL METER_std_adjustReq(char *sn, uint8 caliber, uint8 maker, uint16 q3, */
/* 			 uint16 qt, uint16 q2, uint16 q1) */
/* { */
/* 	LCD_wait(); */

/* 	// printf("Maker/Caliber : %d/%d \n", maker, caliber); */
/* 	// printf("Q3/Qt/Q2/Q1   : %d/%d/%d/%d \n", q3, qt, q2, q1); */

/* 	StdMeterAdjustReq_t msg; */

/* 	memcpy(msg.serial, sn, 4); */
/* 	msg.stx = STD_REQ_START; */
/* 	msg.cfield = STD_METER_ADJUST_REQ; */
/* 	msg.afield = STD_REQ_ADDR; */
/* 	msg.caliber = caliber; */
/* 	msg.maker = maker; */
/* 	msg.q3[0] = q3 & 0x00FF; */
/* 	msg.q3[1] = q3 & 0xFF00 >> 8; */
/* 	msg.qt[0] = qt & 0x00FF; */
/* 	msg.qt[1] = qt & 0xFF00 >> 8; */
/* 	msg.q2[0] = q2 & 0x00FF; */
/* 	msg.q2[1] = q2 & 0xFF00 >> 8; */
/* 	msg.q1[0] = q1 & 0x00FF; */
/* 	msg.q1[1] = q1 & 0xFF00 >> 8; */
/* 	msg.checksum = std_checksum((uint8 *)&msg.cfield, */
/* 				    (int)(&msg.checksum - &msg.cfield)); */
/* 	msg.etx = STD_REQ_STOP; */

/* 	METER_enable(W_STANDARD_D); */
/* 	MISC_delayMs(20); // STD spec delay for waitting slave prepare */

/* 	UART_send(UART_A3, &msg, sizeof(StdMeterAdjustReq_t), FALSE); */
/* 	return TRUE; */
/* } */

/* Comment METER_std_adjustResp */
// Only STD, It is response of adjust request
/* BOOL METER_std_adjustResp(StdAdjustResult_t *result) */
/* { */
/* 	uint8 buf[LEN_MAX_METER_RX]; */
/* 	memset(buf, 0, LEN_MAX_METER_RX); */
/* 	int len = UART_receive(UART_A3, buf, LEN_MAX_METER_RX); */

/* 	StdMeterAdjustResp_t *resp = (StdMeterAdjustResp_t *)buf; */

/* 	if (resp->stx1 != STD_RESP_START || resp->stx2 != STD_RESP_START) { */
/* 		printf("invalid message : start bit is not matched\n"); */
/* 		return FAIL; */
/* 	} */

/* 	int checksum = std_checksum((uint8 *)&resp->cfield, */
/* 				    (int)(&resp->checksum - &resp->cfield)); */
/* 	if (resp->checksum != checksum) { */
/* 		printf("invalid message : checksum fail\n"); */
/* 		return FAIL; */
/* 	} */

/* 	if (resp->cfield == STD_METER_ADJUST_RESP) { */
/* 		memcpy(result, (buf + (int)(&(resp->caliber) - &(resp->stx1))), */
/* 		       sizeof(StdAdjustResult_t)); */
/* 	} else { */
/* 		printf("invalid message, not matched 'cfield'\n"); */
/* 	} */

/* 	return SUCCESS; */
/* } */

/**
 * @brief Send STD request from NFC to meter(STD)
 *
 * @param body      request data
 * @param bodyLen   request data len
 */
void METER_bypassReq(uint8 *body, int bodyLen)
{
	//TODO METER_bypassReq
	/* Comment METER_bypassReq() */
	/* LCD_wait(); */

	/* if (bodyLen > LEN_MAX_METER_TX - 2) { */
	/* 	bodyLen = LEN_MAX_METER_TX - 2; */
	/* } */

	/* uint8 txmsg[LEN_MAX_METER_TX]; */
	/* memset(txmsg, 0, LEN_MAX_METER_TX); */

	/* txmsg[0] = STD_REQ_START; // stx */
	/* memcpy((uint8 *)&txmsg[1], body, bodyLen); */
	/* txmsg[bodyLen + 1] = STD_REQ_STOP; // etx */

	/* METER_enable(E_METER); */
	/* MISC_delayMs(20); // STD spec delay for waitting slave prepare */

	/* UART_send(UART_A3, txmsg, bodyLen + 2, FALSE); */

	//    PRINT_hexBuffer(txmsg, bodyLen+2);
}

BOOL METER_sendRequest(uint8 meterType)
{
	BOOL result = FALSE;

	if (meterType == E_METER) {
		//TODO
	}

	return result;
}

void METER_enable(uint8 meterType)
{
	//TODO
	/* #if (REPEATER_DEVICE == 0) */
	/* 	UartInitParam_t param = { */
	/* 		.isRxEnable = TRUE, */
	/* 		.parity = UART_PARITY_NONE, */
	/* 		.stopbit = UART_STOPBIT_ONE, */
	/* 		.charLen = UART_8BITS_CHAR, */
	/* 	}; */

	/* 	if (meterType == W_SHINHAN_D || meterType == W_SHINHAN_D_BIG) { */
	/* 		param.isTxEnable = FALSE; */
	/* 		param.baudrate = UART_BAUDRATE_600; */
	/* 	} else if (METER_TYPE_STANDARD(meterType)) { */
	/* 		param.isTxEnable = TRUE; */
	/* 		param.baudrate = UART_BAUDRATE_1200; */
	/* 	} else if (meterType == W_MNS_D) { */
	/* 		param.isTxEnable = FALSE; */
	/* 		param.baudrate = UART_BAUDRATE_1200; */
	/* 		param.parity = UART_PARITY_EVEN; */
	/* 		param.stopbit = UART_STOPBIT_ONE; */
	/* 		param.charLen = UART_7BITS_CHAR; */
	/* 	} else if (METER_TYPE_ONETL(meterType)) { */
	/* 		param.isTxEnable = TRUE; */
	/* 		param.baudrate = UART_BAUDRATE_2400; */
	/* 	} else { */
	/* 		return; */
	/* 	} */

	/* 	UART_open(UART_A3, &param); */
	/* 	if (meterType == W_MNS_D || meterType == W_SHINHAN_D || */
	/* 	    meterType == W_SHINHAN_D_BIG) { */
	/* 		METER_INTERRUPT_LOW(); */
	/* 		METER_RXD_PULL_DOWN(); */
	/* 		METER_RXD_PULL_UP(); */
	/* 	} */
	/* #endif */
}

void METER_disable()
{
	UART_close(UART_A3, GPIO_LOW);
	METER_INTERRUPT_LOW();
	METER_RXD_PULL_DOWN();
}

BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t *pUnit)
{
	uint8 buf[LEN_MAX_METER_RX];
	memset(buf, 0, LEN_MAX_METER_RX);
	int len = UART_receive(UART_A3, buf, LEN_MAX_METER_RX);

	BOOL result = FALSE;

	if (meterType == E_METER) {
		//TODO
	}
	/* Comment */
	/* if (meterType == W_STANDARD_D) { */
	/* 	result = read_std_d_meter(buf, pUnit); */
	/* } else if (meterType == W_SHINHAN_D || meterType == W_SHINHAN_D_BIG) { */
	/* 	result = read_shinhan_d_meter(buf, pUnit); */
	/* } else if (meterType == W_MNS_D) { */
	/* 	result = read_mns_d_meter(buf, pUnit); */
	/* } else if (METER_TYPE_ONETL(meterType)) { */
	/* 	result = read_onetl_meter(buf, len, pUnit); */
	/* } */

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
