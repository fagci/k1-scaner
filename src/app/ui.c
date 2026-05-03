#include "ui.h"
#include "app.h"
#include "../driver/keyboard.h"
#include "../driver/st7565.h"
#include "../driver/usb_cdc.h"
#include "../driver/systick.h"
#include "../ui/graphics.h"
#include "../inc/radio_types.h"
#include "../helper/measurements.h"
#include "../helper/storage.h"
#include <string.h>

#define YY(y) ((y) + 4)
#define TABS_Y 0
#define TABS_H 8
#define STATUS_Y 8
#define LIST_Y 16
#define LIST_H (LCD_HEIGHT - LIST_Y)

extern LootItem gLoot[]; extern uint8_t gLootCount; extern uint32_t gLootTotal;
extern ScanEntry gChannels[]; extern uint16_t gChannelCount;
extern uint16_t gScanList[]; extern uint16_t gScanCount;
extern BlackItem gBlack[]; extern uint8_t gBlackCount;
extern uint32_t gScanCurrentFreq;
extern uint32_t gScanProgress;
extern uint32_t gListenFreq;
extern bool gListenAudioOn;
extern void ch_save(void);

#define PCOUNT 5
static const uint32_t gPresets[PCOUNT][3] = {
    {14400000,17600000,2500},{40000000,47000000,2500},
    {13600000,17400000,2500},{42000000,45000000,1250},{8800000,10800000,1000},
};

typedef enum { TAB_RANGE, TAB_LOOT, TAB_CH, TAB_SL, TAB_BL, TAB_SET, TAB_NUM } Tab;
static Tab gTab;
static uint16_t gScroll, gCursor;
static bool gScanning;

static void save_settings(void) {
    ScanSettings *s = APP_GetSettings();
    Storage_Save("settings.cfg", 0, s, sizeof(ScanSettings));
}

void UI_Init(void) { gTab = TAB_RANGE; gScroll = 0; gCursor = 0; }

void ui_draw_scan_progress(uint32_t freq) {
    gScanning = 1;
    FillRect(0, LIST_Y, LCD_WIDTH, LIST_H, C_CLEAR);
    PrintSmall(2, YY(LIST_Y + 2), ">> %lu.%05lu", freq / 100000, freq % 100000);
    // прогресс-бар
    uint8_t bar_w = (uint8_t)(gScanProgress * 100 / 1000);
    if (bar_w > 100) bar_w = 100;
    FillRect(2, YY(LIST_Y + 12), bar_w * 124 / 100, 3, C_FILL);
    PrintSmallEx(LCD_XCENTER, YY(LIST_Y + 20), POS_C, C_FILL, "%u%%", (unsigned)(gScanProgress / 10));
    ST7565_Blit();
}

static void draw_tabs(void) {
    static const char *n[TAB_NUM] = {"Rng","Loot","CH","SL","BL","Set"};
    uint8_t tw = LCD_WIDTH / TAB_NUM;
    for (uint8_t i = 0; i < TAB_NUM; i++)
        PrintSmallEx(i * tw + tw / 2, YY(TABS_Y + 1), POS_C, C_FILL, "%s", n[i]);
    FillRect(gTab * tw, TABS_Y, tw, TABS_H, C_INVERT);
}

static void draw_status(void) {
    if (APP_GetState() != APP_IDLE) {
        PrintSmall(1, YY(STATUS_Y + 1), ">> %lu.%05lu", gScanCurrentFreq / 100000, gScanCurrentFreq % 100000);
    } else {
        PrintSmall(1, YY(STATUS_Y + 1), "ch%u l%u", gChannelCount, gLootCount);
    }
    if (USB_CDC_IsReady()) PrintSmallEx(LCD_WIDTH - 1, YY(STATUS_Y + 1), POS_R, C_FILL, "CDC");
}

