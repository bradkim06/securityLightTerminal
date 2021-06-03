#include "common.h"
#include "device.h"
#include "misc.h"

#include "alarm.h"
#include "app.h"
#include "meter.h"
#include "metering.h"
#include "modem.h"
#include "nfc_app.h"
#include "test.h"

#include "battery.h"
#include "flash.h"
#include "lcd.h"
#include "nfc.h"
#include "rtc.h"
#include "timer.h"
#include "uart.h"

typedef struct {
	uint8 nRetry;
	uint8 inProcess;
	uint8 meterType;
	uint8 success;
	MeteringType_t type;
} Metering_t;

/**
 * @brief Meter metering context. (related meter.c & slave_access.c)
 */
static Metering_t Metering;

/**
 * @brief APP event(include ISR event) task ID
 */
static uint8 AppTaskId;

/**
 * @brief Terminal configuration context(Global context)
 */
Config_t conf;

void APP_showConfig(Config_t *p)
{
	char serial[SERIAL_NUM_LEN + 1] = "";
	memcpy(serial, p->serialNum, SERIAL_NUM_LEN);
	serial[SERIAL_NUM_LEN] = '\0';

	printf("===========================================\n");
	printf("         NB-IoT Terminal\n");
	printf("===========================================\n");
	printf(" [ Model      ] %s \n", TERM_MODEL_STRING(p->termModel));
	printf(" [ Build      ] %s %s (%s)\n", __DATE__, __TIME__,
	       APP_getFwVer(p->isUsingPF));
	printf(" [ S/N        ] %s\n", serial);
	printf(" [ IMEI       ] %02x-%02x%02x%02x-%02x%02x%02x-%x\n",
	       p->imei[0], p->imei[1], p->imei[2], p->imei[3], p->imei[4],
	       p->imei[5], p->imei[6], p->imei[7] / 0x10);
	if (p->isUsingPF) {
		printf(" [ Server     ] %s : %d, %s\n", p->serverIp,
		       p->serverPort, p->serviceCode);
	} else {
		printf(" [ Server     ] %s : %d, %s\n", p->serverIp,
		       p->serverPort, p->serviceCode);
		printf(" [ FOTA       ] %s : %d, %dday\n", p->fotaIp,
		       p->fotaPort, p->fotaInterval);
	}
	if (p->isShortInterval) {
		printf(" [ MI & RI    ] %d (minutes)\n", p->reportInterval);
		printf(" [ Base Min   ] %d min\n", p->reportMin);
	} else {
		printf(" [ RI         ] %d (hours)\n", p->reportInterval);
		printf(" [ Report Hour] ");
		if (p->reportInterval == 1) {
			printf("Every Hour\n");
		} else {
			for (int h = 0; h < 24; h++) {
				if (MISC_checkInterval(h, p->reportHour,
						       p->reportInterval)) {
					printf("%d, ", h);
				}
			}
			printf("\n");
		}
		printf(" [ Report Min ] %d (%d sec)\n", p->reportMin,
		       p->reportSec);
	}
	printf(" [ RI Control ] %s \n", (p->rtCtrlMode > 0) ? "On" : "Off");
	printf(" [ Meter Type ] %s\n", METER_getMeterName(p->meterType));
	uint8 batt = BATT_getLastVoltage();
	printf(" [ Battery    ] %d.%dV\n", batt / 10, batt % 10);
	printf(" [ Reset Count] %d times\n", p->resetCount);
	printf(" [ Debug Out  ] %s\n", p->debugPrint ? "On" : "Off");
	printf(" [ RST CAUSE  ] %Xh \n", SYSRSTIV);
	printf("===========================================\n");
	printf("\n");
}

uint8 APP_getTaskId()
{
	return AppTaskId;
}

char *APP_getFwVer(BOOL isUsingPlatform)
{
#if NBIOT_DEVICE
	return FIRMWARE_VER_NBIOT_ONEM2M;
#else
	return NULL;
#endif
}

//===========================================
// INTERRUPT Service Routine
//===========================================

void APP_meterRxIsr()
{
	if (TEST_isTestMode()) {
		OSAL_startEventTimer(TEST_getTaskId(), TEST_EVENT_METER_RX,
				     (uint32)200);
	} else {
		OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_RX,
				     (uint32)200);
	}
}

void APP_modemRxIsr()
{
	if (TEST_isTestMode()) {
		OSAL_startEventTimer(TEST_getTaskId(), TEST_EVENT_MODEM_RX,
				     (uint32)200);
	} else {
		OSAL_startEventTimer(MODEM_getTaskId(), MODEM_EVENT_RECV_ATCMD,
				     (uint32)200);
	}
}

