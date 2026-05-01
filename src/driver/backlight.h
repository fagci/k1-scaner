#ifndef DRIVER_BACKLIGHT_H
#define DRIVER_BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

void BACKLIGHT_InitHardware();
void BACKLIGHT_TurnOn();
void BACKLIGHT_TurnOff();
bool BACKLIGHT_IsOn();
void BACKLIGHT_SetBrightness(uint8_t brightness);
uint8_t BACKLIGHT_GetBrightness(void);
void BACKLIGHT_UpdateTimer(void);

#endif
