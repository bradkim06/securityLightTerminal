#ifndef __METER_H__
#define __METER_H__

#include "lcd.h"

//=======================================
// Meter caliber type
//=======================================
/* #define MT_15mm 0x01 */
/* #define MT_20mm 0x02 */
/* #define MT_25mm 0x03 */
/* #define MT_32mm 0x04 */
/* #define MT_40mm 0x05 */
/* #define MT_50mm 0x06 */
/* #define MT_80mm 0x07 */
/* #define MT_100mm 0x08 */
/* #define MT_150mm 0x09 */
/* #define MT_200mm 0x0A */
/* #define MT_250mm 0x0B */
/* #define MT_300mm 0x0C */

//=======================================
// Electric Meter Type
//=======================================
#define E_METER 0x01

//=======================================
// Water Meter Type
//=======================================
/* #define W_STANDARD_D 0x01 */
/* #define W_HITEC_D 0x10 */
/* #define W_SHINHAN_D 0x20 */
/* #define W_SHINHAN_D_BIG 0x22 */
/* #define W_MNS_D 0x30 */
/* #define W_PRIMO 0x40 */
/* #define W_BADGER 0x42 */
/* #define W_ONETL_D 0x50 */

/* #define W_PULSE_1000L 0x71 */
/* #define W_PULSE_500L 0x72 */
/* #define W_PULSE_100L 0x73 */
/* #define W_PULSE_50L 0x74 */
/* #define W_PULSE_10L 0x75 */
/* #define W_PULSE_5L 0x76 */
/* #define W_PULSE_1L 0x77 */
/* #define W_PULSE_05L 0x78 */

//=======================================
// Gas Meter Type
//=======================================
#define G_STANDARD_D 0x02
#define G_ONETL_D 0x51

#define G_PULSE_1000L 0xA1
#define G_PULSE_500L 0xA2
#define G_PULSE_100L 0xA3
#define G_PULSE_50L 0xA4
#define G_PULSE_10L 0xA5
#define G_PULSE_5L 0xA6
#define G_PULSE_1L 0xA7
#define G_PULSE_05L 0xA8

//=======================================
// Calori Meter Type - ¿­·®
//=======================================
#define C_STANDARD_D 0x03
#define C_ONETL_D 0x52

#define C_PULSE_1000L 0x91
#define C_PULSE_500L 0x92
#define C_PULSE_100L 0x93
#define C_PULSE_50L 0x94
#define C_PULSE_10L 0x95
#define C_PULSE_5L 0x96
#define C_PULSE_1L 0x97
#define C_PULSE_05L 0x98

//=======================================
// Hot Water Meter Type
//=======================================
#define H_STANDARD_D 0x04
#define H_ONETL_D 0x53

#define H_PULSE_1000L 0x81
#define H_PULSE_500L 0x82
#define H_PULSE_100L 0x83
#define H_PULSE_50L 0x84
#define H_PULSE_10L 0x85
#define H_PULSE_5L 0x86
#define H_PULSE_1L 0x87
#define H_PULSE_05L 0x88

//=======================================
// Electric Meter Type
//=======================================
#define E_PULSE_1000W 0x61
#define E_PULSE_100W 0x62
#define E_PULSE_10W 0x63
#define E_PULSE_8W 0x64
#define E_PULSE_4W 0x65
#define E_PULSE_2W 0x66
#define E_PULSE_1W 0x67
#define E_PULSE_04W 0x68
#define E_PULSE_02W 0x69
#define E_PULSE_01W 0x6A

//=======================================
// Meter Type Macro
//=======================================
#define METER_TYPE_W_PULSE(x)                                                  \
	(x == W_PULSE_05L || x == W_PULSE_1L || x == W_PULSE_5L ||             \
	 x == W_PULSE_10L || x == W_PULSE_50L || x == W_PULSE_100L ||          \
	 x == W_PULSE_500L || x == W_PULSE_1000L)

#define METER_TYPE_E_PULSE(x)                                                  \
	(x == E_PULSE_01W || x == E_PULSE_02W || x == E_PULSE_04W ||           \
	 x == E_PULSE_1W || x == E_PULSE_2W || x == E_PULSE_4W ||              \
	 x == E_PULSE_8W || x == E_PULSE_10W || x == E_PULSE_100W ||           \
	 x == E_PULSE_1000W)

#define METER_TYPE_H_PULSE(x)                                                  \
	(x == H_PULSE_05L || x == H_PULSE_1L || x == H_PULSE_5L ||             \
	 x == H_PULSE_10L || x == H_PULSE_50L || x == H_PULSE_100L ||          \
	 x == H_PULSE_500L || x == H_PULSE_1000L)

