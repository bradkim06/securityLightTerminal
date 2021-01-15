
#ifndef __BATTERY_H__
#define __BATTERY_H__

#include "var_type.h"

#define BATT_ADC_IN_P6_PIN 0 // P6.0

void BATT_init();
int BATT_getLevel();
uint8 BATT_getVoltage(void);
uint8 BATT_getLastVoltage(void);
void BATT_loadLastVoltage(void);

#endif // __BATTERY_H__
