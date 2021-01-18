#include "flash.h"
#include "flash_msp430.h"

#include "app.h"
#include "common.h"
#include "device.h"
#include "meter.h"
#include "misc.h"
#include "osal.h"
#include "rtc.h"

const uint8 *FlashAddr[NUM_FLASH_SECTOR] = { (uint8 *)FLASH_SEGA_ADDR,
					     (uint8 *)FLASH_SEGB_ADDR,
					     (uint8 *)FLASH_SEGC_ADDR,
					     (uint8 *)FLASH_SEGD_ADDR };

static uint8 calChecksum(uint8 *p, int len)
{
	uint8 checksum = 0;
	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

//! Check data is BCD format
static BOOL isBcd(uint8 *p, int len)
{
	while (--len >= 0) {
		if (((*(p + len) & 0xF0) >> 4) > 9 || (*(p + len) & 0x0F) > 9) {
			return FALSE;
		}
	}

	return TRUE;
}

//! Macro for date copy to flash data date
static void date2FlashDate(Date_t *date, FlashMeterData_t *flash)
{
	flash->year = date->year - 2000;
	flash->mon = date->mon;
	flash->day = date->day;
	flash->hour = date->hour;
}

//! Macro for flash data date copy to date
static void flashDate2Date(FlashMeterData_t *flash, Date_t *date)
{
	date->year = flash->year + 2000;
	date->mon = flash->mon;
	date->day = flash->day;
	date->hour = flash->hour;
	date->min = date->sec = 0;
}

/**************************************************
 * Flash functions for Configuration data
 **************************************************/

uint8 FLASH_readResetCause()
{
	uint8 cause = 0;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)SYSRSTIV_ADDR, &cause, 1);
	// 현재 Reset cuase data를 사용하고 있지 않으므로 추가처리는 생략한다.
	MSP430FLASH_erasePage((uint8 *)SYSRSTIV_ADDR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	return cause;
}

static void saveConfig(FlashConfig_t *pInfo)
{
	// Info Flash A와 C에 동일한 정보를 저장함

	uint8 flash[LEN_INFO_FLASH_SECTOR];

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);

	BOOL result = FALSE;
	MSP430FLASH_read((uint8 *)FLASH_SEGA_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashConfig_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGA_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGA_ADDR, flash,
				      LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	MSP430FLASH_read((uint8 *)FLASH_SEGC_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashConfig_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGC_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGC_ADDR, flash,
				      LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	if (!result) {
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
	}
}

#if NBIOT_DEVICE
#include "modem.h"
#endif
static void readConfig(FlashConfig_t *pInfo)
{
	// Info Flash A와 C에 저장된 정보를 읽어서 checksum을 확인하여
	// 저장된 값의 에러 여부를 확인함. 만일 두 부분 중 한 곳의 정보가
	// 깨져 있다면 에러 없는 부분의 데이터를 복사하여 복구함.

	uint8 flash[LEN_INFO_FLASH_SECTOR];

	halIntState_t intState;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGA_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashConfig_t info_a;
	memcpy((uint8 *)&info_a, flash, sizeof(FlashConfig_t));

	uint8 checksum = calChecksum((uint8 *)&info_a, LEN_FLASH_CONFIG);
	BOOL a_valid = (checksum == info_a.checksum) ? TRUE : FALSE;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGC_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashConfig_t info_c;
	memcpy((uint8 *)&info_c, flash, sizeof(FlashConfig_t));
	checksum = calChecksum((uint8 *)&info_c, LEN_FLASH_CONFIG);
	BOOL c_valid = (checksum == info_c.checksum) ? TRUE : FALSE;

	if (a_valid == TRUE && c_valid == TRUE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_a, sizeof(FlashConfig_t));
	} else if (a_valid == TRUE && c_valid == FALSE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_a, sizeof(FlashConfig_t));
		saveConfig(pInfo);
	} else if (a_valid == FALSE && c_valid == TRUE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_c, sizeof(FlashConfig_t));
		saveConfig(pInfo);
	} else {
		printf("Config: both side are false\n");
		memcpy((uint8 *)pInfo, (uint8 *)&info_a, sizeof(FlashConfig_t));
		pInfo->meterType = W_STANDARD_D;
		pInfo->meterInterval = 1;
		pInfo->reportInterval = 6;
		/* Remove sleepMode */
		/* pInfo->sleepMode = 0; */
		pInfo->rtCtrlMode = 0;
		pInfo->isShortInterval = 0;
		pInfo->debugPrint = 0;

#if NBIOT_DEVICE
		MISC_ipToHexArray((char *)ONEM2M_FOTA_SERVER_IP, pInfo->fotaIp);
		pInfo->fotaPort[0] = LO_UINT16(ONEM2M_FOTA_SERVER_PORT);
		pInfo->fotaPort[1] = HI_UINT16(ONEM2M_FOTA_SERVER_PORT);
		pInfo->fotaInterval = ONEM2M_FOTA_DAY_INTERVAL;
#endif

		memset(pInfo->resetCount, 0, 2);
		saveConfig(pInfo);
	}
}

