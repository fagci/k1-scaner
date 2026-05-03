#include "app.h"
#include "../inc/radio_types.h"
#include "../driver/bk4829.h"
#include "../driver/audio.h"
#include "../driver/gpio.h"
#include "../driver/systick.h"
#include "../driver/usb_cdc.h"
#include "../helper/storage.h"
#include <string.h>

extern LootItem gLoot[]; extern uint8_t gLootCount; extern uint32_t gLootTotal;
extern ScanEntry gChannels[]; extern uint16_t gChannelCount;
extern uint16_t gScanList[]; extern uint16_t gScanCount;
extern BlackItem gBlack[]; extern uint8_t gBlackCount;
extern void ch_save(void);

static AppState gState;
static ScanSettings gSettings;
static uint32_t gScanStep;           // текущий шаг для диапазона
static uint16_t gListenIdx;

// Flat scan list: все частоты для сканирования одним проходом
#define SCAN_FLAT_MAX 512
static uint32_t gFlatFreq[SCAN_FLAT_MAX];
static uint16_t gFlatCount;
static uint16_t gFlatPos;

uint32_t gScanCurrentFreq;
uint32_t gScanProgress;

// ── helpers ────────────────────────────────────────────────────────────
static bool black_has(uint32_t f) {
    for (uint8_t i = 0; i < gBlackCount; i++) {
        if (gBlack[i].hi == 0) { if (gBlack[i].lo == f) return 1; }
        else { if (f >= gBlack[i].lo && f <= gBlack[i].hi) return 1; }
    }
    return 0;
}

static bool loot_has_freq(uint32_t f) {
    for (uint8_t i = 0; i < gLootCount; i++)
        if (gLoot[i].freq == f) return 1;
    return 0;
}

static void loot_save(uint32_t f, uint16_t rssi, uint8_t noise) {
    if (loot_has_freq(f)) return;
    LootItem l;
    l.freq = f; l.rssi_peak = rssi; l.glitch_min = noise;
    l.seen_ago = 0; l.hit_count = 1; l.rx_profile = 0; l.ctcss = 0;
    l.flags = LOOT_CONFIRMED;
    if (BK4819_IsSquelchOpen()) {
        uint16_t cf; uint32_t dc;
        BK4819_CssScanResult_t css = BK4819_GetCxCSSScanResult(&dc, &cf);
        if (css == BK4819_CSS_RESULT_CTCSS) { l.ctcss = cf / 10; l.flags |= LOOT_HAS_CTCSS; }
        else if (css == BK4819_CSS_RESULT_CDCSS) { l.ctcss = 0xFF; l.flags |= LOOT_HAS_DCS; }
    }
    gLootTotal++;
    if (gLootCount < LOOT_MAX) { gLoot[gLootCount++] = l; }
    else { memmove(gLoot, gLoot + 1, (LOOT_MAX - 1) * sizeof(LootItem)); gLoot[LOOT_MAX - 1] = l; }

    // авто-действие
    if (gSettings.scan_action == 1 && gChannelCount < MAX_CHANNELS) {
        gChannels[gChannelCount].freq = f; gChannels[gChannelCount].freq_end = 0;
        gChannels[gChannelCount].squelch = 3; gChannels[gChannelCount].step_khz = 0;
        gChannels[gChannelCount].flags = 0; gChannels[gChannelCount].name[0] = 0;
        gChannelCount++; ch_save();
    } else if (gSettings.scan_action == 2 && gBlackCount < BL_MAX) {
        gBlack[gBlackCount].lo = f; gBlack[gBlackCount].hi = 0; gBlackCount++;
    }
}

// ── формирование плоского списка ─────────────────────────────────────
static void flat_add(uint32_t f) {
    if (gFlatCount >= SCAN_FLAT_MAX) return;
    // проверка на дубликат
    for (uint16_t i = 0; i < gFlatCount; i++)
        if (gFlatFreq[i] == f) return;
    gFlatFreq[gFlatCount++] = f;
}

