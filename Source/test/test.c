#include "test.h"
#include "modem_test.h"
#include "shell.h"

#include "alarm.h"
#include "app.h"
#include "battery.h"
#include "common.h"
#include "device.h"
#include "flash.h"
#include "flash_msp430.h"
#include "lcd.h"
#include "metering.h"
#include "misc.h"
#include "modem.h"
#include "nfc.h"
#include "rtc.h"
#include "task_mgr.h"
#include "timer.h"
#include "uart.h"

#if NBIOT_DEVICE
#include "uuid.h"
#endif

static BOOL IsTestMode = FALSE;
static BOOL IsMeterTesting = FALSE;
static BOOL IsReedTesting = FALSE;
static uint8 TestTaskId = 0;
/*
 ***************************************************************
    Test Mode Control functions
 ***************************************************************
 */
BOOL TEST_checkTestMode()
{
	// P1.0 --> if 0 with inernal pull-up, test mode
	//          else normal mode

	P1DIR &= ~BM(TEST_MODE_DETECT_P1_PIN);
	P1REN &= ~BM(TEST_MODE_DETECT_P1_PIN);

	MISC_delayUs(1000); // specific case need this delay

	IsTestMode = (P1IN & BM(TEST_MODE_DETECT_P1_PIN)) ? FALSE : TRUE;

	return IsTestMode;
}

BOOL TEST_isTestMode()
{
	return IsTestMode;
}

BOOL TEST_stop()
{
	uint8 c = SHELL_getChar();
	if (c == 'q' || c == 'Q') {
		return TRUE;
	} else {
		return FALSE;
	}
}

uint8 TEST_getTaskId()
{
	return TestTaskId;
}

/*
 ***************************************************************
    Other functions
 ***************************************************************
 */
static BOOL saveTerminalConfig(const char *sn, const int ri)
{
	BOOL valid = FALSE;

	do {
		if (strlen(sn) != SERIAL_NUM_LEN) {
			printf("length of s/n should be %d bytes\n",
			       SERIAL_NUM_LEN);
			break;
		}
		if (ri <= 0 || ri > 24 || (24 % ri) != 0) {
			printf("wrong report interval(%d)\n", ri);
			break;
		}
		valid = TRUE;
	} while (0);

	if (valid) {
		FLASH_readConfigInfo(&conf);

		memcpy(conf.serialNum, sn, SERIAL_NUM_LEN);
		conf.reportInterval = ri;
		conf.isShortInterval = 0;
		conf.isModemInit = FALSE;
		// Set default FOTA IP/Port and interval
		strcpy(conf.fotaIp, ONEM2M_FOTA_SERVER_IP);
		conf.fotaPort = ONEM2M_FOTA_SERVER_PORT;
		conf.fotaInterval = ONEM2M_FOTA_DAY_INTERVAL;

		FLASH_saveConfigInfo(&conf);
	}

	return valid;
}

void TEST_config()
{
	APP_showConfig(&conf);
}

void TEST_debugPrint(int flag)
{
	if (flag != 0xff) {
		FLASH_readConfigInfo(&conf);
		if (flag) {
			conf.debugPrint = 1;
		} else {
			conf.debugPrint = 0;
		}
		FLASH_saveConfigInfo(&conf);
	}
	printf("debug printing %s\n", conf.debugPrint ? "on" : "off");
}

void TEST_meteringParamer(int ri)
{
	if (ri == 0 || 24 % ri) {
		printf("reportInterval should be one of 1, 2, 3, 4, 6, 12, 24\n");
		return;
	}

	FLASH_readConfigInfo(&conf);

	conf.reportInterval = ri;

	FLASH_saveConfigInfo(&conf);

	printf("reportInterval:%d, reportHour:%d\n", conf.reportInterval,
	       conf.reportHour);
}

void TEST_readRTC(int fromJig)
{
	Date_t date;

	RTC_read(&date);
	if (fromJig) {
		printf("trrtc %04d-%02d-%02d %02d:%02d:%02d\n", date.year,
		       date.mon, date.day, date.hour, date.min, date.sec);
	} else {
		printf("%04d-%02d-%02d %02d:%02d:%02d\n", date.year, date.mon,
		       date.day, date.hour, date.min, date.sec);
	}
}