static void saveID(FlashId_t *pInfo)
{
	// Info Flash B와 D에 동일한 정보를 저장함

	uint8 flash[LEN_INFO_FLASH_SECTOR];

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);

	BOOL result = FALSE;
	MSP430FLASH_read((uint8 *)FLASH_SEGB_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashId_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGB_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGB_ADDR, flash,
				      LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	MSP430FLASH_read((uint8 *)FLASH_SEGD_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashId_t));
	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGD_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGD_ADDR, flash,
				      LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	if (!result) {
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
	}
}

static void readID(FlashId_t *pInfo)
{
	// Info Flash B와 D에 저장된 정보를 읽어서 checksum을 확인하여
	// 저장된 값의 에러 여부를 확인함. 만일 두 부분 중 한 곳의 정보가
	// 깨져 있다면 에러 없는 부분의 데이터를 복사하여 복구함.

	uint8 flash[LEN_INFO_FLASH_SECTOR];

	halIntState_t intState;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGB_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashId_t info_b;
	memcpy((uint8 *)&info_b, flash, sizeof(FlashId_t));

	uint8 checksum = calChecksum((uint8 *)&info_b, LEN_FLASH_ID);
	BOOL b_valid = (checksum == info_b.checksum) ? TRUE : FALSE;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGD_ADDR, flash,
			 LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashId_t info_d;
	memcpy((uint8 *)&info_d, flash, sizeof(FlashId_t));

	checksum = calChecksum((uint8 *)&info_d, LEN_FLASH_ID);
	BOOL d_valid = (checksum == info_d.checksum) ? TRUE : FALSE;

	if (b_valid == TRUE && d_valid == TRUE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_b, sizeof(FlashId_t));
	} else if (b_valid == TRUE && d_valid == FALSE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_b, sizeof(FlashId_t));
		saveID(pInfo);
	} else if (b_valid == FALSE && d_valid == TRUE) {
		memcpy((uint8 *)pInfo, (uint8 *)&info_d, sizeof(FlashId_t));
		saveID(pInfo);
	} else {
		memcpy((uint8 *)pInfo, (uint8 *)&info_b, sizeof(FlashId_t));
		printf("ID: both side are false\n");
		// 두개의 영역이 모두 깨지더라도 Serial Number는 지우지 않음.
#if 0
        memset((uint8 *)pInfo, 0xff, sizeof(FlashId_t));
        memset(pInfo->serialNum, '0', SERIAL_NUM_LEN);

        saveID(pInfo);
#endif
	}
}

#if REPEATER_DEVICE
int getFirstZeroPosition(uint32 nwk_addr)
{
	uint8 temp[4];
	int firstZero = 0;
	memcpy(temp, &nwk_addr, 4);

	for (int i = 0; i < 4; i++) {
		if (temp[i]) {
			break;
		}
		firstZero = i;
	}
	return firstZero;
}
#endif