// Port 1 interrupt service routine - NFC
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
	if (P1IFG & BM(NFC_TAG_P1_PIN)) {
		P1IFG &= ~BM(NFC_TAG_P1_PIN);
		NFC_tagDetect();
		OSAL_startEventTimer(AppTaskId, APP_EVENT_NFC_WAIT,
				     (uint32)700);
		WAKEUP_DEVICE();

		NFCAPP_clearAsyncCmd();
	}

	if (P1IFG & BM(NFC_FD_P1_PIN)) {
		P1IFG &= ~BM(NFC_FD_P1_PIN);
		NFC_recvMessage();
		OSAL_startEventTimer(AppTaskId, APP_EVENT_NFC_WAIT,
				     (uint32)700);
	}
}

// Port 2 interrupt service routine - Reed, Tamper, Flood
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
	if (P2IFG & BM(REED_SENSOR_P2_PIN)) {
		P2IFG &= ~BM(REED_SENSOR_P2_PIN); // P1.2 IFG cleared
		if (TEST_isTestMode()) {
			OSAL_startEventTimer(TEST_getTaskId(),
					     TEST_EVENT_SENSOR_REED,
					     (uint32)10);
		} else {
			OSAL_startEventTimer(AppTaskId, APP_EVENT_SENSOR_REED,
					     (uint32)500);
		}
		WAKEUP_DEVICE();
	}
}

#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR(void)
{
	if (SFRIFG1 & OFIFG) { // OSC fault interrupt flag
		SFRIFG1 &= ~OFIFG; // Clear OSC Fault flag
		UCSCTL7 &= ~(XT1LFOFFG + DCOFFG); // Clear XT1 & DCO fault flags
		SYSTEM_RESET();
	}

	if (SFRIFG1 & NMIIFG) { // NMI pin interrupt flag
		SFRIFG1 &= ~NMIIFG;
		SYSTEM_RESET();
	}

	if (SFRIFG1 & VMAIFG) { // Vacant memory access interrupt flag
		SFRIFG1 &= ~VMAIFG;
		SYSTEM_RESET();
	}
}

void APP_ctrlReportTime()
{
#define RT_CTRL_MAX_THRESHOLD 3 // 최종 반영 횟수
#define RT_CTRL_DEC_SIG_QUAL 80 // 보고주기 감소 기준 값
#define RT_CTRL_INC_SIG_QUAL 110 // 보고주기 증가 기준 값

	if (!conf.rtCtrlMode) {
		printf_ts("report time [control:off] \n");
		return;
	}

	BOOL isIncreased =
		(conf.rtCtrlInterval > conf.reportInterval) ? TRUE : FALSE;
	int error = MODEM_getResult();
	switch (error) {
	case ERROR_CODE_NONE: {
		int sigQual = MODEM_getAbsRsrp();
		if (sigQual <= RT_CTRL_DEC_SIG_QUAL) {
			conf.rtCtrlThreshold =
				(isIncreased) ? (conf.rtCtrlThreshold + 1) : 0;
		} else if (sigQual >= RT_CTRL_INC_SIG_QUAL) {
			conf.rtCtrlThreshold =
				(isIncreased) ? 0 : (conf.rtCtrlThreshold + 1);
		} else {
			conf.rtCtrlThreshold = 0;
		}
	} break;
	case ERROR_CODE_COMM: {
		conf.rtCtrlThreshold =
			(isIncreased) ? 0 : (conf.rtCtrlThreshold + 1);
	} break;

	default:
		// conf.rtCtrlThreshold 값 유지.
		break;
	}

	if (conf.rtCtrlThreshold >= RT_CTRL_MAX_THRESHOLD) {
		conf.rtCtrlThreshold = 0;
		if (isIncreased) {
			conf.rtCtrlInterval = conf.reportInterval;
		} else {
			conf.rtCtrlInterval = conf.reportInterval * 2;
			if (conf.rtCtrlInterval > 24)
				conf.rtCtrlInterval = 24;
		}
	}
	printf_ts("report time [control:on interval:%d Threshold:%d/%d] \n",
		  conf.rtCtrlInterval, conf.rtCtrlThreshold,
		  RT_CTRL_MAX_THRESHOLD);
}

/* Comment APP_prepareToSleep */
void APP_prepareToSleep()
{
	/* 	BOOL isLcdOn = FALSE; */
	/* 	if (Metering.type == INITIAL_METERING || */
	/* 	    Metering.type == IMMEDIATE_METERING_FOR_REPORT) { */
	/* 		isLcdOn = TRUE; */
	/* 	} */

	/* 	if (conf.sleepMode) { */
	/* 		MODEM_stop(TRUE, isLcdOn); */
	/* 		LCD_displayString("SLEEP"); */
	/* 	} else { */
	/* 		MODEM_stop(FALSE, isLcdOn); */
	/* 		LCD_redraw(); */
	/* 		if (Metering.success == 0) { */
	/* 			LCD_displayError(LCD_ERROR_METERING); */
	/* 		} */
	/* 		ALARM_schedule(); */
	/* 	} */
}

