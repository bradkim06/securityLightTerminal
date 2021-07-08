#include "flash.h"
#include "flash_msp430.h"

#include "app.h"
#include "common.h"
#include "device.h"
#include "meter.h"
#include "misc.h"
#include "osal.h"
#include "rtc.h"

const uint8 *FlashAddr[NUM_FLASH_SECTOR] = { (uint8 *)FLASH_SEGA_ADDR, (uint8 *)FLASH_SEGB_ADDR,
					     (uint8 *)FLASH_SEGC_ADDR, (uint8 *)FLASH_SEGD_ADDR };

static uint8 calChecksum(uint8 *p, int len)
{
	uint8 checksum = 0;
	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
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
	MSP430FLASH_read((uint8 *)FLASH_SEGA_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashConfig_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGA_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGA_ADDR, flash, LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	MSP430FLASH_read((uint8 *)FLASH_SEGC_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashConfig_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGC_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGC_ADDR, flash, LEN_INFO_FLASH_SECTOR)) {
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
	MSP430FLASH_read((uint8 *)FLASH_SEGA_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashConfig_t info_a;
	memcpy((uint8 *)&info_a, flash, sizeof(FlashConfig_t));

	uint8 checksum = calChecksum((uint8 *)&info_a, sizeof(FlashConfig_t) - 1);
	BOOL a_valid = (checksum == info_a.checksum) ? TRUE : FALSE;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGC_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashConfig_t info_c;
	memcpy((uint8 *)&info_c, flash, sizeof(FlashConfig_t));
	checksum = calChecksum((uint8 *)&info_c, sizeof(FlashConfig_t) - 1);
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

#if NBIOT_DEVICE
		MISC_ipToHexArray((char *)ONEM2M_FOTA_SERVER_IP, pInfo->fotaIp);
		pInfo->fotaPort[0] = LO_UINT16(ONEM2M_FOTA_SERVER_PORT);
		pInfo->fotaPort[1] = HI_UINT16(ONEM2M_FOTA_SERVER_PORT);
		pInfo->fotaInterval = ONEM2M_FOTA_DAY_INTERVAL;
#endif

		pInfo->meterType = E_METER;
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
	MSP430FLASH_read((uint8 *)FLASH_SEGB_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashId_t));

	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGB_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGB_ADDR, flash, LEN_INFO_FLASH_SECTOR)) {
			result = TRUE;
		}
	}

	MSP430FLASH_read((uint8 *)FLASH_SEGD_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	memcpy(flash, (uint8 *)pInfo, sizeof(FlashId_t));
	if (MSP430FLASH_erasePage((uint8 *)FLASH_SEGD_ADDR)) {
		if (MSP430FLASH_write((uint8 *)FLASH_SEGD_ADDR, flash, LEN_INFO_FLASH_SECTOR)) {
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
	MSP430FLASH_read((uint8 *)FLASH_SEGB_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashId_t info_b;
	memcpy((uint8 *)&info_b, flash, sizeof(FlashId_t));

	uint8 checksum = calChecksum((uint8 *)&info_b, sizeof(FlashId_t) - 1);
	BOOL b_valid = (checksum == info_b.checksum) ? TRUE : FALSE;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_read((uint8 *)FLASH_SEGD_ADDR, flash, LEN_INFO_FLASH_SECTOR);
	HAL_EXIT_CRITICAL_SECTION(intState);

	FlashId_t info_d;
	memcpy((uint8 *)&info_d, flash, sizeof(FlashId_t));

	checksum = calChecksum((uint8 *)&info_d, sizeof(FlashId_t) - 1);
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

void FLASH_saveConfigInfo(Config_t *config)
{
	// save serial number
	FlashId_t flashID;

	memcpy(flashID.serialNum, config->serialNum, SERIAL_NUM_LEN);
	memcpy(flashID.imei, config->imei, 8);
	memcpy(flashID.imsi, config->imsi, 8);

	flashID.checksum = calChecksum((uint8 *)&flashID, sizeof(FlashId_t) - 1);

	saveID(&flashID);

	// save others
	FlashConfig_t flashConfig;

	flashConfig.meterType = config->meterType;
	flashConfig.reportInterval = config->reportInterval;
	MISC_ipToHexArray(config->serverIp, flashConfig.serverIp);
	memcpy(flashConfig.serverPort, &config->serverPort, 2);
	MISC_ipToHexArray(config->fotaIp, flashConfig.fotaIp);
	memcpy(flashConfig.fotaPort, &config->fotaPort, 2);
	flashConfig.fotaInterval = config->fotaInterval;
	memcpy(flashConfig.serviceCode, &config->serviceCode, 4);
	flashConfig.isModemInit = config->isModemInit;
	flashConfig.isShortInterval = config->isShortInterval;
	flashConfig.debugPrint = config->debugPrint;

	memcpy(flashConfig.resetCount, &config->resetCount, 2);

	Date_t date;
	RTC_read(&date);

	flashConfig.checksum = calChecksum((uint8 *)&flashConfig, sizeof(FlashConfig_t) - 1);
	saveConfig(&flashConfig);
}

void FLASH_readConfigInfo(Config_t *config)
{
	config->termModel = MISC_getDeviceType();

	// read others
	FlashConfig_t flashConfig;
	readConfig(&flashConfig);

	config->meterType = flashConfig.meterType;
	config->reportInterval = flashConfig.reportInterval;

	sprintf(config->serverIp, "%d.%d.%d.%d", flashConfig.serverIp[0], flashConfig.serverIp[1],
		flashConfig.serverIp[2], flashConfig.serverIp[3]);
	memcpy(&config->serverPort, flashConfig.serverPort, 2);
	sprintf(config->fotaIp, "%d.%d.%d.%d", flashConfig.fotaIp[0], flashConfig.fotaIp[1],
		flashConfig.fotaIp[2], flashConfig.fotaIp[3]);
	memcpy(&config->fotaPort, flashConfig.fotaPort, 2);
	config->fotaInterval = flashConfig.fotaInterval;
	memcpy(config->serviceCode, flashConfig.serviceCode, 4);
	config->serviceCode[4] = 0;
	config->isModemInit = flashConfig.isModemInit;
	config->isShortInterval = flashConfig.isShortInterval;
	config->debugPrint = flashConfig.debugPrint;

	memcpy(&config->resetCount, flashConfig.resetCount, 2);

	// read serial number
	FlashId_t flashID;
	readID(&flashID);

	memcpy(config->imei, flashID.imei, 8);
	memcpy(config->imsi, flashID.imsi, 8);

	memcpy(config->serialNum, flashID.serialNum, SERIAL_NUM_LEN);

	int serialBase = MISC_asciiToHex(flashID.serialNum[8]) * 1000 +
			 MISC_asciiToHex(flashID.serialNum[9]) * 100 +
			 MISC_asciiToHex(flashID.serialNum[10]) * 10 +
			 MISC_asciiToHex(flashID.serialNum[11]);

	int nSpread = config->reportInterval * 50;

	config->reportSec = ((serialBase / nSpread) % 4) *
			    15; // 같은 '분'에 보고하는 단말기들간에 시간을 15초 단위로 4개로 나눔
	serialBase %= nSpread;

	config->reportHour = serialBase / 50;
	config->reportMin = (serialBase % 50) + 5;

	config->meterType = E_METER;
	config->lightOnHour = 18;
	config->lightOnMin = 44;
	config->lightOffHour = 6;
	config->lightOffMin = 32;
	config->debugPrint = TRUE;
}

void FLASH_updateResetCount(Config_t *config)
{
	if (config->resetCount < 65000) {
		config->resetCount++;
		FLASH_saveConfigInfo(config);
	}
}
