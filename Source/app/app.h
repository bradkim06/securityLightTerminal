#ifndef _AMIAPP_HEADER_
#define _AMIAPP_HEADER_

//#include "var_type.h"
#include "common.h"
#include "osal.h"

#define CONFIRM_NUMBER 0x13C5

/// APP Event
#define APP_EVENT_INITIAL 0x00000001
#define APP_EVENT_RTC_ALARM 0x00000002
#define APP_EVENT_SENSOR_REED 0x00000004
#define APP_EVENT_METER_RX 0x00000008
#define APP_EVENT_METER_TIMEOUT 0x00000010
#define APP_EVENT_METER_BYPASS 0x00000020
#define APP_EVENT_NFC_WAIT 0x00000040
#define APP_EVENT_IMMEDIATE_METERING 0x00000080
#define APP_EVENT_IMMEDIATE_REPORT 0x00000100
#define APP_EVENT_CHANGE_CONFIG 0x00000200
#define APP_EVENT_REBOOT 0x80000000

typedef enum {
	NOT_ACIVE_METERING = 0,
	INITIAL_METERING,
	PERIODIC_METERING,
	LIGHT_ON,
	LIGHT_OFF,
	STABILIZE_ON,
	STABILIZE_OFF,
	IMMEDIATE_METERING,
} MeteringType_t;

typedef enum {
	AUTO = 1,
	WORKER,
	SERVER,
	FORCE,
} Cause_t;

// Function proto-type
char *APP_getFwVer();
uint8 APP_getTaskId();
void APP_init(uint8 taskId);
/* Comment APP_prepareToSleep */
/* void APP_prepareToSleep(); */

void APP_runPeriodicReport();
BOOL APP_isMetering();
void APP_bypassMetering(uint8 *body, int bodyLen);
void APP_runMetering(MeteringType_t meteringType, Cause_t cause);
void APP_showConfig(Config_t *p);

event32_t APP_tasks(uint8 taskId, event32_t events);

void APP_meterRxIsr();
void APP_modemRxIsr();

#endif
