#ifndef __FLASH_DRIVER_H__
#define __FLASH_DRIVER_H__

#include "common.h"

extern const uint8 *FlashAddr[];

//! INFO flash sector count
#define NUM_FLASH_SECTOR 4

//! INFO flash sector length
#define LEN_INFO_FLASH_SECTOR 128

// NBIOT_DEVICE
typedef struct {
	uint8 meterType;
	uint8 reportInterval;
	uint8 serverIp[4];
	uint8 serverPort[2];
	uint8 resetCount[2];
	uint8 isShortInterval;
	uint8 debugPrint;
	// service code는 현재 LG플랫폼 연동시에만 유효
	uint8 serviceCode[4];
	uint8 isModemInit;
	uint8 fotaIp[4];
	uint8 fotaPort[2];
	uint8 fotaInterval;
	uint8 checksum;
} FlashConfig_t; // 49

typedef struct {
	uint8 serialNum[SERIAL_NUM_LEN];
	uint8 imei[8];
	uint8 imsi[8];
	light_t light;
	uint8 latitInt[2];
	uint8 latitDec[3];
	uint8 longitInt[2];
	uint8 longitDec[3];
	uint8 checksum;
} FlashId_t; // 29

//! Meter data position in sector
#define POS_METER_REPORT 32

//! Number of data set per sector.
#define NUM_FLASH_DATA_PER_SECTOR 8

//! Total data count in INFO flash.
#define NUM_TOTAL_FLASH_DATA (NUM_FLASH_DATA_PER_SECTOR * NUM_FLASH_SECTOR)

//! Hour interval value when Diff has no data
#define HOUR_INTERVAL_NO_DATA 0

//! Hour interval MAX value
#define HOUR_INTERVAL_MAX_VALUE 0xFF

//! Diff MAX value
#define METER_DIFF_MAX_VALUE 0xFFF9

//! Diff definition that 2nd data is MT DOWN
#define METER_DIFF_2ND_MT_DOWN 0xFFFF

//! Diff definition that 2nd data set to first data becuase of 1st data is MT
//! DOWN
#define METER_DIFF_2ND_SET_TO_FIRST 0xFFFE

//! Meter data structrue for saving data into INFO flash
typedef struct {
	uint8 year; // year - 2000
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 batt;
	uint8 firstValue[4]; // bcd type
	uint8 hourInterval; // 1 ~ 254 hour (0:두번째 데이터 없음)
	uint8 diff[2]; // int type
} FlashMeterData_t; // 12 byte

/**
 * @brief
 *  Search and Get period flash data and Get period flash data count.
 * @param fromDate  Period date of start
 * @param toDate    Period date of end
 * @param totalNumOfDatas   Searced total data count. (retrun value)
 * @return FlashMeterData_t**   Period flash data in Heap. (have to free)
 *
 * @notice
 *  Flash data는 2개의 meter data(First Value, Diff)를 포함하고 있다.
 *  때문에 Return 데이터 중 1st 혹은 2nd 데이터 중 하나가 기간내 데이터가 아닐
 * 수 있으므로 실제 범위에 벗어나는 부분 데이터는 상위 layer에서 무시해야 한다.
 */
FlashMeterData_t *FLASH_readPeriodData(Date_t *fromDate, Date_t *toDate, int *totalNumOfDatas);

/**************************************************
 * Flash R/W/E interface for Configuration data
 **************************************************/
void FLASH_saveConfigInfo(Config_t *config);
void FLASH_readConfigInfo(Config_t *config);
void FLASH_updateResetCount(Config_t *config);

/**************************************************
 * Etc supporting functions
 **************************************************/
void MISC_bcdToInt(uint8 *pBCD, uint32 *pInt, uint8 nDigit);
void MISC_intToBCD(uint32 *pInt, uint8 *pBCD, uint8 nDigit);
uint8 MISC_asciiToHex(char ch);
uint8 MISC_asciiToBCD(char a, char b);
void MISC_ipToHexArray(char *ip, uint8 *hexIp);

#endif //__FLASH_DRIVER_H__