static void build_flat(void) {
    gFlatCount = 0;
    // 1. все каналы (частоты + диапазоны)
    for (uint16_t i = 0; i < gChannelCount; i++) {
        ScanEntry *e = &gChannels[i];
        if (e->freq_end == 0) {
            if (!black_has(e->freq)) flat_add(e->freq);
        } else {
            uint32_t step = e->step_khz ? e->step_khz * 100 : 2500;
            for (uint32_t f = e->freq; f <= e->freq_end; f += step)
                if (!black_has(f)) flat_add(f);
        }
    }
    // 2. каналы из активного сканлиста (если не дублируются)
    for (uint16_t si = 0; si < gScanCount; si++) {
        uint16_t ci = gScanList[si];
        if (ci < gChannelCount) {
            uint32_t f = gChannels[ci].freq;
            if (!black_has(f)) flat_add(f);
        }
    }
    gFlatPos = 0;
}

void APP_Init(void) {
    gState = APP_IDLE;
    gSettings.noise_threshold = 46;
    gSettings.scan_action = 0;
    gSettings.dwell_ms = 10;
    Storage_Load("settings.cfg", 0, &gSettings, sizeof(gSettings));
    if (gSettings.noise_threshold == 0 || gSettings.noise_threshold > 200)
        gSettings.noise_threshold = 46;
}

AppState APP_GetState(void) { return gState; }
void APP_Stop(void) { gState = APP_IDLE; }
void APP_SetSettings(const ScanSettings *s) { gSettings = *s; Storage_Save("settings.cfg", 0, &gSettings, sizeof(gSettings)); }
ScanSettings *APP_GetSettings(void) { return &gSettings; }

void APP_StartFullScan(void) {
    build_flat();
    gFlatPos = 0;
    gState = APP_SCAN;
}

void APP_Listen(uint16_t ch_index) {
    gListenIdx = ch_index;
    gState = APP_LISTEN;
}

void APP_Tick(void) {
    if (gState == APP_IDLE) return;
    USB_CDC_Poll();

    switch (gState) {
    case APP_SCAN: {
        if (gFlatCount == 0) { build_flat(); }
        if (gFlatCount == 0) { gState = APP_IDLE; return; }
        if (gFlatPos >= gFlatCount) gFlatPos = 0;

        uint32_t f = gFlatFreq[gFlatPos];
        gScanCurrentFreq = f;
        gScanProgress = (uint32_t)gFlatPos * 1000 / gFlatCount;

        BK4819_TuneTo(f, 0);
        BK4819_Squelch(3, f, 1, 1);
        uint32_t t0 = Now();
        while (Now() - t0 < gSettings.dwell_ms) {
            if (gState != APP_SCAN) return;
        }
        uint16_t rssi = BK4819_GetRSSI();
        uint8_t noise = BK4819_GetNoise();
        if (noise <= gSettings.noise_threshold)
            loot_save(f, rssi, noise);

        gFlatPos++;
        break;
    }

    case APP_LISTEN:
        if (gListenIdx >= gChannelCount) { gState = APP_IDLE; return; }
        {
            uint32_t f = gChannels[gListenIdx].freq;
            BK4819_TuneTo(f, 0);
            BK4819_Squelch(gChannels[gListenIdx].squelch, f, 1, 1);
            // слушаем пока squelch open + 5s
            BK4819_ToggleAFDAC(1); BK4819_ToggleAFBit(1);
            GPIO_EnableAudioPath(); AUDIO_ToggleSpeaker(1);
            uint32_t tSilent = Now();
            while (gState == APP_LISTEN) {
                if (BK4819_IsSquelchOpen()) tSilent = Now();
                if (Now() - tSilent > 5000) break;
                USB_CDC_Poll();
            }
            AUDIO_ToggleSpeaker(0); GPIO_DisableAudioPath();
            BK4819_ToggleAFBit(0); BK4819_ToggleAFDAC(0);
        }
        gState = APP_IDLE;
        break;

    default: break;
    }
}
