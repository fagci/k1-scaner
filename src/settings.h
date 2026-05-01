#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define SCANLIST_ALL 0

typedef enum {
    SETTING_COUNT,
} Setting;

typedef enum {
    BL_SQL_OFF, BL_SQL_ON, BL_SQL_OPEN,
} BacklightOnSquelchMode;

typedef enum {
    BAT_1400, BAT_1600, BAT_2200, BAT_3500,
} BatteryType;

typedef enum {
    BAT_CLEAN, BAT_PERCENT, BAT_VOLTAGE,
} BatteryStyle;

extern const char *BATTERY_TYPE_NAMES[4];
extern const char *BATTERY_STYLE_NAMES[3];
extern const uint32_t EEPROM_SIZES[6];

typedef struct {
    uint32_t upconverter : 27;
    uint8_t checkbyte : 5;
    uint16_t batteryCalibration : 12;
    BatteryType batteryType : 2;
    BatteryStyle batteryStyle : 2;
    uint8_t contrast : 4;
    uint8_t brightness : 4;
} __attribute__((packed)) Settings;

extern Settings gSettings;

void SETTINGS_Save(void);
void SETTINGS_Load(void);

#endif
