#include "timer.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_tim.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_lptim.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_bus.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_rcc.h"

void TIM_PWM_Init(uint32_t freq_hz, uint16_t duty_cycle) {
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

    uint32_t psc = 48e6 / freq_hz / 65536 + 1;
    uint32_t arr = 48e6 / (psc * freq_hz);

    LL_TIM_SetPrescaler(TIM1, psc - 1);
    LL_TIM_SetAutoReload(TIM1, arr - 1);
    LL_TIM_OC_SetCompareCH1(TIM1, duty_cycle);
    LL_TIM_OC_SetMode(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);
    LL_TIM_EnableCounter(TIM1);
    LL_TIM_EnableAllOutputs(TIM1);
}

void TIM_PWM_SetDuty(uint16_t duty) {
    LL_TIM_OC_SetCompareCH1(TIM1, duty);
}

static timer_cb_t g_periodic_cb;

void TIM3_IRQHandler(void) {
    if (LL_TIM_IsActiveFlag_UPDATE(TIM3)) {
        LL_TIM_ClearFlag_UPDATE(TIM3);
        if (g_periodic_cb) g_periodic_cb();
    }
}

void TIM_Periodic_Init(uint32_t period_us, timer_cb_t callback) {
    g_periodic_cb = callback;
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    uint32_t psc = 48;
    uint32_t arr = period_us;

    LL_TIM_SetPrescaler(TIM3, psc - 1);
    LL_TIM_SetAutoReload(TIM3, arr - 1);
    LL_TIM_EnableIT_UPDATE(TIM3);
    LL_TIM_EnableCounter(TIM3);

    NVIC_SetPriority(TIM3_IRQn, 1);
    NVIC_EnableIRQ(TIM3_IRQn);
}
