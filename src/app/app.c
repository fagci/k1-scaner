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
extern BlackItem gBlack[]; extern uint8_t gBlackCount;
extern void ch_save(void);

static AppState gState;
static uint32_t gScanLo, gScanHi, gScanStep, gScanFreq;
static uint16_t gListenIdx;
static const uint16_t *gScanListPtr;
static uint16_t gScanListCount, gScanListIdx;
static ScanSettings gSettings;
uint32_t gLastFreq;
uint32_t gListenFreq;
bool gListenAudioOn;

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
static void loot_add(uint32_t f, uint16_t rssi, uint8_t noise) {
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
    if (gSettings.scan_action == 1) { // WL
        if (gChannelCount < MAX_CHANNELS) {
            gChannels[gChannelCount].freq = f; gChannels[gChannelCount].freq_end = 0;
            gChannels[gChannelCount].squelch = 3; gChannels[gChannelCount].step_khz = 0;
            gChannels[gChannelCount].flags = 0; gChannels[gChannelCount].name[0] = 0;
            gChannelCount++; ch_save();
        }
    } else if (gSettings.scan_action == 2) { // BL
        if (gBlackCount < BL_MAX) { gBlack[gBlackCount].lo = f; gBlack[gBlackCount].hi = 0; gBlackCount++; }
    } else if (gSettings.scan_action == 3) {
        // listen — делается в APP_Tick
    }
}

void APP_Init(void) {
    gState = APP_IDLE;
    gSettings.noise_threshold = 46;
    gSettings.scan_action = 0;
    gSettings.dwell_ms = 10;
    // загружаем из файла
    Storage_Load("settings.cfg", 0, &gSettings, sizeof(gSettings));
    if (gSettings.noise_threshold == 0 || gSettings.noise_threshold > 200) {
        gSettings.noise_threshold = 46;
        gSettings.scan_action = 0;
        gSettings.dwell_ms = 10;
    }
}
AppState APP_GetState(void) { return gState; }
void APP_Stop(void) { gState = APP_IDLE; }
void APP_SetSettings(const ScanSettings *s) {
    gSettings = *s;
    Storage_Save("settings.cfg", 0, &gSettings, sizeof(gSettings));
}
ScanSettings *APP_GetSettings(void) { return &gSettings; }

void APP_StartScan(uint32_t lo, uint32_t hi, uint32_t step) {
    gScanListPtr = 0;
    gScanLo = lo; gScanHi = hi; gScanStep = step;
    gScanFreq = lo;
    gState = APP_SCAN;
}
void APP_StartScanList(const uint16_t *list, uint16_t count) {
    gScanListPtr = list; gScanListCount = count; gScanListIdx = 0;
    gState = APP_SCAN;
}
void APP_Listen(uint16_t ch_index) {
    gListenIdx = ch_index;
    gListenAudioOn = 0;
    gState = APP_LISTEN;
}

void APP_Tick(void) {
    if (gState == APP_IDLE) return;
    USB_CDC_Poll();
    switch (gState) {
    case APP_SCAN: {
        uint32_t f;
        if (gScanListPtr) {
            if (gScanListIdx >= gScanListCount) gScanListIdx = 0;
            uint16_t ci = gScanListPtr[gScanListIdx];
            if (ci >= gChannelCount) { gScanListIdx++; break; }
            f = gChannels[ci].freq; gScanListIdx++;
        } else {
            f = gScanFreq;
            if (f > gScanHi) f = gScanLo;
        }
        gLastFreq = f;
        if (!black_has(f)) {
            BK4819_TuneTo(f, 0);
            BK4819_Squelch(3, f, 1, 1);
            uint32_t t0 = Now();
            while (Now() - t0 < gSettings.dwell_ms) { if (gState != APP_SCAN) return; }
            uint16_t rssi = BK4819_GetRSSI();
            uint8_t noise = BK4819_GetNoise();
            if (noise <= gSettings.noise_threshold) {
                loot_add(f, rssi, noise);
                if (gSettings.scan_action == 3) {
                    // слушаем 3 секунды
                    BK4819_ToggleAFDAC(1); BK4819_ToggleAFBit(1);
                    GPIO_EnableAudioPath(); AUDIO_ToggleSpeaker(1);
                    uint32_t tend = Now() + 3000;
                    while (Now() < tend && gState == APP_SCAN) { USB_CDC_Poll(); SYSTICK_DelayMs(10); }
                    AUDIO_ToggleSpeaker(0); GPIO_DisableAudioPath();
                    BK4819_ToggleAFBit(0); BK4819_ToggleAFDAC(0);
                }
            }
        }
        if (!gScanListPtr) { gScanFreq = f + gScanStep; if (gScanFreq > gScanHi) gScanFreq = gScanLo; }
        break;
    }
    case APP_LISTEN:
        if (gListenIdx >= gChannelCount) { gState = APP_IDLE; break; }
        gListenFreq = gChannels[gListenIdx].freq;
        BK4819_TuneTo(gListenFreq, 0);
        BK4819_Squelch(gChannels[gListenIdx].squelch, gListenFreq, 1, 1);
        if (!gListenAudioOn) {
            BK4819_ToggleAFDAC(1); BK4819_ToggleAFBit(1);
            GPIO_EnableAudioPath(); AUDIO_ToggleSpeaker(1);
            gListenAudioOn = 1;
        }
        // слушаем, пока сигнал есть (squelch open) или 5 секунд после пропадания
        {
            static uint32_t tSilent;
            if (BK4819_IsSquelchOpen()) {
                tSilent = Now();
            }
            if (Now() - tSilent > 5000) {
                AUDIO_ToggleSpeaker(0); GPIO_DisableAudioPath();
                BK4819_ToggleAFBit(0); BK4819_ToggleAFDAC(0);
                gState = APP_IDLE;
            }
        }
        break;
    default: break;
    }
}