void TEST_setRTC(int fromJig, int year, int mon, int day, int hour, int min,
		 int sec)
{
	Date_t date;

	date.year = year;
	date.mon = mon;
	date.day = day;
	date.hour = hour;
	date.min = min;
	date.sec = sec;

	RTC_set(&date);

	TEST_readRTC(fromJig);
}

void TEST_readFlash(int sector)
{
	uint8 flash[LEN_INFO_FLASH_SECTOR];

	if ((sector < 0 || sector > 3) && sector != 0xff) {
		printf("Invalid sector number[%d]\n", sector);
		return;
	}

	halIntState_t intState;
	uint8 start, end;

	if (sector == 0xff) {
		start = 0, end = 4;
	} else {
		start = sector, end = sector + 1;
	}

	for (int i = start; i < end; i++) {
		HAL_ENTER_CRITICAL_SECTION(intState);
		MSP430FLASH_read((uint8 *)FlashAddr[i], flash,
				 LEN_INFO_FLASH_SECTOR);
		HAL_EXIT_CRITICAL_SECTION(intState);

		printf("\n======== Flash Memory - Sector[%d] =======\n", i);
		for (int j = 0; j < LEN_INFO_FLASH_SECTOR; j++) {
			if (j && !(j % 16)) {
				printf("\n");
			}
			printf("%02X ", flash[j]);
		}
		printf("\n");
	}
}

void TEST_eraseFlash(int sector)
{
	if ((sector < 0 || sector > 3)) {
		printf("Invalid sector number[%d]\n", sector);
		return;
	}

	halIntState_t intState;

	HAL_ENTER_CRITICAL_SECTION(intState);
	MSP430FLASH_erasePage((uint8 *)FlashAddr[sector]);
	HAL_EXIT_CRITICAL_SECTION(intState);
}

void TEST_readBattery()
{
	uint16 battery;

	printf("ACK\n");
	BATT_init();

	while (1) {
		// 생산공정에서 모뎀부팅시간을 벌기위한 딜레이 5초
		uint32 startMs = TIMER_getMsec();
		do {
			if (TEST_stop() == TRUE) {
				return;
			}
		} while (TIMER_getMsecDiff(startMs) <= 1000);

		battery = BATT_getVoltage();
		printf("tbatt %d.%dV\n", battery / 10, battery % 10);
	}
}

void TEST_readSerialNum()
{
	printf("trsn ");
	for (int i = 0; i < SERIAL_NUM_LEN; i++) {
		printf("%c", conf.serialNum[i]);
	}
	printf(" %s\n", APP_getFwVer(conf.isUsingPF));
}

void TEST_writeSerialNum(char *serialNum)
{
	if (strlen(serialNum) != SERIAL_NUM_LEN) {
		printf("serial number must be %d digits\n", SERIAL_NUM_LEN);
		return;
	}

	FLASH_readConfigInfo(&conf);

	for (int i = 0; i < SERIAL_NUM_LEN; i++) {
		conf.serialNum[i] = *serialNum++;
	}

	FLASH_saveConfigInfo(&conf);

	TEST_readSerialNum();
}

static void reedResponse()
{
	if (IsReedTesting)
		printf("treed OK\n");
}

