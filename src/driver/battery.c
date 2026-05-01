#include "battery.h"
#include "../board.h"
#include "../misc.h"
#include "../settings.h"

uint16_t gBatteryVoltage = 0;
uint16_t gBatteryCurrent = 0;
uint8_t gBatteryPercent = 0;
bool gChargingWithTypeC = true;

static uint16_t batAdcV = 0;
static uint16_t batAvgV = 0;

const char *BATTERY_TYPE_NAMES[4] = {"1400mAh", "1600mAh", "2200mAh", "3500mAh"};
const char *BATTERY_STYLE_NAMES[3] = {"Icon", "%", "V"};

const uint16_t Voltage2PercentageTable[][11][2] = {
    [BAT_1400] = {
        {828, 100}, {813, 97}, {794, 80}, {782, 65}, {770, 45},
        {758, 25}, {750, 18}, {742, 12}, {734, 9}, {726, 6}, {700, 0},
    },
    [BAT_1600] = {
        {828, 100}, {813, 97}, {794, 80}, {782, 65}, {770, 45},
        {758, 25}, {750, 18}, {742, 12}, {734, 9}, {726, 6}, {700, 0},
    },
    [BAT_2200] = {
        {828, 100}, {820, 97}, {810, 80}, {798, 65}, {786, 45},
        {774, 25}, {766, 18}, {758, 12}, {750, 9}, {742, 6}, {700, 0},
    },
    [BAT_3500] = {
        {840, 100}, {828, 97}, {816, 80}, {804, 65}, {792, 45},
        {780, 25}, {772, 18}, {764, 12}, {756, 9}, {748, 6}, {700, 0},
    },
};

static const uint16_t BAT_CAL_MIN = 1900;

static uint16_t applyCalibration(uint16_t raw) {
    if (gSettings.batteryCalibration < BAT_CAL_MIN) return raw;
    uint32_t cal = gSettings.batteryCalibration;
    return (uint32_t)raw * cal / 2184;
}

void BATTERY_UpdateBatteryInfo(void) {
    BOARD_ADC_GetBatteryInfo(&batAdcV, &gBatteryCurrent);
    uint16_t adcCal = applyCalibration(batAdcV);
    if (batAvgV == 0) batAvgV = adcCal;
    batAvgV = (batAvgV * 3 + adcCal) / 4;
    gBatteryVoltage = batAvgV;
    gBatteryPercent = 100;
    for (uint8_t i = 0; i < 10; i++) {
        if (batAvgV >= Voltage2PercentageTable[BAT_1600][i][0]) {
            gBatteryPercent = Voltage2PercentageTable[BAT_1600][i][1];
            break;
        }
    }
}

uint8_t BATTERY_GetBatteryPercent(void) { return gBatteryPercent; }
uint16_t BATTERY_GetBatteryVoltage(void) { return gBatteryVoltage; }
uint32_t BATTERY_GetPreciseVoltage(uint16_t calibration) {
    (void)calibration;
    return gBatteryVoltage;
}
uint16_t BATTERY_GetCal(uint32_t v) {
    (void)v;
    return 0;
}
uint16_t BATTERY_GetCurrent(void) { return gBatteryCurrent; }
bool BATTERY_IsCharging(void) { return gChargingWithTypeC; }
