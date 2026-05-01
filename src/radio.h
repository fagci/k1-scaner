#ifndef RADIO_H
#define RADIO_H

#include "driver/bk4829.h"
#include "inc/vfo.h"
#include <stdbool.h>
#include <stdint.h>

void RADIO_Init(void);
void RADIO_SetModulation(ModulationType mod);
void RADIO_SetFilterBandwidth(BK4819_FilterBandwidth_t bw);
void RADIO_SetAGC(bool enable, uint8_t gainIdx);
void RADIO_SetSquelch(uint8_t level);
void RADIO_TuneTo(uint32_t freq);

uint16_t RADIO_GetRSSI(void);
uint8_t RADIO_GetNoise(void);
uint8_t RADIO_GetGlitch(void);
uint8_t RADIO_GetSNR(void);

void RADIO_Measure(Measurement *m);
bool RADIO_IsSquelchOpen(void);

void RADIO_EnableAudio(bool on);

#endif