void FLASH_saveConfigInfo(Config_t *config)
{
	// save serial number
	FlashId_t flashID;

#if REPEATER_DEVICE
	memcpy(&flashID.pan_id, &config->pan_id, 2);
	memcpy(&flashID.nwk_addr, &config->nwk_addr, 4);

	int zeroPos = getFirstZeroPosition(config->nwk_addr);
	uint8 addr[4];
	memcpy(addr, &config->slaveNwk, 4);
	flashID.slaveId = addr[zeroPos];
#endif

	memcpy(flashID.serialNum, config->serialNum, SERIAL_NUM_LEN);
#if LORA_DEVICE
	memcpy(flashID.devEui, config->devEui, 8);
#else // NBIOT_DEVICE
	memcpy(flashID.imei, config->imei, 8);
#endif

	flashID.checksum = calChecksum((uint8 *)&flashID, LEN_FLASH_ID);

	saveID(&flashID);

	// save others
	FlashConfig_t flashConfig;

	flashConfig.meterType = config->meterType;
	/* Remove sleepMode */
	/* flashConfig.sleepMode = config->sleepMode; */
	flashConfig.rtCtrlMode = config->rtCtrlMode;
	flashConfig.meterInterval = config->meterInterval;
	flashConfig.reportInterval = config->reportInterval;
#if NBIOT_DEVICE
	MISC_ipToHexArray(config->serverIp, flashConfig.serverIp);
	memcpy(flashConfig.serverPort, &config->serverPort, 2);
	MISC_ipToHexArray(config->fotaIp, flashConfig.fotaIp);
	memcpy(flashConfig.fotaPort, &config->fotaPort, 2);
	flashConfig.fotaInterval = config->fotaInterval;
	memcpy(flashConfig.serviceCode, &config->serviceCode, 4);
	flashConfig.isModemInit = config->isModemInit;
	flashConfig.isUsingPF = config->isUsingPF;
#endif
	flashConfig.isShortInterval = config->isShortInterval;
	flashConfig.debugPrint = config->debugPrint;

	memcpy(flashConfig.resetCount, &config->resetCount, 2);

	Date_t date;
	RTC_read(&date);

	flashConfig.checksum =
		calChecksum((uint8 *)&flashConfig, LEN_FLASH_CONFIG);
	saveConfig(&flashConfig);
}

void FLASH_readConfigInfo(Config_t *config)
{
	config->termModel = MISC_getDeviceType();

	// read others
	FlashConfig_t flashConfig;
	readConfig(&flashConfig);

	config->meterType = flashConfig.meterType;
	config->meterInterval = flashConfig.meterInterval;
	config->reportInterval = flashConfig.reportInterval;

#if NBIOT_DEVICE
	sprintf(config->serverIp, "%d.%d.%d.%d", flashConfig.serverIp[0],
		flashConfig.serverIp[1], flashConfig.serverIp[2],
		flashConfig.serverIp[3]);
	memcpy(&config->serverPort, flashConfig.serverPort, 2);
	sprintf(config->fotaIp, "%d.%d.%d.%d", flashConfig.fotaIp[0],
		flashConfig.fotaIp[1], flashConfig.fotaIp[2],
		flashConfig.fotaIp[3]);
	memcpy(&config->fotaPort, flashConfig.fotaPort, 2);
	config->fotaInterval = flashConfig.fotaInterval;
	memcpy(config->serviceCode, flashConfig.serviceCode, 4);
	config->serviceCode[4] = 0;
	config->isModemInit = flashConfig.isModemInit;
	config->isUsingPF = flashConfig.isUsingPF;
#endif
	/* Remove sleepMode */
	/* config->sleepMode = flashConfig.sleepMode; */
	config->rtCtrlMode = flashConfig.rtCtrlMode;
	config->isShortInterval = flashConfig.isShortInterval;
	config->debugPrint = flashConfig.debugPrint;

	memcpy(&config->resetCount, flashConfig.resetCount, 2);

	// read serial number
	FlashId_t flashID;
	readID(&flashID);

#if REPEATER_DEVICE
	memcpy(&config->pan_id, &flashID.pan_id, 2);
	memcpy(&config->nwk_addr, &flashID.nwk_addr, 4);

	int zeroPos = getFirstZeroPosition(flashID.nwk_addr);
	uint8 addr[4];
	memcpy(addr, &flashID.nwk_addr, 4);
	addr[zeroPos] = flashID.slaveId;
	memcpy(&config->slaveNwk, addr, 4);
#endif

#if LORA_DEVICE
	memcpy(config->devEui, flashID.devEui, 8);
#else // NBIOT_DEVICE
	memcpy(config->imei, flashID.imei, 8);
#endif

	memcpy(config->serialNum, flashID.serialNum, SERIAL_NUM_LEN);

	int serialBase = MISC_asciiToHex(flashID.serialNum[8]) * 1000 +
			 MISC_asciiToHex(flashID.serialNum[9]) * 100 +
			 MISC_asciiToHex(flashID.serialNum[10]) * 10 +
			 MISC_asciiToHex(flashID.serialNum[11]);

	int nSpread = config->reportInterval * 50;

	config->reportSec =
		((serialBase / nSpread) % 4) *
		15; // 같은 '분'에 보고하는 단말기들간에 시간을 15초 단위로 4개로 나눔
	serialBase %= nSpread;

	config->reportHour = (config->isUsingPF == TRUE) ? 0 : serialBase / 50;
	config->reportMin = (serialBase % 50) + 5;
}