void APP_init(uint8 taskId)
{
	AppTaskId = taskId;

	Metering.nRetry = 0;
	Metering.inProcess = 0;
	Metering.meterType = 0;
	Metering.success = 0;
	Metering.type = NOT_ACIVE_METERING;

	PRINT_enable();

	FLASH_readConfigInfo(&conf);
	conf.rtCtrlInterval = conf.reportInterval;
	conf.rtCtrlThreshold = 0;
	conf.isUsingPF = TRUE;

	BATT_init();
	NFC_init();
	ALARM_init();

	FLASH_updateResetCount(&conf);
	APP_showConfig(&conf);
	METER_enable(E_METER);

	// TEST
	conf.debugPrint = 1;
	if (conf.debugPrint == 0) {
		printf("\n--console disabled--\n");
		MISC_delayMs(2000);
		PRINT_stop();
		PRINT_disable();
	}

	MISC_delayMs(500);

	METER_clearTempData();

	OSAL_setEvent(AppTaskId, APP_EVENT_INITIAL);
}

static void meteringCompleted()
{
	/* int state = MODEM_getState(); */
	/* if (state != DEVICE_STATUS_IDLE) { */
	/* 	// INITIAL_METERING 때 모뎀이 동작 중인 경우 */
	/* 	// 1. 부팅 후 MODEM_init()을 호출하지 않은 경우 */
	/* 	// 2. Active 상태에서 INITIAL_METERING 를 수행하는 경우. */
	/* 	if (Metering.type == INITIAL_METERING) { */
	/* 		// 위 2가지 경우를 모두 만족하기 위해 close 수행. */
	/* 		MODEM_stop(TRUE, FALSE); */
	/* 	} else { */
	/* 		printf_ts( */
	/* 			"MODEM is running(%d) even afeter metering completed\n", */
	/* 			state); */
	/* 		return; */
	/* 	} */
	/* } */

	/* if (conf.isShortInterval) { */
	/* 	if (Metering.type == PERIODIC_METERING) { */
	/* 		MODEM_run(MODEM_PROC_PERIODIC_REPORT, FALSE); */
	/* 	} else if (Metering.type == INITIAL_METERING) { */
	/* 		MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE); */
	/* 	} */
	/* } else { */
	/* 	if (Metering.type == INITIAL_METERING) { */
	/* 		MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE); */
	/* 	} else if (Metering.type == IMMEDIATE_METERING_FOR_REPORT) { */
	/* 		MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE); */
	/* 	} */
	/* } */
}

BOOL APP_isMetering()
{
	return Metering.inProcess;
}

void APP_bypassMetering(uint8 *body, int bodyLen)
{
	// 미터 조정을 위한 요청이라는 걸 기록하고, 재전송은 없음
	Metering.nRetry = 1;
	Metering.inProcess = 1;
	Metering.meterType = conf.meterType;
	Metering.type = BYPASS_METERING;

	METER_bypassReq(body, bodyLen);

	// nfc로 돌려 줄 회신을 meter로부터 수신할 때까지 다소 시간이 걸림
	// 그 시간 동안 vcc를 계속 유지하기 위해 timeout을 길게 설정
	OSAL_stopEventTimer(AppTaskId, APP_EVENT_NFC_WAIT);
	OSAL_startEventTimer(AppTaskId, APP_EVENT_NFC_WAIT, (uint32)1500);

	OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT, (uint32)1200);
}

void APP_runMetering(MeteringType_t meteringType)
{
	if (Metering.inProcess) // 현재 검침중이면 재 검침하지 않음
	{
		return;
	}

	// INITIAL_METERING 과 PERIODIC_METERING은 무조건 처리하나 즉시 검침은 확인
	// 필요
	if (meteringType == IMMEDIATE_METERING ||
	    meteringType == IMMEDIATE_METERING_FOR_REPORT) {
		if (METER_needNewMetering(10) ==
		    0) // 즉시 검침의 경우에는 10초 이하이면 재 검침하지 않음.
		{
			// 즉시 검침의 경우에는 10초 이하이면 재 검침하지 않음
			Metering.type = meteringType;
			meteringCompleted();
			return;
		}
	}

	memset(&Metering, 0, sizeof(Metering));
	Metering.type = meteringType;
	Metering.inProcess = 1;
	Metering.meterType = conf.meterType;
	Metering.success = 0;

	Metering.nRetry = 3;

	METER_sendRequest(Metering.meterType);
	OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT, (uint32)1500);
}

