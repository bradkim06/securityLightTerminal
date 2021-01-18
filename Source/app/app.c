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

#if REPEATER_DEVICE
#include "cc1200.h"
#include "slave_access.h"
#endif

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
#if LORA_DEVICE
	printf("         LoRa Terminal(Solu-M)\n");
#else // NBIOT_DEVICE
	printf("         NB-IoT Terminal\n");
#endif
	printf("===========================================\n");
	printf(" [ Model      ] %s \n", TERM_MODEL_STRING(p->termModel));
	printf(" [ Build      ] %s %s (%s)\n", __DATE__, __TIME__,
	       APP_getFwVer(p->isUsingPF));
	printf(" [ S/N        ] %s\n", serial);
#if LORA_DEVICE
	printf(" [ Device EUI ] %02x%02x%02x-%02x%02x-%02x%02x%02x\n",
	       p->devEui[0], p->devEui[1], p->devEui[2], p->devEui[3],
	       p->devEui[4], p->devEui[5], p->devEui[6], p->devEui[7]);
#else // NBIOT_DEVICE
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
#endif
	if (p->isShortInterval) {
		printf(" [ MI & RI    ] %d (minutes)\n", p->reportInterval);
		printf(" [ Base Min   ] %d min\n", p->reportMin);
	} else {
		printf(" [ MI/RI/SI   ] %d/%d/%d (hours)\n", p->meterInterval,
		       p->reportInterval, METER_DATA_SAVE_INTERVAL);
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
	/* Comment sleepMode APP_showConfig */
	/* printf(" [ Operating  ] %s\n", p->sleepMode ? "Sleep" : "Active"); */
	uint8 batt = BATT_getLastVoltage();
	printf(" [ Battery    ] %d.%dV\n", batt / 10, batt % 10);
	printf(" [ Reset Count] %d times\n", p->resetCount);
	printf(" [ Debug Out  ] %s\n", p->debugPrint ? "On" : "Off");
#if REPEATER_DEVICE
	printf(" [ PAN ID     ] %04X\n", p->pan_id);

	uint8 *master = (uint8 *)&p->nwk_addr;
	uint8 *slave = (uint8 *)&p->slaveNwk;
	printf(" [ NWK        ] %d.%d.%d.%d\n", *(master + 3), *(master + 2),
	       *(master + 1), *(master + 0));
	printf(" [ Slave NWK  ] %d.%d.%d.%d\n", *(slave + 3), *(slave + 2),
	       *(slave + 1), *(slave + 0));
#endif
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
	if (isUsingPlatform) {
		return FIRMWARE_VER_NBIOT_ONEM2M;
	} else {
		return FIRMWARE_VER_NBIOT_UDP;
	}
#elif LORA_DEVICE
	if (isUsingPlatform) {
		return FIRMWARE_VER_LORA_THINGPLUG;
	} else {
		return FIRMWARE_VER_LORA_PRIVATE;
	}
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

#if REPEATER_DEVICE
	// CC1200 tx/rx interrupt
	if (P2IFG & BM(CC1200_INT_P2_PIN)) {
		P2IFG &= ~BM(CC1200_INT_P2_PIN);

		if (SLAVE_getAccessState() == SLAVE_ACCESS_IDLE &&
		    CC1200_getRfMode() == RF_MODE_RX) {
			// Rx 동작은 Slave operating이 아닌 경우 쓰레기 값으로 간주하고 그 외에는
			// 모두 처리. (Slave에 대한 ACK 전송 처리가 Interrupt에 의한 비동기로
			// 처리되므로
			//  Slave operating이 아니더라도 ISR에 의한 처리가 필요.)
			if (TEST_isTestMode() == FALSE) {
				// Test mode인 경우 모두 처리.
				return;
			}
		}

		if (CC1200_getRfMode() == RF_MODE_TX) {
			// P2IES &= ~BM(CC1200_INT_P2_PIN);   // P2.7 Low/Hi edge
			CC1200_isr();
			WAKEUP_DEVICE();
		} else { // RF_MODE_RX
			if (P2IES & BM(CC1200_INT_P2_PIN)) {
				P2IES &= ~BM(
					CC1200_INT_P2_PIN); // P2.7 Low/Hi edge
				CC1200_isr();
				WAKEUP_DEVICE();
				CC1200_worDuty(WOR_SHORT_DUTY);
				CC1200_wor();
			} else { // Change Len
				P2IES |= BM(
					CC1200_INT_P2_PIN); // P2.7 Hi/Low edge
				CC1200_read_rxByte();
				WAKEUP_DEVICE();
			}
		}
	}
#endif
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
#if LORA_DEVICE
#define RT_CTRL_DEC_SIG_QUAL 90 // 보고주기 감소 기준 값
#define RT_CTRL_INC_SIG_QUAL 120 // 보고주기 증가 기준 값
#else
#define RT_CTRL_DEC_SIG_QUAL 80 // 보고주기 감소 기준 값
#define RT_CTRL_INC_SIG_QUAL 110 // 보고주기 증가 기준 값
#endif

	if (!conf.rtCtrlMode) {
		printf_ts("report time [control:off] \n");
		return;
	}

	BOOL isIncreased =
		(conf.rtCtrlInterval > conf.reportInterval) ? TRUE : FALSE;
	int error = MODEM_getResult();
	switch (error) {
#if LORA_DEVICE
	// LoRa의 경우 GET TIME 실패도 데이터 전송은 성공한 것이므로 통신성공으로
	// 간주한다.
	case ERROR_CODE_GET_TIME:
#endif
	case ERROR_CODE_NONE: {
#if LORA_DEVICE
		int sigQual = MODEM_getAbsRssi();
#else
		int sigQual = MODEM_getAbsRsrp();
#endif
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
#if LORA_DEVICE
	// LoRa의 경우 Join 실패도 통신실패로 간주한다.
	case ERROR_CODE_NOT_JOINED:
#endif
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
/* void APP_prepareToSleep() */
/* { */
/* #if REPEATER_DEVICE */
/* 	SLAVE_close(); */
/* #endif */

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
/* } */

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

	BATT_init();
	LCD_init();
	NFC_init();
	ALARM_init();

	FLASH_updateResetCount(&conf);
	APP_showConfig(&conf);

	if (conf.debugPrint == 0) {
		/* printf("\n--console disabled--\n"); */
		/* MISC_delayMs(2000); */
		/* PRINT_stop(); */
		/* PRINT_disable(); */
	}

	// Flash memory에 저장된 데이터 점검 및 프린트
	FLASH_initData();
	FLASH_printData();

	// 초기화 직후 LCD에 펌웨어 버전을 표시
	LCD_clear();
	LCD_displayFirmwareVersion();
	MISC_delayMs(500);

	/* Comment sleepMode APP_init */
	/* if (conf.sleepMode) { */
	/* 	// NFC통신 후 Sleep여부에 따라 H마크를 표시히자만, */
	/* 	// H/W Reset이 발생할 경우 H마크가 초기화되기 때문에 */
	/* 	// Sleep 모드인 경우에만 H마크 On동작을 수행한다. */
	/* 	if (SMART_WATER_METER(conf.termModel)) { */
	/* 		MISC_delayMs( */
	/* 			1500); // 검침기 MCU 부팅 및 통신 준비시간 대기 */
	/* 		METER_std_lcdMarkReq(conf.sleepMode); */
	/* 		MISC_delayMs(100); */
	/* 	} */
	/* 	APP_prepareToSleep(); */
	/* 	return; */
	/* } */

	METER_clearTempData();
	METER_clearStoredData(TRUE);
	LCD_redraw();

	OSAL_setEvent(AppTaskId, APP_EVENT_INITIAL);
}

static void meteringCompleted()
{
	int state = MODEM_getState();
	if (state != DEVICE_STATUS_IDLE) {
		// INITIAL_METERING 때 모뎀이 동작 중인 경우
		// 1. 부팅 후 MODEM_init()을 호출하지 않은 경우
		// 2. Active 상태에서 INITIAL_METERING 를 수행하는 경우.
		if (Metering.type == INITIAL_METERING) {
			// 위 2가지 경우를 모두 만족하기 위해 close 수행.
			MODEM_stop(TRUE, FALSE);
		} else {
			printf_ts(
				"MODEM is running(%d) even afeter metering completed\n",
				state);
			return;
		}
	}

	if (conf.isShortInterval) {
		if (Metering.type == PERIODIC_METERING) {
			MODEM_run(MODEM_PROC_PERIODIC_REPORT, FALSE);
		} else if (Metering.type == INITIAL_METERING) {
			MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE);
		}
		/* Comment APP_prepareToSleep */
		/* } else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	} else {
		if (Metering.type == INITIAL_METERING) {
			MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE);
		} else if (Metering.type == IMMEDIATE_METERING_FOR_REPORT) {
			MODEM_run(MODEM_PROC_EVENT_REPORT, TRUE);
#if REPEATER_DEVICE
		} else if (Metering.type == PERIODIC_METERING) {
			// 보조중계기로 쓰일때에는 slave 주기 검침 직후 report.
			MODEM_run(MODEM_PROC_PERIODIC_REPORT, FALSE);
#endif
		}
		/* Comment APP_prepareToSleep */
		/* } else { */
		/* 	APP_prepareToSleep(); */
		/* } */
	}
}

#if REPEATER_DEVICE

void APP_slaveAccessCompleted()
{
	printf_ts("Slave access complete\n");
	Metering.inProcess = 0;
	Metering.success = 1;
	APP_prepareToSleep();
}

void APP_slaveAccessFailed()
{
	printf_ts("Slave access fail\n");
	Metering.inProcess = 0;
	Metering.success = 0;
	APP_prepareToSleep();
}

void APP_slaveMeteringCompleted()
{
	printf_ts("Slave metering complete\n");
	Metering.inProcess = 0;
	Metering.success = 1;

	// Slave 검침 후 LCD에 검침데이터를 표시할 때 LCD Icon 정보가 없으므로
	// 기본 Icon 정보를 설정한 후 LCD에 표시함.
	if (Metering.type == PERIODIC_METERING) {
		// 최초 주기동작에서 Slave가 검침데이터를 안줄 수 있음.
		if (METER_getNumberOfStoredData() > 0) {
			MeterUnitData_t *unit = METER_getStoredData(0);
			unit->icon.fArrow = 1;
			unit->icon.m3 = 1;
			unit->icon.notUsed = 0;
			LCD_updateMeterValue(
				(uint8 *)unit); // Display meter data
		}
	} else {
		MeterUnitData_t *unit = METER_getTempData();
		unit->icon.fArrow = 1;
		unit->icon.m3 = 1;
		unit->icon.notUsed = 0;
		LCD_updateMeterValue((uint8 *)unit); // Display meter data
	}

	meteringCompleted();
}

void APP_slaveMeteringFailed()
{
	printf_ts("Slave metering fail\n");
	Metering.inProcess = 0;
	Metering.success = 0;

	if (Metering.type == INITIAL_METERING ||
	    Metering.type == IMMEDIATE_METERING ||
	    Metering.type == IMMEDIATE_METERING_FOR_REPORT) {
		MeterUnitData_t unit;
		memset(&unit, 0, sizeof(unit));
		memset(unit.meterData, 0xff, 4);
		unit.meterStatus = 0xff;

		Date_t date;
		RTC_read(&date);
		METER_addTempData(&date, &unit);
		METER_displayTempData();
	}

	meteringCompleted();

	/*
   * 주기검침 MT DOWN 데이터를 저장 및 전송시 2가지 고려 사항이 필요.
   * 1. 단말기에서 다음 주기에 정상적인 데이터를 받을 경우 기존(MT_DOWN) 데이터
   * 삭제.
   * 2. 서버에서 다음 주기에 정상적인 데이터를 받을 경우 기존(MT_DOWN) 데이터
   * 삭제.
   *
   * 다만, 위 사항을 추가하여 얻을 수 있는 이점이 미미하므로 주기 검침 시 MT
   * DOWN을 저장하지 않고 기존 데이터만 (데이터 개수가 0 포함) 전송하도록 적용.
   */
}

#endif

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
#if REPEATER_DEVICE
	if (SLAVE_getAccessState() !=
	    SLAVE_ACCESS_IDLE) // 현재 Slave 동작 중일 경우 검침하지 않음.
#else
	if (Metering.inProcess) // 현재 검침중이면 재 검침하지 않음
#endif
	{
		return;
	}

	// INITIAL_METERING 과 PERIODIC_METERING은 무조건 처리하나 즉시 검침은 확인
	// 필요
	if (meteringType == IMMEDIATE_METERING ||
	    meteringType == IMMEDIATE_METERING_FOR_REPORT) {
#if REPEATER_DEVICE
		if (SLAVE_needToUpdateMeterInfo() ==
		    FALSE) // Meter Info update에 대한 Interval 확인.
#else
		if (METER_needNewMetering(10) ==
		    0) // 즉시 검침의 경우에는 10초 이하이면 재 검침하지 않음.
#endif
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
	if (meteringType == IMMEDIATE_METERING ||
	    meteringType == IMMEDIATE_METERING_FOR_REPORT) {
		// 자석을 댄 경우 마지막에 미터 타입을 바꾸어 한 번 더 시도해 보기 위해
		// 4회로 늘림.
		Metering.nRetry = 4;
	}

#if REPEATER_DEVICE
	SLAVE_metering(Metering.type);
#else
	METER_sendRequest(Metering.meterType);
	OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT, (uint32)1500);
#endif
}

