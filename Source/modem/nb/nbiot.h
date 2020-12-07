#ifndef __NBIOT_H__
#define __NBIOT_H__

#include "common.h"
#include "modem.h"
#include "queue.h"

// MCC in IMSI
#define IMSI_MCC_KOREA "450"

// MNC in IMSO
#define IMSI_MCC_MNC_SKT   IMSI_MCC_KOREA "05"
#define IMSI_MCC_MNC_KT_1  IMSI_MCC_KOREA "04"
#define IMSI_MCC_MNC_KT_2  IMSI_MCC_KOREA "08"
#define IMSI_MCC_MNC_UPLUS IMSI_MCC_KOREA "06"

// NB-IoT each error log legnth
#define LEN_NBIOT_ERROR_LOG 7

// NB-IoT number of error log
#define NUM_OF_NBIOT_ERROR_LOG 4

#define NBIOT_QA_REPORT_INTERVAL  24  // unit is hour
#define NBIOT_ONEM2M_REG_INTERVAL 18  // unit is hour
#define NBIOT_PUASED_DELAY        48  // unit is hour

#define TIMEOUT_NBIOT_DEFAULT 3000
#define TIMEOUT_NRB           15000
#define TIMEOUT_ATTACH        5000
#define TIMEOUT_BIP           10000
#define TIMEOUT_UDP_SEND      5000
#define TIMEOUT_UDP_RECV      3000
#define TIMEOUT_LWM2M_RECV    40000
#define TIMEOUT_ENTER_PSM     30000
#define TIMEOUT_FOTA_DOWN_REQ 30000
#define TIMEOUT_FOTA_DOWNLOAD 120000  // 120 sec
#define TIMEOUT_FOTA_UPDATE   60000   // 60 sec
#define TIMEOUT_FOTA_COMPLETE 60000

#define RETRY_NBIOT_DEFAULT 3
#define RETRY_NRB           0
#define RETRY_ATTACH        7
#define RETRY_BIP           11
#define RETRY_UDP_SEND      2
#define RETRY_UDP_RECV      1
#define RETRY_LWM2M_SEND    1
#define RETRY_LWM2M_RECV    0
#define RETRY_LWM2M_FOTA    0

#define DELAY_NBIOT_BOOTUP 7000
#define DELAY_NBIOT_UPLINK 5000

// Modem cell registration status(= attach status)
typedef enum {
  NBIOT_CELLREG_TRYING     = 0,
  NBIOT_CELLREG_ATTACHED   = 1,
  NBIOT_CELLREG_PAUSE      = 2,
  NBIOT_CELLREG_REJECTED   = 3,
  NBIOT_CELLREG_NO_SERVICE = 4,
} NbIotCellreg_t;

// Modem access step
typedef enum {
  NBIOT_STEP_FINISH,
  NBIOT_STEP_IDLE,
  NBIOT_STEP_BIP,
  NBIOT_STEP_INIT,       // Initialize step
  NBIOT_STEP_ATTACH_NW,  // Connecting N/W
  NBIOT_STEP_TRANSFER,   // Send UDP uplink
  NBIOT_STEP_FOTA,
  NBIOT_STEP_DETACH_NW,
  NBIOT_STEP_UNKNOWN = 0xFF,
} NbIotStep_t;

// Modem access step
typedef enum {
  NBIOT_ERROR_NONE           = 0,
  NBIOT_ERROR_USIM_INVALID   = 0x11,  // AT+CIMI 무응답 OR ERROR
  NBIOT_ERROR_PF_EPNS_CHANGE = 0x12,  // EPNS 변경 발생.
  NBIOT_ERROR_PF_CERITY_FAIL = 0x13,  // Bootstrap or Register 실패
  NBIOT_ERROR_PF_UL_FAIL     = 0x14,  // Platform 연동 시 UL 실패
  NBIOT_ERROR_PF_DL_FAIL     = 0x15,  // Platform 연동 시 DL 실패
  NBIOT_ERROR_PF_FOTA_FAIL   = 0x16,  // Platform module FOTA 실패
  NBIOT_ERROR_ATTACH_FAIL    = 0x21,  // 망접속 실패
  NBIOT_ERROR_UDP_UL_FAIL    = 0x31,  // UDP UL 실패
  NBIOT_ERROR_UDP_DL_FAIL    = 0x32,  // UDP DL 실패
  NBIOT_ERROR_AT_CMD_NO_RESP = 0x35,  // AT command no response
  NBIOT_ERROR_AT_CMD_FAIL    = 0x36,  // AT command error
} NbIotError_t;

