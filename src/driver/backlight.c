#include "backlight.h"
#include "../board.h"
#include "../settings.h"
#include "gpio.h"
#include "systick.h"
#include "st7565.h"
#include <string.h>

static uint8_t gBacklightCountdown_500ms = 0;
static uint8_t gBrightness = 8;

void BACKLIGHT_InitHardware(void) {
    GPIO_TurnOnBacklight();
}

void BACKLIGHT_TurnOn(void) {
    GPIO_TurnOnBacklight();
    gBacklightCountdown_500ms = 120;
}

void BACKLIGHT_TurnOff(void) {
    GPIO_TurnOffBacklight();
}

void BACKLIGHT_SetBrightness(uint8_t value) {
    gBrightness = value;
}

uint8_t BACKLIGHT_GetBrightness(void) {
    return gBrightness;
}

void BACKLIGHT_UpdateTimer(void) {
    if (gBacklightCountdown_500ms > 0) {
        gBacklightCountdown_500ms--;
        if (gBacklightCountdown_500ms == 0) {
            BACKLIGHT_TurnOff();
        }
    }
}
