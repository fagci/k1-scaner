#include "radio.h"
#include "driver/audio.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4829.h"
#include "driver/gpio.h"
#include "driver/systick.h"
#include "misc.h"

void RADIO_Init(void) {
    BK4819_Init();
    BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
    BK4819_RX_TurnOn();
    BK4819_SetAFC(0);
    RADIO_SetAGC(true, 1);
    RADIO_SetFilterBandwidth(BK4819_FILTER_BW_12k);
    RADIO_SetModulation(MOD_FM);
    RADIO_SetSquelch(0);
    BK4819_SelectFilterEx(FILTER_UHF);
}

void RADIO_SetModulation(ModulationType mod) {
    BK4819_SetModulation(mod);
}

void RADIO_SetFilterBandwidth(BK4819_FilterBandwidth_t bw) {
    BK4819_SetFilterBandwidth(bw);
}

void RADIO_SetAGC(bool enable, uint8_t gainIdx) {
    BK4819_SetAGC(enable, gainIdx);
}

void RADIO_SetSquelch(uint8_t level) {
    SQL sq = GetSql(level);
    BK4819_SetupSquelch(sq, 1, 1);
}

void RADIO_TuneTo(uint32_t freq) {
    BK4819_TuneTo(freq, false);
}

uint16_t RADIO_GetRSSI(void) {
    return BK4819_GetRSSI();
}

uint8_t RADIO_GetNoise(void) {
    return BK4819_GetNoise();
}

uint8_t RADIO_GetGlitch(void) {
    return BK4819_GetGlitch();
}

uint8_t RADIO_GetSNR(void) {
    return BK4819_GetSNR();
}

void RADIO_Measure(Measurement *m) {
    m->rssi = BK4819_GetRSSI();
    m->noise = BK4819_GetNoise();
    m->glitch = BK4819_GetGlitch();
    m->snr = BK4819_GetSNR();
}

bool RADIO_IsSquelchOpen(void) {
    return BK4819_IsSquelchOpen();
}

void RADIO_EnableAudio(bool on) {
    BK4819_ToggleAFDAC(on);
    BK4819_ToggleAFBit(on);
    if (on) {
        GPIO_EnableAudioPath();
        AUDIO_ToggleSpeaker(true);
    } else {
        AUDIO_ToggleSpeaker(false);
        GPIO_DisableAudioPath();
    }
}
