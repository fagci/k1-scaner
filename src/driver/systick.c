#include "systick.h"
#include "py32f071_ll_utils.h"
#include "py32f0xx.h"
#include "core_cm0plus.h"

static volatile uint32_t elapsedMilliseconds;
static const uint32_t TICK_MULTIPLIER = 48;

void SYSTICK_Init(void) {
  LL_SetSystemCoreClock(48000000);
  SystemCoreClockUpdate();
  SysTick_Config(48000);
  NVIC_SetPriority(SysTick_IRQn, 0);
  __enable_irq();
}

void SysTick_Handler(void) {
  elapsedMilliseconds++;
}

uint32_t Now() { return elapsedMilliseconds; }

void SYSTICK_DelayTicks(uint32_t ticks) {
  const uint32_t load = SysTick->LOAD + 1U;
  uint32_t prev = SysTick->VAL;

  while (ticks > 0) {
    const uint32_t cur = SysTick->VAL;
    uint32_t delta;

    if (cur <= prev)
      delta = prev - cur;
    else
      delta = prev + (load - cur);

    if (delta != 0) {
      if (delta >= ticks)
        break;
      ticks -= delta;
      prev = cur;
    }
  }
}

void SYSTICK_DelayUs(const uint32_t Delay) {
  SYSTICK_DelayTicks(Delay * TICK_MULTIPLIER);
}

void SYSTICK_DelayMs(uint32_t ms) {
  const uint32_t start = Now();
  while ((uint32_t)(Now() - start) < ms) {
  }
}
// void SYSTICK_DelayMs(uint32_t ms) { SYSTICK_DelayUs(ms * 1000); }

void SetTimeout(uint32_t *v, uint32_t t) {
  *v = t == UINT32_MAX ? UINT32_MAX : Now() + t;
}

bool CheckTimeout(uint32_t *v) {
  if (*v == UINT32_MAX) {
    return false;
  }
  return (int32_t)(Now() - *v) >= 0;
}