/**************************************************
 * Flash functions for Meter data
 **************************************************/

static void readSectorData(uint8 *pData, int sector)
{
	uint8 flash[LEN_INFO_FLASH_SECTOR];
	uint8 *address = (uint8 *)FlashAddr[sector];

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);

	MSP430FLASH_read(address, flash, LEN_INFO_FLASH_SECTOR);

	HAL_EXIT_CRITICAL_SECTION(intState);

	memcpy(pData, flash + POS_METER_REPORT,
	       sizeof(FlashMeterData_t) * NUM_FLASH_DATA_PER_SECTOR);
}

static void writeSectorData(uint8 *pData, int sector)
{
	uint8 flash[LEN_INFO_FLASH_SECTOR];
	uint8 *address = (uint8 *)FlashAddr[sector];

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);

	BOOL result = FALSE;
	if (MSP430FLASH_read(address, flash, LEN_INFO_FLASH_SECTOR)) {
		memcpy(flash + POS_METER_REPORT, pData,
		       sizeof(FlashMeterData_t) * NUM_FLASH_DATA_PER_SECTOR);
		if (MSP430FLASH_erasePage(address)) {
			if (MSP430FLASH_write(address, flash,
					      LEN_INFO_FLASH_SECTOR)) {
				result = TRUE;
			}
		}
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	if (!result) {
		OSAL_setEvent(APP_getTaskId(), APP_EVENT_REBOOT);
	}
}

static void readOneData(FlashMeterData_t *pData, int num)
{
	uint8 sector = num / NUM_FLASH_DATA_PER_SECTOR;
	uint8 pos = num % NUM_FLASH_DATA_PER_SECTOR;

	uint8 data[LEN_INFO_FLASH_SECTOR];
	memset(data, 0, sizeof(data));

	readSectorData(data, sector);
	memcpy((uint8 *)pData, data + (pos * sizeof(FlashMeterData_t)),
	       sizeof(FlashMeterData_t));
}

static void deleteOneData(int num)
{
	uint8 sector = num / NUM_FLASH_DATA_PER_SECTOR;
	uint8 pos = num % NUM_FLASH_DATA_PER_SECTOR;

	uint8 data[LEN_INFO_FLASH_SECTOR];
	memset(data, 0, sizeof(data));

	readSectorData(data, sector);
	memset(data + (pos * sizeof(FlashMeterData_t)), 0,
	       sizeof(FlashMeterData_t));

	writeSectorData(data, sector);
}

static void writeOneData(int num, FlashMeterData_t *pData)
{
	uint8 sector = num / NUM_FLASH_DATA_PER_SECTOR;
	uint8 pos = num % NUM_FLASH_DATA_PER_SECTOR;

	// printf("writeOneData - num(%d) sector(%d) pos(%d)\n", num, sector, pos);

	uint8 data[LEN_INFO_FLASH_SECTOR];
	memset(data, 0, sizeof(data));

	readSectorData(data, sector);
	memcpy(data + (pos * sizeof(FlashMeterData_t)), (uint8 *)pData,
	       sizeof(FlashMeterData_t));

	writeSectorData(data, sector);
}

static uint32 date2int(FlashMeterData_t *p)
{
	if (p->year > 100 || p->mon > 12 || p->day > 31 || p->hour > 23) {
		return 0;
	}

	uint32 time = p->year * 0x1000000 + p->mon * 0x10000 + p->day * 0x100 +
		      p->hour;
	return time;
}

