#include "settings.h"
#include "driver/backlight.h"
#include "driver/st7565.h"

Settings gSettings = {
    .batteryCalibration = 2184,
    .batteryType = BAT_1600,
    .batteryStyle = BAT_PERCENT,
    .contrast = 8,
    .brightness = 8,
};

const uint32_t EEPROM_SIZES[6] = {0};

void SETTINGS_Save(void) {}
void SETTINGS_Load(void) {}
