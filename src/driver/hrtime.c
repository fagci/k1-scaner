#include "hrtime.h"
#include "py32f071_ll_bus.h"
#include "py32f071_ll_tim.h"

/**
 * TIM2 is configured as a free-running up-counter at 48 MHz.
 * - Prescaler: 0 (count every APB clock = 48 MHz cycle)
 * - Auto-reload: 0xFFFFFFFF (32-bit, wraps every ~89 seconds)
 * - No output compare, no interrupts — just a running counter.
 */

void HRTIME_Init(void) {
  /* Enable TIM2 clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

  /* Reset TIM2 to defaults */
  LL_TIM_DeInit(TIM2);

  /* Configure as free-running counter:
   * - Counter direction: up
   * - Prescaler: 0 (count at full 48 MHz)
   * - Auto-reload: 0xFFFFFFFF (max 32-bit value, never triggers update)
   * - Clock division: none
   * - Repetition counter: 0
   */
  LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERDIRECTION_UP);
  LL_TIM_SetPrescaler(TIM2, 0);
  LL_TIM_SetAutoReload(TIM2, 0xFFFFFFFFu);
  LL_TIM_DisableARRPreload(TIM2);
  LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_GenerateEvent_UPDATE(TIM2); /* Load prescaler & ARR immediately */

  /* Start the counter */
  LL_TIM_EnableCounter(TIM2);
}

uint32_t HRTIME_Now(void) { return LL_TIM_GetCounter(TIM2); }

void HRTIME_DelayUs(uint32_t us) {
  if (us == 0)
    return;
  uint32_t start = LL_TIM_GetCounter(TIM2);
  uint32_t target = us * 48u; /* 48 ticks per microsecond */
  while ((LL_TIM_GetCounter(TIM2) - start) < target) {
    /* spin — precise, no interrupt latency */
  }
}