static int getWritePosition(int *pSecondhalf)
{
	// 시간이 0이 아니면서 가장 늦은, 즉 시간 값의 크기가 가장 큰 영역을 찾음
	//  - 못 찾으면, 모두가 비어 있는 즉, 초기화 된 상태이므로 0번의 firstValue로
	//  기록
	//  - 찾았는데 secondHalf가 비어 있으면(즉, hourInterval이 0이면) 그 영역에
	//  secondValue로 기록
	//  -          secondHalf가 기록되어 있으면 다음 영역에 firstValue로 기록

	uint32 lastTime = 0;
	int writePos = -1;
	FlashMeterData_t lastData;

	for (int sector = 0; sector < 4; sector++) {
		uint8 data[LEN_INFO_FLASH_SECTOR];
		memset(data, 0, sizeof(data));

		readSectorData(data, sector);

		for (int pos = 0; pos < 8; pos++) {
			FlashMeterData_t *p =
				(FlashMeterData_t
					 *)(data +
					    pos * sizeof(FlashMeterData_t));
			uint32 dataTime = date2int(p);
			if (dataTime != 0 && lastTime < dataTime) {
				lastTime = dataTime;
				memcpy(&lastData, p, sizeof(FlashMeterData_t));
				writePos = sector * 8 + pos;
			}
			p++;
		}
	}

	if (writePos < 0) {
		// 모두가 비어 있는 영역이므로 그 자리에 첫번째 데이터를 기록하면 됨
		writePos = 0;
		*pSecondhalf = 0;
	} else {
		if (lastData.hourInterval == HOUR_INTERVAL_NO_DATA) {
			// 두번째 데이터가 비어 있음
			*pSecondhalf = 1;
		} else {
			// 다음 영역으로 이동하고 첫번째에 기록하도록 함.
			if (++writePos >= NUM_TOTAL_FLASH_DATA) {
				writePos = 0;
			}
			*pSecondhalf = 0;
		}
	}

	// printf("(getWritePosition) writePos:%d, secondHalf:%d\n", writePos,
	// *pSecondHalf);

	return writePos;
}

static int getReadPosition(int *pDataNum)
{
	// 현재 저장된 데이터 개수 및 가장 마지막에 저장된 데이터 위치를 계산.

	uint32 leadingTime = 0xFFFFFFFF;
	int readPos = -1;
	int nData = 0;

	for (int sector = 0; sector < 4; sector++) {
		uint8 data[LEN_INFO_FLASH_SECTOR];
		memset(data, 0, sizeof(data));

		readSectorData(data, sector);

		for (int pos = 0; pos < 8; pos++) {
			FlashMeterData_t *p =
				(FlashMeterData_t
					 *)(data +
					    pos * sizeof(FlashMeterData_t));
			uint32 dataTime = date2int(p);
			if (dataTime != 0) {
				nData++;
				if (p->hourInterval != HOUR_INTERVAL_NO_DATA) {
					nData++;
				}

				if (leadingTime > dataTime) {
					leadingTime = dataTime;
					readPos = sector * 8 + pos;
				}
			}
		}
	}

	// printf("(flash_getReadPosition) readPos:%d, nData:%d\n", readPos, nData);

	*pDataNum = nData;
	return readPos;
}