typedef union {
  uint8 value;  // Flag for reset
  struct {
    uint8 error : 1,  // AT command Error 여부를 표시. (모뎀 시작/종료/AT
                      // command 전송 시 초기화)
        setCfg : 1,  // 모뎀 설정 변경 여부를 표시. (최초 부팅 혹은 USIM 및 서버
                     // 주소 변경 여부 표시)
        psmOn : 1,   // PSM 동작 여부를 표시. (기능 여부 및 동작에 따라 설정 및
                     // 초기화)
        waitDL : 1,  // Uplink 성공 후 Downlink 대기 여부를 표시. (기능 여부 및
                     // 동작에 따라 설정 및 초기화)
        reserved : 5;
  };
} NbIotStatus_t;

typedef union {
  uint8 value;  // Flag for reset
  struct {
    uint8 updateReg : 1,  // Flag to set whether Register Update is necessary in
                          // CERTIFY step
        updateQa : 1,     // Flag to set whether Update QA is necessary after
                          // attached
        runFota : 1,      // Flag to set whether to check FOTA request.
        reserved : 5;
  };
} NbiotProc_t;

typedef union {
  uint16 value;  // Flag for reset
  struct {
    uint16 bsFinish : 1,    // Finish Bootstrap
        regFinish : 1,      // Finish Register
        regUpdate : 1,      // Update Register
        regDelete : 1,      // Delete Register
        obsObj10250 : 1,    // Observe object 10250
        obsObj503 : 1,      // Observe object 503
        obsObj16241 : 1,    // Observe object 16241
        cxlObj10250 : 1,    // Cancel object 10250
        fotaDownReq : 1,    // FOTA download requested
        fotaUpdateReq : 1,  // FOTA update requested
        fotaFinish : 1,     // Finish FOTA
        reserved : 5;
  };
} Lwm2m_t;

typedef struct {
  BOOL   isOperating;
  uint16 newPort;
  char   newIp[SERVER_IP_STR_LEN];
} MsgCmd_t;

typedef struct {
  NbIotError_t   error;
  NbIotStep_t    currentStep;
  NbIotStep_t    nextStep;
  NbIotCellreg_t cellreg;
  NbIotStatus_t  status;
  NbiotProc_t    proc;
  Lwm2m_t        lwm2m;
  MsgCmd_t       msgCmd;
  int            socket;
} NbIotContext_t;

typedef struct {
  int    rssi;
  int    cid;  // Physical Cell ID
  int    rsrp;
  int    rsrq;
  int    snr;
  int    txPower;
  uint32 cgi;  // Serving Cell ID
} NbIotSigQual_t;

typedef struct {
  NbIotSigQual_t sigQuality;
  Queue_t*       errorLog;
  int            udpUlCnt;
  int            udpDlCnt;
  int            pfUlCnt;
  int            pfDlCnt;
  char           imeiStr[LEN_MODEM_IMEI_IMSI + 1];
  char           imsiStr[LEN_MODEM_IMEI_IMSI + 1];
  char           ctnStr[LEN_MODEM_CTN + 1];
  char           iccidStr[LEN_MODEM_ICCID + 1];
  char           epName[LEN_MODEM_EP_NAME + 1];
  char           bsParam[LEN_MODEM_BS_PARAM + 1];
  Date_t         lastQaReportTime;
  Date_t         lastCertifyTime;
  Date_t         lastAccessTime;
  Date_t         lastAttachTime;
} NbIotInfo_t;

#define NBIOT_STEP_STR(x)                                               \
  (((x) == NBIOT_STEP_IDLE)                                             \
       ? "IDLE"                                                         \
       : ((x) == NBIOT_STEP_BIP)                                        \
             ? "BIP"                                                    \
             : ((x) == NBIOT_STEP_INIT)                                 \
                   ? "INIT"                                             \
                   : ((x) == NBIOT_STEP_ATTACH_NW)                      \
                         ? "ATTACH"                                     \
                         : ((x) == NBIOT_STEP_DETACH_NW)                \
                               ? "DETACH"                               \
                               : ((x) == NBIOT_STEP_FOTA)               \
                                     ? "FOTA"                           \
                                     : ((x) == NBIOT_STEP_TRANSFER)     \
                                           ? "TRANSFER"                 \
                                           : ((x) == NBIOT_STEP_FINISH) \
                                                 ? "FINISH"             \
                                                 : "UNKNOWN")

extern NbIotContext_t nbCtx;
extern NbIotInfo_t    nbInfo;

void NBIOT_retryProcess(BOOL isNeedAttach, BOOL isHwReset);
BOOL NBIOT_sendDataProcess();
void NBIOT_finishProcess();
void NBIOT_registerData(char* data, int len);
void NBIOT_init();
void NBIOT_open();
void NBIOT_close();
void NBIOT_release();

#endif