void TEST_readMeter(int type, int sec)
{
	int meterType;
	if (type == 1) {
		meterType = E_METER;
	} else {
		printf("rmeter [TYPE] [TIME(sec)]\n");
		printf("First Arg Meter type, see below\n");
		printf(" 1 : Electric STD\n");
		printf("Second Arg Time(Sec)\n");
		return;
	}

	int nLoop = 0;
	while (++nLoop) {
		printf("Meter Send Req Count : %d Result : %d set Time : %d\n",
		       nLoop, METER_sendRequest(meterType), sec);

		IsMeterTesting = TRUE;
		uint32 ms = TIMER_getMsec();
		while (TIMER_getMsecDiff(ms) < (2 * 1000)) {
			if (TEST_stop()) {
				goto TEST_RMETER_FINISH;
			} else {
				TEST_runTasks();
			}
			if (!IsMeterTesting)
				break;
		}

		int result;
		MeterUnitData_t unit;
		memset(&unit, 0xFF, sizeof(MeterUnitData_t));
		if (IsMeterTesting) {
			result = FAIL;
		} else {
			result = METER_recvResponse(meterType, &unit);
		}

		printf("result : %d\n", result);

		while (TIMER_getMsecDiff(ms) < (sec * 1000)) {
			if (TEST_stop()) {
				goto TEST_RMETER_FINISH;
			}
		}
	}

TEST_RMETER_FINISH:
	IsMeterTesting = FALSE;
}

void TEST_readFwVersion()
{
	printf("tver %s\n", APP_getFwVer(conf.isUsingPF));
}

void TEST_relayMode(int flag)
{
	uint32 ms = TIMER_getMsec();
	Relay_Status(flag);

	if (flag == RELAY_TOGGLE) {
		while (1) {
			if (TIMER_getMsecDiff(ms) > 10000) {
				ms = TIMER_getMsec();
				Relay_Status(flag);
			}

			if (TEST_stop()) {
				return;
			}
		}
	}
}

static volatile unsigned char *gpioPortSelect(int port, int isInput)
{
	volatile unsigned char *portAddress = &P1OUT;
	char pinSelect = (port - 1) / 2;

	for (int i = 0; i < pinSelect; i++) {
		portAddress += 0x20;
	}

	if ((port % 2) == 0) {
		portAddress++;
	}

	if (isInput) {
		portAddress -= 0x02;
	}

	return portAddress;
}

void TEST_gpioWrite(int port, int pin, int status)
{
#define OUTPUT 0
	volatile unsigned char *portAddress = gpioPortSelect(port, OUTPUT);

	printf("port(%d) pin(%d) status(0x%02X)\n", port, pin, portAddress);

	if (status) {
		*portAddress |= BM(pin);
	} else if (status == 0) {
		*portAddress &= ~BM(pin);
	}

	printf("P%dOUT address : 0x%02X, status : 0x%02X\n", port, portAddress,
	       *portAddress);
}

void TEST_gpioRead(int port)
{
#define INPUT 1
	volatile unsigned char *portAddress = gpioPortSelect(port, INPUT);

	printf("P%dIN address : 0x%02X, status : 0x%02X\n", port, portAddress,
	       *portAddress);
}

void TEST_shortInterval(int flag)
{
	if (flag != 0xff) {
		FLASH_readConfigInfo(&conf);
		conf.isShortInterval = flag ? 1 : 0;
		FLASH_saveConfigInfo(&conf);
	}

	printf("short interval %s\n", conf.isShortInterval ? "on" : "off");
	return;
}

void TEST_setMeterType(int type)
{
	FLASH_readConfigInfo(&conf);

	if (type == 1) {
		conf.meterType = E_METER;
	} else {
		printf("Meter type error, see below\n");
		printf(" 1 : Security Light\n");
		return;
	}

	FLASH_saveConfigInfo(&conf);
}

void TEST_pwsn(char *sn, int ri)
{
	if (saveTerminalConfig(sn, ri) != TRUE) {
		printf("trsn ERR\n");
		return;
	}

	TEST_prsn();
}

void TEST_prsn()
{
	char serial[SERIAL_NUM_LEN + 1] = "";
	memcpy(serial, conf.serialNum, SERIAL_NUM_LEN);
	serial[SERIAL_NUM_LEN] = '\0';

	printf("trsn %s %s %d\n", serial, APP_getFwVer(conf.isUsingPF),
	       conf.reportInterval);
}

