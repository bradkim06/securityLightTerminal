#ifndef __CHECK_METER_MISC_H__
#define __CHECK_METER_MISC_H__

#include "var_type.h"

int MISC_getDeviceType(void);
void MISC_delayUs(uint32 timeout);
void MISC_delayMs(uint32 timeout);
BOOL MISC_initRandom(uint32 myAddr);
uint16 MISC_getRandomNum();
BOOL MISC_checkInterval(int h, int basetime, int interval);

void MISC_ipToHexArray(char *ip, uint8 *hexIp);
void MISC_intToBCD(uint32 *pInt, uint8 *pBCD, uint8 nDigit);
void MISC_bcdToInt(uint8 *pBCD, uint32 *pInt, uint8 nDigit);

void MISC_decDateHour(Date_t *p, uint8 hour);
void MISC_incDateHour(Date_t *p, uint8 hour);
void MISC_incDateTime(Date_t *p, uint16 secOffset);
int MISC_isValidDate(Date_t *date);
int MISC_calcHourDiff(Date_t *prev, Date_t *next);
int MISC_calcMinDiff(Date_t *prev, Date_t *next);
long MISC_calcSecDiff(Date_t *prev, Date_t *next);

uint8 MISC_asciiToHex(char ch);
uint8 MISC_asciiToBCD(char a, char b);
uint8 MISC_hexStringToInt(uint8 value);

#endif // __CHECK_METER_MISC_H__
