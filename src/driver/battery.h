#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

#define BAT_WARN_PERCENT 15

extern uint16_t gBatteryVoltage;
extern uint16_t gBatteryCurrent;
extern uint8_t gBatteryPercent;
extern bool gChargingWithTypeC;

extern const char *BATTERY_TYPE_NAMES[4];
extern const char *BATTERY_STYLE_NAMES[3];

void BATTERY_UpdateBatteryInfo();
uint32_t BATTERY_GetPreciseVoltage(uint16_t cal);
uint16_t BATTERY_GetCal(uint32_t v);

#endif
