#include "../inc/radio_types.h"
#include "../driver/bk4829.h"
#include "../helper/measurements.h"

// ── apply-функции ──────────────────────────────────────────────────────
typedef void (*apply_fn)(int32_t);

static void ap_freq(int32_t v)  { BK4819_TuneTo((uint32_t)v, false); }
static void ap_mod(int32_t v)   { BK4819_SetModulation((ModulationType)v); }
static void ap_bw(int32_t v)    { BK4819_SetFilterBandwidth((BK4819_FilterBandwidth_t)v); }
static void ap_sql(int32_t v)   { BK4819_Squelch((uint8_t)v, BK4819_GetFrequency(), 1, 1); }
static void ap_gain(int32_t v)  { BK4819_SetAGC(v == 0, (uint8_t)v); }
static void ap_afc(int32_t v)   { BK4819_SetAFC((uint8_t)v); }
static void ap_flt(int32_t v)   { BK4819_SelectFilterEx((Filter)v); }

static const apply_fn bk_tbl[PARAM_COUNT] = {
    [PARAM_FREQUENCY]      = ap_freq,
    [PARAM_MODULATION]     = ap_mod,
    [PARAM_BANDWIDTH]      = ap_bw,
    [PARAM_SQUELCH_LEVEL]  = ap_sql,
    [PARAM_GAIN_INDEX]     = ap_gain,
    [PARAM_AFC_ENABLE]     = ap_afc,
    [PARAM_FILTER]         = ap_flt,
};

static const apply_fn *chip_tbl[] = { bk_tbl, NULL, NULL };

static const ParamEntry defs[PARAM_COUNT] = {
    [PARAM_RADIO_TYPE]    = { .value = 0 },
    [PARAM_FREQUENCY]     = { .value = 433000000 },
    [PARAM_MODULATION]    = { .value = 0 },
    [PARAM_BANDWIDTH]     = { .value = 4 },
    [PARAM_SQUELCH_LEVEL] = { .value = 3 },
    [PARAM_SQUELCH_TYPE]  = { .value = 0 },
    [PARAM_GAIN_INDEX]    = { .value = 0 },
    [PARAM_AGC_ENABLE]    = { .value = 1 },
    [PARAM_AFC_ENABLE]    = { .value = 0 },
    [PARAM_FILTER]        = { .value = 2 },
    [PARAM_STEP]          = { .value = 2500 },
    [PARAM_POWER]         = { .value = 0 },
    [PARAM_VOLUME]        = { .value = 4 },
    [PARAM_DWELL_MS]      = { .value = 500 },
    [PARAM_RSSI]          = { .flags = FLAG_READONLY },
    [PARAM_NOISE]         = { .flags = FLAG_READONLY },
    [PARAM_GLITCH]        = { .flags = FLAG_READONLY },
    [PARAM_SNR]           = { .flags = FLAG_READONLY },
};

void Param_Init(ParamSet *ps, uint32_t magic) {
    ps->magic = magic;
    ps->saved_at = 0;
    for (int i = 0; i < PARAM_COUNT; i++) ps->p[i] = defs[i];
}

int32_t Param_Get(const ParamSet *ps, ParamID id) { return ps->p[id].value; }

void Param_Set(ParamSet *ps, ParamID id, int32_t v) {
    if (ps->p[id].flags & FLAG_READONLY) return;
    ps->p[id].value = v;
}

void Param_Apply(ParamSet *ps, uint8_t radio_type) {
    const apply_fn *tbl = chip_tbl[radio_type];
    if (!tbl) return;
    for (int i = 0; i < PARAM_COUNT; i++)
        if (tbl[i]) tbl[i](ps->p[i].value);
}

void Param_ApplyOne(ParamSet *ps, uint8_t radio_type, ParamID id) {
    const apply_fn *tbl = chip_tbl[radio_type];
    if (!tbl || !tbl[id]) return;
    tbl[id](ps->p[id].value);
}