static void draw_list(void) {
    FillRect(0, LIST_Y, LCD_WIDTH, LIST_H, C_CLEAR);
    uint8_t max = LIST_H / 8;

    switch (gTab) {
    case TAB_RANGE:
        for (uint8_t i = 0; i < PCOUNT && i < max; i++) {
            char m = (i == gCursor) ? '>' : ' ';
            PrintSmall(2, YY(LIST_Y + 2 + i*8), "%c%lu-%lu %lu.%lu", m,
                       gPresets[i][0]/100000, gPresets[i][1]/100000,
                       gPresets[i][2]/100, gPresets[i][2]%100);
        }
        break;
    case TAB_LOOT:
        if (!gLootCount) { PrintSmall(2, YY(LIST_Y+2), "empty"); break; }
        for (uint8_t i = 0; i < max && gScroll+i < gLootCount; i++) {
            char m = (i == gCursor) ? '>' : ' ';
            uint32_t f = gLoot[gScroll+i].freq;
            PrintSmall(2, YY(LIST_Y+2+i*8), "%c%lu.%05lu %ddBm", m, f/100000, f%100000,
                       Rssi2DBm(gLoot[gScroll+i].rssi_peak));
        }
        break;
    case TAB_CH:
        if (!gChannelCount) { PrintSmall(2, YY(LIST_Y+2), "empty"); break; }
        for (uint8_t l = 0; l < max && gScroll+l < gChannelCount; l++) {
            char m = (l == gCursor) ? '>' : ' ';
            uint32_t f = gChannels[gScroll+l].freq;
            PrintSmall(2, YY(LIST_Y+2+l*8), "%c%lu.%05lu", m, f/100000, f%100000);
        }
        break;
    case TAB_SL:
        if (!gScanCount) { PrintSmall(2, YY(LIST_Y+2), "empty"); break; }
        for (uint8_t i = 0; i < max && gScroll+i < gScanCount; i++) {
            uint16_t ci = gScanList[gScroll+i];
            if (ci < gChannelCount) {
                uint32_t f = gChannels[ci].freq;
                PrintSmall(2, YY(LIST_Y+2+i*8), "%lu.%05lu", f/100000, f%100000);
            }
        }
        break;
    case TAB_BL:
        if (!gBlackCount) { PrintSmall(2, YY(LIST_Y+2), "empty"); break; }
        for (uint8_t i = 0; i < max && gScroll+i < gBlackCount; i++) {
            BlackItem *b = &gBlack[gScroll+i];
            if (b->hi == 0) PrintSmall(2, YY(LIST_Y+2+i*8), "%lu.%05lu", b->lo/100000, b->lo%100000);
            else PrintSmall(2, YY(LIST_Y+2+i*8), "%lu-%lu", b->lo/100000, b->hi/100000);
        }
        break;
    case TAB_SET: {
        ScanSettings *s = APP_GetSettings();
        static const char *action_names[] = {"skip","+WL","+BL","lstn"};
        const char *mark[3] = {" "," "," "};
        if (gCursor < 3) mark[gCursor] = ">";
        PrintSmall(2, YY(LIST_Y+2), "%snoise:%u (%c)", mark[0], s->noise_threshold, 127);
        PrintSmall(2, YY(LIST_Y+10), "%sact:%s", mark[1], action_names[s->scan_action]);
        PrintSmall(2, YY(LIST_Y+18), "%sdwell:%ums", mark[2], s->dwell_ms);
        break;
    default: break;
    }
}
}

void UI_Draw(void) {
    FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, C_CLEAR);
    draw_tabs();
    draw_status();
    draw_list();
    ST7565_Blit();
    gScanning = 0; (void)gScanning;
}

static void on_enter(void);
static void on_updown(int dir);

