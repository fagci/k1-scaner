#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include "../misc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SQ_PRESETS_COUNT 11

typedef struct {
    uint16_t ro; uint8_t no; uint8_t go;
    uint8_t rc; uint8_t nc; uint8_t gc;
} SQL;

extern const uint16_t RSSI_MIN;
extern const uint16_t RSSI_MAX;

long long Clamp(long long v, long long min, long long max);
int ConvertDomain(int aValue, int aMin, int aMax, int bMin, int bMax);
int16_t Rssi2DBm(uint16_t rssi);
uint16_t DBm2Rssi(int16_t dbm);
uint32_t AdjustU(uint32_t val, uint32_t min, uint32_t max, int32_t inc);
uint32_t IncDecU(uint32_t val, uint32_t min, uint32_t max, bool inc);
SQL GetSql(uint8_t level);
uint32_t DeltaF(uint32_t f1, uint32_t f2);
uint32_t RoundToStep(uint32_t f, uint32_t step);

#endif
