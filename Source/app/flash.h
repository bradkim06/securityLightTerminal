#ifndef __FLASH_DRIVER_H__
#define __FLASH_DRIVER_H__

#include "common.h"

extern const uint8 *FlashAddr[];

//! INFO flash sector count
#define NUM_FLASH_SECTOR 4

//! INFO flash sector length
#define LEN_INFO_FLASH_SECTOR 128

//! Configuration data length without checksum
#define LEN_FLASH_CONFIG 31

// NBIOT_DEVICE
typedef struct {
	uint8 meterType; // 0
	uint8 meterInterval; // 2
	uint8 reportInterval; // 3
	uint8 serverIp[4]; // 4, 5, 6, 7
	uint8 serverPort[2]; // 8, 9
	uint8 resetCount[2]; // 10, 11
	uint8 isShortInterval; // 12
	uint8 debugPrint; // 13
	// service code는 현재 LG플랫폼 연동시에만 유효
	uint8 serviceCode[4]; // 14, 15, 16, 17
	uint8 isModemInit; // 18
	uint8 fotaIp[4]; // 19, 20, 21, 22
	uint8 fotaPort[2]; // 23, 24
	uint8 fotaInterval; // 25
	uint8 rtCtrlMode; // 26
	uint8 isUsingPF; // 27
	uint8 reserved[3]; // 28 ~ 30
	uint8 checksum; // 31
} FlashConfig_t;

#define LEN_FLASH_ID 31
typedef struct {
	uint8 serialNum[SERIAL_NUM_LEN]; //  0 ~ 11
	uint8 imei[8]; // 12 ~ 19
	uint8 reserved[11]; // 20 ~ 30
	uint8 checksum; // 31
} FlashId_t;

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

/**************************************************
 * Flash R/W/E interface for Meter data
 **************************************************/
void FLASH_printData();
void FLASH_initData();
void FLASH_deleteAllData();
void FLASH_saveData(Date_t *date, uint8 batt, uint8 *value);
int FLASH_readData(FlashMeterData_t *fData, uint8 num);

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
FlashMeterData_t *FLASH_readPeriodData(Date_t *fromDate, Date_t *toDate,
				       int *totalNumOfDatas);

/**************************************************
 * Flash R/W/E interface for Configuration data
 **************************************************/
void FLASH_saveConfigInfo(Config_t *config);
void FLASH_readConfigInfo(Config_t *config);
uint8 FLASH_readResetCause();
void FLASH_updateResetCount(Config_t *config);

/**************************************************
 * Etc supporting functions
 **************************************************/
void MISC_bcdToInt(uint8 *pBCD, uint32 *pInt, uint8 nDigit);
void MISC_intToBCD(uint32 *pInt, uint8 *pBCD, uint8 nDigit);
uint8 MISC_asciiToHex(char ch);
uint8 MISC_asciiToBCD(char a, char b);
void MISC_ipToHexArray(char *ip, uint8 *hexIp);
#if REPEATER_DEVICE
int getFirstZeroPosition(uint32 nwk_addr);
#endif

#endif //__FLASH_DRIVER_H__