void TEST_resetNFC()
{
	printf("ACK\n");

	NFC_tagDisable();
	NFC_fdDisable();

	// VCC control 사용전 보드 => 추후 삭제
	P1DIR |= BM(NFC_TAG_P1_PIN);
	P1OUT |= BM(NFC_TAG_P1_PIN);

	MISC_delayMs(
		1000); // 생산공정에서 모뎀 BIP완료 메세지 대기를 위한 딜레이
	if (NFC_factoryResetTag() == TRUE) {
		printf("tnfc OK\n");
	} else {
		printf("tnfc ERR\n");
	}

	// VCC control 사용전 보드 => 추후 삭제
	P1DIR &= ~BM(NFC_TAG_P1_PIN);
	NFC_init();
}

void TEST_checkNFC()
{
	printf("ACK\n");

	while (1) {
		if (TEST_stop() == TRUE) {
			break;
		}
	}
}

/*
 ***************************************************************
    Test process and task
 ***************************************************************
 */
static event32_t interruptTask(uint8 taskId, event32_t events)
{
	if (events & TEST_EVENT_SENSOR_REED) {
		reedResponse();
		return (events ^ TEST_EVENT_SENSOR_REED);
	}

	if (events & TEST_EVENT_MODEM_RX) {
		TEST_modemResponse();
		return (events ^ TEST_EVENT_MODEM_RX);
	}

	if (events & TEST_EVENT_METER_RX) {
		IsMeterTesting = FALSE;
		return (events ^ TEST_EVENT_METER_RX);
	}

	return 0;
}

const pTaskEventHandlerFn TestTasksArray[] = {
	interruptTask,
};

const uint8 testTasksCnt = sizeof(TestTasksArray) / sizeof(TestTasksArray[0]);
extern event32_t *TasksEvents;

void TEST_runTasks()
{
	uint8 idx = 0;

	WDTCTL = WDTPW | WDTCNTCL; // Clear watchdog timer
	WDTCTL = WDT_ARST_16SEC; // Start watchdog timer(16 sec)

	for (int i = 0; i < (testTasksCnt + 1); i++) {
		if (++idx >= testTasksCnt) {
			idx = 0;
		}

		do {
			if (TasksEvents[idx]) { // Task is highest priority that is ready.
				break;
			}
		} while (++idx < testTasksCnt);

		if (idx < testTasksCnt) {
			halIntState_t intState;
			event32_t events;

			HAL_ENTER_CRITICAL_SECTION(intState);
			events = TasksEvents[idx];
			TasksEvents[idx] = 0;
			HAL_EXIT_CRITICAL_SECTION(intState);

			// yikim 2017.09.06 NFC TEST중 발생하여 처리
			if (events == 0xFFFFFFFF) {
				__no_operation();
			} else {
				events = (TestTasksArray[idx])(idx, events);

				HAL_ENTER_CRITICAL_SECTION(intState);
				TasksEvents[idx] |= events;
				HAL_EXIT_CRITICAL_SECTION(intState);
			}
		}
	}
}

void TEST_run()
{
	WDTCTL = WDTPW | WDTCNTCL; // Clear watchdog timer
	WDTCTL = WDT_ARST_16SEC; // Start watchdog timer(16 sec)
	PRINT_enable();

	TasksEvents = (event32_t *)malloc(sizeof(event32_t) * testTasksCnt);
	if (TasksEvents == NULL) {
		printf("malloc error - TASKMGR_init\n");
		exit(1);
	}
	memset(TasksEvents, 0, (sizeof(event32_t) * testTasksCnt));

	BATT_init();
	NFC_init();
	METER_enable(E_METER);

	// task 초기화
	TestTaskId = 0;

	ALARM_init();
	FLASH_readConfigInfo(&conf);
	FLASH_updateResetCount(&conf);

	PRINT_stop();
	TEST_initModem();
	PRINT_resume();

	printf("\n\n\n\n");
	printf("===========================================\n");
	printf("             TEST MODE ");
	printf("(NB-IoT)\n");
	printf("   Compiled at (%s / %s)\n", __DATE__, __TIME__);
	printf("===========================================\n");
	printf("\n");

	SHELL_init();
	SHELL_run();

	printf("goto normal mode ... wait a moment\n");
	REBOOT_SYSTEM();
}