void APP_runPeriodicReport()
{
	int state = MODEM_getState();
	if (state == DEVICE_STATUS_IDLE) {
		MODEM_run(MODEM_PROC_PERIODIC_REPORT, FALSE);
		printf_ts("modem run\n");
	} else {
		printf_ts("Modem is running(%d) when reporting period\n",
			  state);
	}
}

static void meteringTimeout()
{
	if (Metering.type == BYPASS_METERING) {
		Metering.inProcess = 0;
		Metering.success = 0;
	} else {
		if (--Metering.nRetry > 0) {
			// 약간의 시간 gap을 준 뒤 재검침 시도
			MISC_delayMs(200);
			METER_sendRequest(Metering.meterType);
			OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT,
					     (uint32)1500);
		} else {
			Metering.inProcess = 0;
			Metering.success = 0;

			MeterUnitData_t unit;
			memset(&unit, 0, sizeof(unit));
			/* memset(unit.meterData, 0xff, 4); */
			/* unit.meterStatus = 0xff; */

			Date_t date;
			RTC_read(&date);
			/* switch (Metering.type) { */
			/* case PERIODIC_METERING: */
			/* 	METER_addStoredData(&date, &unit); */
			/* 	METER_displayStoredData(); */
			/* 	break; */
			/* case PERIODIC_METERING_FOR_SAVE: */
			/* 	METER_saveInFlash(&date, &unit); */
			/* 	break; */
			/* default: */
			METER_addTempData(&date, &unit);
			/* METER_displayTempData(); */
			/* break; */
			/* } */

			meteringCompleted();
		}
	}
}

static void meteringRecvResponse()
{
	OSAL_stopEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT);

	if (Metering.type == BYPASS_METERING) {
		Metering.inProcess = 0;
		Metering.success = 1;
		METER_bypassResp();
	} else {
		MeterUnitData_t unit;
		BOOL result = METER_recvResponse(Metering.meterType, &unit);
		if (result == TRUE) {
			Metering.inProcess = 0;
			Metering.success = 1;

			Date_t date;
			RTC_read(&date);
			METER_addTempData(&date, &unit);

			if (Metering.type == IMMEDIATE_METERING ||
			    Metering.type == IMMEDIATE_METERING_FOR_REPORT) {
				if (Metering.meterType != conf.meterType) {
					printf("meter type changed (%02x --> %02X)\n",
					       conf.meterType,
					       Metering.meterType);

					conf.meterType = Metering.meterType;
					FLASH_saveConfigInfo(&conf);
					FLASH_readConfigInfo(&conf);
				}
			}

			meteringCompleted();

		} else {
			OSAL_setEvent(AppTaskId, APP_EVENT_METER_TIMEOUT);
		}
	}
}

event32_t APP_tasks(uint8 taskId, event32_t events)
{
	if (events & APP_EVENT_IMMEDIATE_REPORT) {
		if (MODEM_getState() == DEVICE_STATUS_IDLE) {
			/* APP_runMetering(IMMEDIATE_METERING_FOR_REPORT); */
		}
		return (events ^ APP_EVENT_IMMEDIATE_REPORT);
	}

	if (events & APP_EVENT_RTC_ALARM) {
		ALARM_run();
		return (events ^ APP_EVENT_RTC_ALARM);
	}

	if (events & APP_EVENT_INITIAL) {
		MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE);
		return (events ^ APP_EVENT_INITIAL);
	}

	if (events & APP_EVENT_METER_RX) {
		meteringRecvResponse();
		return (events ^ APP_EVENT_METER_RX);
	}

	if (events & APP_EVENT_METER_TIMEOUT) {
		meteringTimeout();
		return (events ^ APP_EVENT_METER_TIMEOUT);
	}

	if (events & APP_EVENT_METER_BYPASS) {
		NFCAPP_meterBypassReq();
		return (events ^ APP_EVENT_METER_BYPASS);
	}

	if (events & APP_EVENT_IMMEDIATE_METERING) {
		/* APP_runMetering(IMMEDIATE_METERING); */
		return (events ^ APP_EVENT_IMMEDIATE_METERING);
	}

	if (events & APP_EVENT_SENSOR_REED) {
		/* APP_runMetering(IMMEDIATE_METERING); */
		return (events ^ APP_EVENT_SENSOR_REED);
	}

	if (events & APP_EVENT_NFC_WAIT) {
		NFC_init();
		return (events ^ APP_EVENT_NFC_WAIT);
	}

	if (events & APP_EVENT_REBOOT) {
		REBOOT_SYSTEM();
		return (events ^ APP_EVENT_REBOOT);
	}

	if (events & APP_EVENT_CHANGE_CONFIG) {
		FLASH_saveConfigInfo(&conf);
		REBOOT_SYSTEM();
		return (events ^ APP_EVENT_CHANGE_CONFIG);
	}

	return (0);
}
