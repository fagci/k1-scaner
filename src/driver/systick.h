#ifndef DRIVER_SYSTICK_H
#define DRIVER_SYSTICK_H

#include <stdbool.h>
#include <stdint.h>

void SYSTICK_Init(void);
void SYSTICK_DelayTicks(const uint32_t ticks);
void SYSTICK_DelayUs(uint32_t Delay);
void SYSTICK_DelayMs(uint32_t Delay);
uint32_t Now();

void SetTimeout(uint32_t *v, uint32_t t);
bool CheckTimeout(uint32_t *v);

#endif