void UI_HandleKey(KEY_Code_t key, KEY_State_t state) {
    if (state != KEY_RELEASED) return;
    uint8_t max = LIST_H / 8;
    ScanSettings *s = APP_GetSettings();

    switch (key) {
    case KEY_4: gTab = (gTab + TAB_NUM - 1) % TAB_NUM; gScroll = 0; gCursor = 0; break;
    case KEY_6: gTab = (gTab + 1) % TAB_NUM; gScroll = 0; gCursor = 0; break;

    case KEY_UP:
        if (gTab == TAB_RANGE) { if (gCursor > 0) gCursor--; }
        else if (gTab == TAB_SET) { if (gCursor > 0) gCursor--; }
        else if (gScroll + gCursor > 0) {
            if (gCursor > 0) gCursor--;
            else if (gScroll > 0) gScroll--;
        }
        break;
    case KEY_DOWN:
        if (gTab == TAB_RANGE) { if (gCursor + 1 < PCOUNT) gCursor++; }
        else if (gTab == TAB_SET) { if (gCursor < 2) gCursor++; }
        else {
            uint16_t cnt = (gTab == TAB_LOOT) ? gLootCount : (gTab == TAB_CH) ? gChannelCount : (gTab == TAB_BL) ? gBlackCount : gTab == TAB_SL ? gScanCount : 0;
            if (cnt > 0 && gScroll + gCursor + 1 < cnt) {
                if (gCursor + 1 < max) gCursor++; else gScroll++;
            }
        }
        break;

    case KEY_SIDE1: // BL
        if (APP_GetState() != APP_IDLE) {
            // во время скана — BL текущей частоты
            uint32_t f = gScanCurrentFreq;
            if (f && gBlackCount < BL_MAX) {
                gBlack[gBlackCount].lo = f; gBlack[gBlackCount].hi = 0; gBlackCount++;
            }
        } else if (gTab == TAB_LOOT && gLootCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gLootCount && gBlackCount < BL_MAX) {
                gBlack[gBlackCount].lo = gLoot[idx].freq; gBlack[gBlackCount].hi = 0; gBlackCount++;
            }
        }
        break;
    case KEY_SIDE2: // WL
        if (APP_GetState() != APP_IDLE) {
            // во время скана — WL текущей частоты
            uint32_t f = gScanCurrentFreq;
            if (f && gChannelCount < MAX_CHANNELS) {
                gChannels[gChannelCount].freq = f; gChannels[gChannelCount].freq_end = 0;
                gChannels[gChannelCount].squelch = 3; gChannels[gChannelCount].step_khz = 0;
                gChannels[gChannelCount].flags = 0; gChannels[gChannelCount].name[0] = 0;
                gChannelCount++; ch_save();
            }
        } else if (gTab == TAB_LOOT && gLootCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gLootCount && gChannelCount < MAX_CHANNELS) {
                gChannels[gChannelCount].freq = gLoot[idx].freq;
                gChannels[gChannelCount].freq_end = 0; gChannels[gChannelCount].squelch = 3;
                gChannels[gChannelCount].step_khz = 0; gChannels[gChannelCount].flags = 0;
                gChannels[gChannelCount].name[0] = 0; gChannelCount++; ch_save();
            }
        } else if (gTab == TAB_RANGE) {
            APP_StartFullScan();
        } else if (gTab == TAB_SL && gScanCount > 0) {
            APP_StartFullScan();
        }
        break;

    case KEY_EXIT:
        if (APP_GetState() != APP_IDLE) { APP_Stop(); break; }
        if (gTab == TAB_CH && gChannelCount > 0) {
            uint16_t ci = gScroll + gCursor;
            if (ci < gChannelCount) {
                memmove(&gChannels[ci], &gChannels[ci+1], (gChannelCount-ci-1)*sizeof(ScanEntry));
                gChannelCount--; ch_save();
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gChannelCount) gScroll--;
            }
        } else if (gTab == TAB_BL && gBlackCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gBlackCount) {
                memmove(&gBlack[idx], &gBlack[idx+1], (gBlackCount-idx-1)*sizeof(BlackItem));
                gBlackCount--;
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gBlackCount) gScroll--;
            }
        } else if (gTab == TAB_LOOT && gLootCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gLootCount) {
                memmove(&gLoot[idx], &gLoot[idx+1], (gLootCount-idx-1)*sizeof(LootItem));
                gLootCount--;
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gLootCount) gScroll--;
            }
        }
        break;

    case KEY_STAR:
        if (gTab == TAB_SET) {
            if (gCursor == 0 && s->noise_threshold < 200) s->noise_threshold++;
            else if (gCursor == 1 && s->scan_action < 3) s->scan_action++;
            else if (gCursor == 2 && s->dwell_ms < 100) s->dwell_ms++;
            save_settings();
        }
        break;

    case KEY_F:
        if (gTab == TAB_SET) {
            if (gCursor == 0 && s->noise_threshold > 10) s->noise_threshold--;
            else if (gCursor == 1 && s->scan_action > 0) s->scan_action--;
            else if (gCursor == 2 && s->dwell_ms > 2) s->dwell_ms--;
            save_settings();
        }
        break;

    default: break;
    }
}
