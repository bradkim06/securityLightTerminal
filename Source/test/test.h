#ifndef __TEST_H__
#define __TEST_H__

#include "var_type.h"

#define TEST_EVENT_SENSOR_REED 0x00000001
#define TEST_EVENT_MODEM_RX 0x00000002
#define TEST_EVENT_METER_RX 0x00000004

BOOL TEST_stop();
BOOL TEST_checkTestMode();
BOOL TEST_isTestMode();
uint8 TEST_getTaskId();
void TEST_readRTC(int fromJig);
void TEST_setRTC(int fromJig, int year, int mon, int day, int hour, int min,
		 int sec);
void TEST_readFlash(int sector);
void TEST_eraseFlash(int sector);
void TEST_readSerialNum();
void TEST_writeSerialNum(char *serialNum);
void TEST_config();
void TEST_meteringParamer(int ri);
void TEST_riCtrlMode(int flag);
void TEST_setMeterType(int type);
void TEST_shortInterval(int flag);
void TEST_debugPrint(int flag);
void TEST_readMeter(int type);

void TEST_pwsn(char *sn, int ri);

void TEST_prsn();
void TEST_readBattery();
void TEST_checkNFC();
void TEST_resetNFC();

void TEST_run();
void TEST_runTasks();
void TEST_readFwVersion();
#endif
