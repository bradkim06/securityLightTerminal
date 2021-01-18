#ifndef __METERING_H__
#define __METERING_H__

#include "meter.h"

#define LEN_MAX_METER_TX 0x40
#define LEN_MAX_METER_RX 0x60

BOOL METER_sendRequest(uint8 meterType);
BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t *pUnit);
/* Comment METER_std_configRequest */
/* BOOL METER_std_configRequest(); */
/* Comment METER_std_adjustReq */
/* BOOL METER_std_adjustReq(char *sn, uint8 caliber, uint8 maker, uint16 q3, */
/* 			 uint16 qt, uint16 q2, uint16 q1); */
/* Comment METER_std_adjustResp */
/* BOOL METER_std_adjustResp(StdAdjustResult_t *result); */
/* Comment METER_std_lcdTestReq */
/* BOOL METER_std_lcdTestReq(); */
/* Comment METER_std_lcdMarkReq */
/* BOOL METER_std_lcdMarkReq(BOOL isMarkOn); */

void METER_enable(uint8 meterType);
void METER_disable();

void METER_bypassReq(uint8 *body, int bodyLen);
BOOL METER_bypassResp();

#endif //__METERING_H__