void FLASH_printData()
{
	int nSavedData = 0;
	int firstPos = getReadPosition(&nSavedData);
	if (firstPos < 0) {
		printf("no data saved\n");
		return;
	}

	printf("%d data stored\n", nSavedData);

	for (int num = 0; num < NUM_TOTAL_FLASH_DATA; num++) {
		int pos = firstPos + num;
		if (pos >= NUM_TOTAL_FLASH_DATA) {
			pos -= NUM_TOTAL_FLASH_DATA;
		}

		FlashMeterData_t fData;
		memset(&fData, 0, sizeof(FlashMeterData_t));
		readOneData(&fData, pos);

		if (fData.year == 0 || fData.year == 255) {
			continue;
		}

		printf("%2d [%d:%d] %04d-%02d-%02d %02dH %02X%02X%02X%02X %d.%dV  ",
		       num, pos / 8, pos % 8, fData.year + 2000, fData.mon,
		       fData.day, fData.hour, fData.firstValue[0],
		       fData.firstValue[1], fData.firstValue[2],
		       fData.firstValue[3], fData.batt / 10, fData.batt % 10);

		if (fData.hourInterval != HOUR_INTERVAL_NO_DATA) {
			// Diff를 측정값으로 표현하려면 Fitst 값이 All FFh인지 확인 후 숫자 값으로
			// 변환하여 합산해야 함. 단순 디버깅 출력을 복잡하게 할 필요 없다고
			// 생각되어 Diff값만 출력함.
			uint16 diff = 0;
			memcpy(&diff, fData.diff, 2);

			Date_t date2;
			date2.year = fData.year + 2000;
			date2.mon = fData.mon;
			date2.day = fData.day;
			date2.hour = fData.hour;
			date2.min = date2.sec = 0;

			int hourDiff = fData.hourInterval;
			do {
				if (hourDiff >= 24)
					MISC_incDateHour(&date2, 24);
				else
					MISC_incDateHour(&date2, hourDiff);

				hourDiff =
					(hourDiff >= 24) ? (hourDiff - 24) : 0;
			} while (hourDiff > 0);

			printf("|  %04d-%02d-%02d %02dH DIFF(%04X)", date2.year,
			       date2.mon, date2.day, date2.hour, diff);
		}

		printf("\n");
	}
}

void FLASH_initData()
{
	// Clear garbage data (include broken data)
	for (int num = 0; num < NUM_TOTAL_FLASH_DATA; num++) {
		FlashMeterData_t data;
		readOneData(&data, num);
		if (data.year != 0) {
			if (data.year < 17 || data.year > 99 || data.mon < 1 ||
			    data.mon > 12 || data.day < 1 || data.day > 31 ||
			    data.hour >= 24) {
				// invalid date/time
				deleteOneData(num);
			}
		}
	}
}

void FLASH_deleteAllData()
{
	for (int num = 0; num < NUM_TOTAL_FLASH_DATA; num++) {
		deleteOneData(num);
	}
}

void FLASH_saveData(Date_t *pDate, uint8 batt, uint8 *pValue)
{
#if 0
    printf("[FLASH_saveData] %04d-%02d-%02d %02dH %02x%02x%02x%02x %d.%dV\n",
            pDate->year, pDate->mon, pDate->day, pDate->hour,
            pValue[0], pValue[1], pValue[2], pValue[3], batt/10, batt%10);
#endif

	int secondHalf = 0;
	int pos = getWritePosition(&secondHalf);

	FlashMeterData_t flashData;
	memset(&flashData, 0, sizeof(flashData));

	if (secondHalf == 0) {
		memcpy(flashData.firstValue, pValue, 4);
		date2FlashDate(pDate, &flashData);
		flashData.batt = batt;
		flashData.hourInterval = 0;
	} else {
		readOneData(&flashData, pos);

		uint16 diff = 0;
		BOOL is1stMtDown = !isBcd(flashData.firstValue, 4);
		BOOL is2ndtMtDown = !isBcd(pValue, 4);
		if (is2ndtMtDown) {
			diff = METER_DIFF_2ND_MT_DOWN;
		} else {
			if (is1stMtDown) {
				diff = METER_DIFF_2ND_SET_TO_FIRST;
				memcpy(flashData.firstValue, pValue, 4);
			} else {
				uint32 firstValue = 0;
				MISC_bcdToInt(flashData.firstValue, &firstValue,
					      4);
				uint32 secondValue = 0;
				MISC_bcdToInt(pValue, &secondValue, 4);

				if (firstValue > secondValue) {
					// 1st value > 2nd value 인 경우 아래와 같은 이유로 MT_DOWN으로
					// 처리한다.
					// 1. Diff값은 unsigned이므로 표현할 방법이 없음.(Signed로 사용할 경우
					// 최대 검침값이 감소.)
					// 2. 주로 계량기 교체시 발생하며, 교체 후 최초 값이 이전 2개의
					// 검침값보다 중요한 데이터는 아님.
					// 3. 다른 방식으로 해당 상황을 표시하더라도 결국 2nd 데이터는
					// 버려지므로 필요성이 희박.
					diff = METER_DIFF_2ND_MT_DOWN;
				} else {
					diff = secondValue - firstValue;
					if (diff > METER_DIFF_MAX_VALUE) {
						diff = METER_DIFF_MAX_VALUE;
					}
				}
			}
		}
		memcpy(flashData.diff, &diff, 2);

		Date_t prevDate;
		flashDate2Date(&flashData, &prevDate);

		// 2nd data empty여부를 확인하는 조건이며, 'secondHalf'으로 판단되지만
		// 정확한 처리를 위해 추가적으로 확인한다.
		int hourInterval = flashData.hourInterval;
		if (hourInterval == HOUR_INTERVAL_NO_DATA) {
			hourInterval = MISC_calcHourDiff(&prevDate, pDate);
			if (hourInterval > HOUR_INTERVAL_MAX_VALUE) {
				hourInterval = HOUR_INTERVAL_MAX_VALUE;
			}
		}
		flashData.hourInterval = hourInterval;
	}

	writeOneData(pos, &flashData);
}