void APP_runPeriodicReport()
{
	int state = MODEM_getState();
	if (state == DEVICE_STATUS_IDLE) {
		MODEM_run(MODEM_PROC_PERIODIC_REPORT, FALSE);
	} else {
		printf_ts("Modem is running(%d) when reporting period\n",
			  state);
	}
}

static void meteringTimeout()
{
	LCD_redraw(); // LCD의 프로펠러 회전을 중지하기 위함.

	if (Metering.type == BYPASS_METERING) {
		Metering.inProcess = 0;
		Metering.success = 0;
	} else {
		if (--Metering.nRetry > 0) {
			MISC_delayMs(
				100); // 약간의 시간 gap을 준 뒤 재검침 시도

			if (Metering.nRetry == 1 &&
			    (Metering.type == IMMEDIATE_METERING ||
			     Metering.type == IMMEDIATE_METERING_FOR_REPORT)) {
				// 마지막 검침에서 미터 타입을 바꾸어 한 번 더 시도함.
				if (Metering.meterType == W_STANDARD_D) {
					Metering.meterType = W_SHINHAN_D;
				} else if (Metering.meterType == W_SHINHAN_D ||
					   Metering.meterType ==
						   W_SHINHAN_D_BIG) {
					Metering.meterType = W_STANDARD_D;
				}
			}
			METER_sendRequest(Metering.meterType);
			OSAL_startEventTimer(AppTaskId, APP_EVENT_METER_TIMEOUT,
					     (uint32)1500);
		} else {
			Metering.inProcess = 0;
			Metering.success = 0;

			MeterUnitData_t unit;
			memset(&unit, 0, sizeof(unit));
			memset(unit.meterData, 0xff, 4);
			unit.meterStatus = 0xff;

			Date_t date;
			RTC_read(&date);
			switch (Metering.type) {
			case PERIODIC_METERING:
				METER_addStoredData(&date, &unit);
				METER_displayStoredData();
				break;
			case PERIODIC_METERING_FOR_SAVE:
				METER_saveInFlash(&date, &unit);
				break;
			default:
				METER_addTempData(&date, &unit);
				METER_displayTempData();
				break;
			}

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
			switch (Metering.type) {
			case PERIODIC_METERING:
				unit.sec =
					0; // 주기검침의 경우 초단위는 0으로 설정한다.
				METER_addStoredData(&date, &unit);
				METER_displayStoredData();
				break;
			case PERIODIC_METERING_FOR_SAVE:
				METER_saveInFlash(&date, &unit);
				break;
			default:
				METER_addTempData(&date, &unit);
				METER_displayTempData();
				break;
			}

			LCD_updateMeterValue((uint8 *)&unit);

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
		/* Comment sleepMode */
		/* if (!conf.sleepMode) { */
		if (MODEM_getState() == DEVICE_STATUS_IDLE) {
			APP_runMetering(IMMEDIATE_METERING_FOR_REPORT);
		}
		/* } */
		return (events ^ APP_EVENT_IMMEDIATE_REPORT);
	}

	if (events & APP_EVENT_RTC_ALARM) {
		ALARM_run();
		return (events ^ APP_EVENT_RTC_ALARM);
	}

	if (events & APP_EVENT_INITIAL) {
		APP_runMetering(INITIAL_METERING);
		return (events ^ APP_EVENT_INITIAL);
	}

	if (events & APP_EVENT_METER_RX) {
		METER_disable();
		meteringRecvResponse();
		return (events ^ APP_EVENT_METER_RX);
	}

	if (events & APP_EVENT_METER_TIMEOUT) {
		METER_disable();
		meteringTimeout();
		return (events ^ APP_EVENT_METER_TIMEOUT);
	}

	if (events & APP_EVENT_METER_BYPASS) {
		NFCAPP_meterBypassReq();
		return (events ^ APP_EVENT_METER_BYPASS);
	}

	if (events & APP_EVENT_IMMEDIATE_METERING) {
		APP_runMetering(IMMEDIATE_METERING);
		return (events ^ APP_EVENT_IMMEDIATE_METERING);
	}

	if (events & APP_EVENT_SENSOR_REED) {
		APP_runMetering(IMMEDIATE_METERING);
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
		/* Comment sleepMode */
		/* if (SMART_WATER_METER(conf.termModel)) { */
		/* 	// SWM인 경우 sleep 모드에 따라 H마크 on/off */
		/* 	METER_std_lcdMarkReq(conf.sleepMode); */
		/* 	MISC_delayMs(100); // UART 전송완료 대기. */
		/* } */
		FLASH_saveConfigInfo(&conf);
		REBOOT_SYSTEM();
		return (events ^ APP_EVENT_CHANGE_CONFIG);
	}

	return (0);
}
