#ifndef DRIVER_TIMER_H
#define DRIVER_TIMER_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*timer_cb_t)(void);

// TIM1 — high-res PWM/capture (16-bit, up to 48MHz)
void TIM_PWM_Init(uint32_t freq_hz, uint16_t duty_cycle);
void TIM_PWM_SetDuty(uint16_t duty);

// TIM3 — general purpose periodic callback
void TIM_Periodic_Init(uint32_t period_us, timer_cb_t callback);

// TIM6/LPTIM1 — low-power wake timer
void TIM_LP_Init(uint32_t period_ms, timer_cb_t callback);

#endif