int FLASH_readData(FlashMeterData_t *fData, uint8 num)
{
	readOneData(fData, num);

	return 1;
}

// See function description comment in Hearder file
FlashMeterData_t *FLASH_readPeriodData(Date_t *fromDate, Date_t *toDate,
				       int *totalNumOfDatas)
{
	// 1. Read all flash data
	FlashMeterData_t pAllDatas[NUM_TOTAL_FLASH_DATA];
	for (int sector = 0; sector < NUM_FLASH_SECTOR; sector++) {
		readSectorData(
			(uint8 *)&pAllDatas[sector * NUM_FLASH_DATA_PER_SECTOR],
			sector);
	}

	// 2. find period data pos and sum total valid data.
	Date_t date;
	uint32 oldestDataTime = 0xFFFFFFFF;
	uint16 oldestDataPos = 0;
	uint16 totalDatas = 0;
	BOOL isValidData[NUM_TOTAL_FLASH_DATA];
	for (int pos = 0; pos < NUM_TOTAL_FLASH_DATA; pos++) {
		// Clear valid data flag
		isValidData[pos] = FALSE;
		// Check flash data validation using Date.
		uint32 dataTime = date2int(&pAllDatas[pos]);
		if (dataTime == 0) {
			continue; // Ignore invalid data.
		}

		flashDate2Date(&pAllDatas[pos], &date);

		// 1st or 2nd Data 둘중 하나만 기간내 데이터인 경우 Valid data로 간주한다.
		if (MISC_calcSecDiff(fromDate, &date) >= 0 &&
		    MISC_calcSecDiff(&date, toDate) >= 0) {
			isValidData[pos] = TRUE;
		} else {
			if (pAllDatas[pos].hourInterval) {
				MISC_incDateHour(&date,
						 pAllDatas[pos].hourInterval);
				if (MISC_calcSecDiff(fromDate, &date) >= 0 &&
				    MISC_calcSecDiff(&date, toDate) >= 0) {
					isValidData[pos] = TRUE;
				}
			}
		}

		// Update totla data counts and oldest valid data pos
		if (isValidData[pos] == TRUE) {
			totalDatas++;
			if (dataTime < oldestDataTime) {
				oldestDataTime = dataTime;
				oldestDataPos = pos;
			}
		}
	}

	// 3. malloc data memory and copy data.
	if (totalDatas > 0) {
		FlashMeterData_t *retData = (FlashMeterData_t *)malloc(
			sizeof(FlashMeterData_t) * totalDatas);

		// copy period data to allocated heap memory
		int retDataPos = 0;
		while (retDataPos < totalDatas) {
			// 한번에 Data를 Copy할 수 있으나, Flash data가 동작 도중 깨지는 상황을
			// 고려하여 Loopt안에서 Valid여부를 확인하며 Copy한다.
			if (isValidData[oldestDataPos] == TRUE) {
				memcpy(&retData[retDataPos],
				       &pAllDatas[oldestDataPos],
				       sizeof(FlashMeterData_t));
				retDataPos++;
			}

			if (++oldestDataPos >= NUM_TOTAL_FLASH_DATA) {
				oldestDataPos = 0;
			}
		}

		*totalNumOfDatas = totalDatas;
		return retData;
	} else {
		*totalNumOfDatas = 0;
		return NULL;
	}
}

void FLASH_updateResetCount(Config_t *config)
{
	if (config->resetCount < 65000) {
		config->resetCount++;
		FLASH_saveConfigInfo(config);
	}
}
