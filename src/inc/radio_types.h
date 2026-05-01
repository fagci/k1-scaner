#ifndef RADIO_TYPES_H
#define RADIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// ── параметры ──────────────────────────────────────────────────────────
typedef enum {
    PARAM_RADIO_TYPE,     PARAM_FREQUENCY,      PARAM_RX_FREQUENCY,
    PARAM_TX_FREQUENCY,   PARAM_MODULATION,     PARAM_BANDWIDTH,
    PARAM_SQUELCH_LEVEL,  PARAM_SQUELCH_TYPE,   PARAM_GAIN_INDEX,
    PARAM_AGC_ENABLE,     PARAM_AFC_ENABLE,     PARAM_AFC_SPEED,
    PARAM_FILTER,         PARAM_PPM,            PARAM_UPCONVERTER,
    PARAM_STEP,           PARAM_POWER,          PARAM_VOLUME,
    PARAM_DWELL_MS,       PARAM_SCANLIST_MASK,
    PARAM_RSSI,           PARAM_NOISE,          PARAM_GLITCH,
    PARAM_SNR,
    PARAM_COUNT,
} ParamID;

#define FLAG_READONLY 1

typedef struct {
    int32_t value;
    uint8_t flags;
} ParamEntry;

typedef struct {
    uint32_t magic;
    ParamEntry p[PARAM_COUNT];
    uint32_t saved_at;
} ParamSet;

// ── ScanEntry ───────────────────────────────────────────────────────────
#define MAX_CHANNELS 200

typedef struct {
    uint32_t freq;
    uint16_t dwell_ms;
    uint16_t rx_profile;
    uint8_t  squelch;
    uint8_t  flags;
} ScanEntry;

// ── RxProfile ───────────────────────────────────────────────────────────
#define MAX_RX_PROFILES 16

typedef struct {
    uint32_t freq_anchor;
    uint8_t  gain_index;
    uint8_t  squelch_level;
    uint8_t  squelch_type;
    uint8_t  modulation;
    uint8_t  bw;
    uint8_t  agc_enable;
    uint8_t  afc_enable;
    uint8_t  filter;
    int8_t   ppm;
} RxProfile;

// ── Loot ────────────────────────────────────────────────────────────────
#define LOOT_MAX 32

typedef struct {
    uint32_t freq;
    uint16_t rssi_peak;
    uint16_t seen_ago;       // секунд с последнего обнаружения
    uint8_t  glitch_min;
    uint8_t  hit_count;
    uint8_t  rx_profile;
    uint8_t  ctcss;          // 0 = none, 1..50 = CTCSS index, 0xFF = DCS
    uint8_t  flags;
} LootItem;  // 14 байт

#define LOOT_CONFIRMED  1
#define LOOT_SAVED      2
#define LOOT_BLACKLIST  4
#define LOOT_WHITELIST  8
#define LOOT_HAS_CTCSS  16
#define LOOT_HAS_DCS    32

// ── Blacklist entry ─────────────────────────────────────────────────────
#define BL_MAX 64

typedef struct {
    uint32_t lo;        // частота или начало диапазона
    uint32_t hi;        // конец диапазона (0 = одиночная частота)
} BlackItem;  // 8 байт

// ── File formats ────────────────────────────────────────────────────────
#define VFOFILE_MAGIC  0x56460001

// channels.ch: массив ScanEntry, count в первых 2 байтах
typedef struct {
    uint16_t count;
    ScanEntry e[MAX_CHANNELS];
} ChannelsFile;

// scanlist.sl: массив индексов (uint16_t) в channels.ch
typedef struct {
    uint16_t count;
    uint16_t idx[MAX_CHANNELS];
} ScanListIdx;

typedef struct {
    uint32_t magic;
    ParamSet params;
    uint16_t rx_profile_index;
} VFOFile;

// ── API ─────────────────────────────────────────────────────────────────
void Param_Init(ParamSet *ps, uint32_t magic);
int32_t Param_Get(const ParamSet *ps, ParamID id);
void Param_Set(ParamSet *ps, ParamID id, int32_t value);
void Param_Apply(ParamSet *ps, uint8_t radio_type);
void Param_ApplyOne(ParamSet *ps, uint8_t radio_type, ParamID id);

#endif
