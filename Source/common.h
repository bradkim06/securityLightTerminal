#ifndef _COMMON_HEADER_
#define _COMMON_HEADER_

#include <ctype.h>
#include <stdarg.h> // va_list, va_start, va_end
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rtc.h"
#include "var_type.h"

// Error Code
#define ERROR_CODE_NONE 0 // 정상
#define ERROR_CODE_MODEM 1 // 모뎀 응답 없음
#if NBIOT_DEVICE
#define ERROR_CODE_SIM 2 // SIM 오류
#else // LORA_DEVICE
#define ERROR_CODE_NOT_JOINED 3 // Join 실패
#endif
#define ERROR_CODE_COMM 4 // 통신 실패
#define ERROR_CODE_GET_TIME 5 // 시간 읽기 오류
#define ERROR_CODE_UNKNOWN 99 // 알 수 없는 오류

// 단말기 타입
#define TERM_MODEL_UNKNOWN 0
#define TERM_MODEL_HAT_114W 1 // 표시형 단말기
#define TERM_MODEL_HAT_124W 2 // 비표시형 단말기
#define TERM_MODEL_HTM_115W 3 // 스마트 워터미터
#define TERM_MODEL_HAT_314W 4 // 외부표시기
#define TERM_MODEL_HAT_435W 5 // 보조중계기

#define TERM_MODEL_STRING_LEN 8
#define TERM_MODEL_STRING(x)                                                                       \
	((x) == TERM_MODEL_HAT_114W ?                                                              \
		 "HAT-114W" :                                                                      \
		 (x) == TERM_MODEL_HAT_124W ?                                                      \
		 "HAT-124W" :                                                                      \
		 (x) == TERM_MODEL_HTM_115W ?                                                      \
		 "HTM-115W" :                                                                      \
		 (x) == TERM_MODEL_HAT_314W ? "HAT-314W" :                                         \
					      (x) == TERM_MODEL_HAT_435W ? "HAT-435W" : "UNKNOWN")

#define TERM_HAS_LCD(x) ((x) == TERM_MODEL_HAT_114W)
#define SMART_WATER_METER(x) ((x) == TERM_MODEL_HTM_115W)

// Terminal Status
#define DEVICE_STATUS_IDLE 0
#define DEVICE_STATUS_CHECK_MODEM 1
#if NBIOT_DEVICE
#define DEVICE_STATUS_CHECK_SIM 2
#else // LORA_DEVICE
#define DEVICE_STATUS_WAIT_JOIN 3
#endif
#define DEVICE_STATUS_COMM 4
#define DEVICE_STATUS_GET_TIME 5
#define DEVICE_STATUS_UNKNOWN 99

// Network access state
#define ACCESS_STATE_IDLE 0 // 접속 절차를 진행하고 있지 않음 - 새 절차 개시 가능
#define ACCESS_STATE_IN_PROGRESS 1 // 접속 절차 진행 중
#define ACCESS_STATE_WAIT_FEW_SEC 2 // 통신과 통신간의 일정 시간 대기하는 상태.
#define ACCESS_STATE_IN_SETTING 3 // 모뎀 재설정 진행 중

// Max num of stored metering data
#if NBIOT_DEVICE
#define MAX_NUM_STORED_DATA 24
#else
#define MAX_NUM_STORED_DATA 12
#endif

#define SERIAL_NUM_LEN 12
#define SERVER_IP_STR_LEN 16 // string 길이이므로 \0 처리 길이까지 포함 시킴.

// Light Termianl Mode
#define INTERNAL 1
#define SETTING 2

// Light Time Base
#define SUNRISE 0
#define CIVIL 1

// PSM Mode
#define PSM_NOT_USING 0
#define PSM_RELIGHT 1
#define PSM_NOT_RELIGHT 2

// Time
#define HOUR 0
#define MINUTE 1

#define MONTH 0
#define DAY 1

typedef struct {
	uint8 mode;
	uint8 timeBase;
	uint8 region;
	uint8 onTime[2];
	uint8 offTime[2];
	uint8 onOffset;
	uint8 offOffset;
	uint8 psm;
	uint8 psmOnTime[2]; // Hour, Min
	uint8 psmOffTime[2]; // Hour, Min
	uint8 psmOnDay[2]; // Month, Day
	uint8 psmOffDay[2]; // Month, Day
	uint8 dimmingMode;
	uint8 dimmingLv;
} light_t;

struct Config {
	uint8 serialNum[SERIAL_NUM_LEN + 1];
	uint8 meterType;
	char serviceCode[5]; // using only LG platfom. others all '0'
	char serverIp[SERVER_IP_STR_LEN];
	uint16 serverPort;
	char fotaIp[SERVER_IP_STR_LEN]; // using for FOTA. It used when not use LG
		// platform.
	uint16 fotaPort; // using for FOTA. It used when not use LG platform.
	uint8 fotaInterval; // using for FOTA. It used when not use LG platform.
	uint8 imei[8];
	uint8 imsi[8];
	BOOL isModemInit;
	uint8 isShortInterval;
	uint8 reportInterval;
	uint8 reportHour;
	uint8 reportMin;
	uint8 reportSec;
	uint16 resetCount;
	uint8 debugPrint;
	uint8 termModel;
	light_t light;
};

extern struct Config conf;

//==============================================================================
// F/W version configuration
//==============================================================================

/**
 * @brief Define firmware main version. It depends on the product family.
 */
#if NBIOT_DEVICE
#if defined(__MODEM_BC95G__)
#define FW_VER_MAJOR "0"
#endif

#define FW_VER_MINOR "01"

#define FIRMWARE_VER_NBIOT_ONEM2M ("L" FW_VER_MAJOR FW_VER_MINOR)

#else
#error "Have to define NBIOT_DEVICE"
#endif

#define FIRMWARE_VER_LEN 4

#endif
