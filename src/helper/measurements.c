#include "measurements.h"

#define SQ_HYSTERESIS 4

const uint16_t RSSI_MIN = 28;
const uint16_t RSSI_MAX = 226;

long long Clamp(long long v, long long min, long long max) {
    return v <= min ? min : (v >= max ? max : v);
}

int ConvertDomain(int aValue, int aMin, int aMax, int bMin, int bMax) {
    const int aRange = aMax - aMin;
    const int bRange = bMax - bMin;
    aValue = Clamp(aValue, aMin, aMax);
    return ((aValue - aMin) * bRange + aRange / 2) / aRange + bMin;
}

int16_t Rssi2DBm(uint16_t rssi) { return (rssi >> 1) - 160; }
uint16_t DBm2Rssi(int16_t dbm) { return (dbm + 160) << 1; }

uint32_t AdjustU(uint32_t val, uint32_t min, uint32_t max, int32_t inc) {
    if (inc > 0) return val == max - inc ? min : val + inc;
    else return val > min ? val + inc : max + inc;
}

uint32_t IncDecU(uint32_t val, uint32_t min, uint32_t max, bool inc) {
    return AdjustU(val, min, max, inc ? 1 : -1);
}

SQL GetSql(uint8_t level) {
    SQL sq = {0, 0, 255, 255, 255, 255};
    if (level == 0) return sq;
    sq.ro = ConvertDomain(level, 0, 10, 60, 180);
    sq.no = ConvertDomain(level, 0, 10, 64, 12);
    sq.go = ConvertDomain(level, 0, 10, 100, 0);
    sq.rc = sq.ro - SQ_HYSTERESIS;
    sq.nc = sq.no + SQ_HYSTERESIS;
    sq.gc = sq.go + SQ_HYSTERESIS;
    return sq;
}

uint32_t DeltaF(uint32_t f1, uint32_t f2) {
    return f1 > f2 ? f1 - f2 : f2 - f1;
}

uint32_t RoundToStep(uint32_t f, uint32_t step) {
    uint32_t sd = f % step;
    f += (sd > step / 2) ? (step - sd) : -sd;
    return f;
}