#define METER_TYPE_C_PULSE(x)                                                  \
	(x == C_PULSE_05L || x == C_PULSE_1L || x == C_PULSE_5L ||             \
	 x == C_PULSE_10L || x == C_PULSE_50L || x == C_PULSE_100L ||          \
	 x == C_PULSE_500L || x == C_PULSE_1000L)

#define METER_TYPE_G_PULSE(x)                                                  \
	(x == G_PULSE_05L || x == G_PULSE_1L || x == G_PULSE_5L ||             \
	 x == G_PULSE_10L || x == G_PULSE_50L || x == G_PULSE_100L ||          \
	 x == G_PULSE_500L || x == G_PULSE_1000L)

#define METER_TYPE_PULSE(x)                                                    \
	(METER_TYPE_W_PULSE(x) || METER_TYPE_E_PULSE(x) ||                     \
	 METER_TYPE_H_PULSE(x) || METER_TYPE_C_PULSE(x) ||                     \
	 METER_TYPE_G_PULSE(x))

#define METER_TYPE_STANDARD(x)                                                 \
	(x == W_STANDARD_D || x == G_STANDARD_D || x == C_STANDARD_D ||        \
	 x == H_STANDARD_D)

#define METER_TYPE_SHINHAN(x) (x == W_SHINHAN_D || x == W_SHINHAN_D_BIG)

#define METER_TYPE_ONETL(x)                                                    \
	(x == W_ONETL_D || x == G_ONETL_D || x == C_ONETL_D || x == H_ONETL_D)

//=======================================
// METER Point Num (fixed only)
//=======================================
#define METER_POINT_0(x)                                                       \
	(x == W_PULSE_500L || x == W_PULSE_1000L || x == H_PULSE_500L ||       \
	 x == H_PULSE_1000L || x == C_PULSE_500L || x == C_PULSE_1000L ||      \
	 x == G_PULSE_500L || x == G_PULSE_1000L)

#define METER_POINT_1(x)                                                       \
	(x == W_PULSE_50L || x == W_PULSE_100L || x == H_PULSE_50L ||          \
	 x == H_PULSE_100L || x == C_PULSE_50L || x == C_PULSE_100L ||         \
	 x == G_PULSE_50L || x == G_PULSE_100L)

#define METER_POINT_2(x)                                                       \
	(x == W_PULSE_5L || x == W_PULSE_10L || x == H_PULSE_5L ||             \
	 x == H_PULSE_10L || x == C_PULSE_5L || x == C_PULSE_10L ||            \
	 x == G_PULSE_5L || x == G_PULSE_10L || METER_TYPE_E_PULSE(x) ||       \
	 x == W_SHINHAN_D_BIG)

#define METER_POINT_3(x)                                                       \
	(x == W_PULSE_05L || x == W_PULSE_1L || x == H_PULSE_05L ||            \
	 x == H_PULSE_1L || x == C_PULSE_05L || x == C_PULSE_1L ||             \
	 x == G_PULSE_05L || x == G_PULSE_1L || x == W_ONETL_D ||              \
	 x == G_ONETL_D || x == C_ONETL_D || x == H_ONETL_D)

//**************************************
// Meter data unit structure
//**************************************
typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 isTimeSync;
	uint8 meterData[4];
	uint8 meterStatus;
	uint8 st[2];
	uint8 rt[2];
	uint8 flowData[4];
	LcdIcon_t icon;
} MeterUnitData_t;

char *METER_getMeterName(int meterType);

BOOL METER_isAllFF(uchar *p, int len);
void METER_displayTempData();
void METER_displayStoredData();
void METER_clearTempData();
void METER_clearStoredData(BOOL isClearAll);
MeterUnitData_t *METER_getTempData();
MeterUnitData_t *METER_getStoredData(int pos);
void METER_addTempData(Date_t *pDate, MeterUnitData_t *pUnit);
void METER_addStoredData(Date_t *pDate, MeterUnitData_t *pUnit);
void METER_collectStoredData();

void METER_saveInFlash(Date_t *pDate, MeterUnitData_t *pUnit);
BOOL METER_needNewMetering(int secLimit);
int METER_getNumberOfStoredData();
int METER_getStoredInterval();
uint8 *METER_getSerialNum();
uint8 METER_getMeterCaliberDp();
int METER_getFirstValidPos();
void METER_saveMeterInfo(uint8 *serial, uint8 caliberDp, uint8 dif, uint8 vif);

#endif // __METER_H__
