#include "ui.h"
#include "app.h"
#include "../driver/keyboard.h"
#include "../driver/st7565.h"
#include "../driver/usb_cdc.h"
#include "../driver/systick.h"
#include "../ui/graphics.h"
#include "../inc/radio_types.h"
#include "../helper/measurements.h"
#include <string.h>

#define YY(y) ((y) + 4)
#define TABS_Y 0
#define TABS_H 7
#define STATUS_Y (TABS_Y + TABS_H)
#define LIST_Y (STATUS_Y + 7)
#define LIST_H (LCD_HEIGHT - LIST_Y)

extern LootItem gLoot[]; extern uint8_t gLootCount; extern uint32_t gLootTotal;
extern ScanEntry gChannels[]; extern uint16_t gChannelCount;
extern uint16_t gScanList[]; extern uint16_t gScanCount;
extern BlackItem gBlack[]; extern uint8_t gBlackCount;
extern void ch_save(void);

#define PCOUNT 5
static const uint32_t gPresets[PCOUNT][3] = {
    {14400000, 17600000, 2500},
    {40000000, 47000000, 2500},
    {13600000, 17400000, 2500},
    {42000000, 45000000, 1250},
    {8800000, 10800000, 1000},
};

typedef enum { TAB_RANGE, TAB_LOOT, TAB_CH, TAB_SL, TAB_BL, TAB_NUM } Tab;
static Tab gTab;
static uint16_t gScroll, gCursor;
static uint32_t gLastProgDraw;
static bool gIsScanning;

void UI_Init(void) { gTab = TAB_RANGE; gScroll = 0; gCursor = 0; }

void ui_draw_scan_progress(uint32_t freq) {
    if (Now() - gLastProgDraw < 100) return;
    gLastProgDraw = Now(); gIsScanning = 1;
    FillRect(0, STATUS_Y, LCD_WIDTH, 7, C_FILL);
    PrintSmallEx(LCD_XCENTER, YY(STATUS_Y), POS_C, C_FILL, ">> %lu.%05lu", freq / 100000, freq % 100000);
    ST7565_Blit();
}

static void draw_tabs(void) {
    static const char *n[TAB_NUM] = {"Rng","Loot","CH","SL","BL"};
    uint8_t tw = LCD_WIDTH / TAB_NUM;
    for (uint8_t i = 0; i < TAB_NUM; i++)
        PrintSmallEx(i * tw + tw / 2, YY(TABS_Y), POS_C, C_FILL, "%s", n[i]);
    FillRect(gTab * tw, TABS_Y, tw, TABS_H, C_INVERT);
}

static void draw_status(void) {
    if (gIsScanning) return; // прогресс поверх
    FillRect(0, STATUS_Y, LCD_WIDTH, 7, C_CLEAR);
    PrintSmall(1, YY(STATUS_Y), "ch%u l%u", gChannelCount, gLootCount);
    if (USB_CDC_IsReady()) PrintSmallEx(LCD_WIDTH - 1, YY(STATUS_Y), POS_R, C_FILL, "CDC");
}

static void draw_list(void) {
    FillRect(0, LIST_Y, LCD_WIDTH, LIST_H, C_CLEAR);
    uint8_t max_lines = LIST_H / 8;

    switch (gTab) {
    case TAB_RANGE:
        for (uint8_t i = 0; i < PCOUNT && i < max_lines; i++) {
            char m = (i == gCursor) ? '>' : ' ';
            PrintSmall(2, YY(LIST_Y + i * 8), "%c%lu-%lu %lu.%lu", m,
                       gPresets[i][0] / 100000, gPresets[i][1] / 100000,
                       gPresets[i][2] / 100, gPresets[i][2] % 100);
        }
        break;
    case TAB_LOOT:
        if (gLootCount == 0) { PrintSmall(2, YY(LIST_Y + 2), "empty"); break; }
        for (uint8_t i = 0; i < max_lines && gScroll + i < gLootCount; i++) {
            char m = (i == gCursor) ? '>' : ' ';
            uint32_t f = gLoot[gScroll + i].freq;
            PrintSmall(2, YY(LIST_Y + i * 8), "%c%lu.%05lu %ddBm", m, f / 100000, f % 100000,
                       Rssi2DBm(gLoot[gScroll + i].rssi_peak));
        }
        break;
    case TAB_CH:
        if (gChannelCount == 0) { PrintSmall(2, YY(LIST_Y + 2), "empty"); break; }
        for (uint8_t l = 0; l < max_lines && gScroll + l < gChannelCount; l++) {
            char m = (l == gCursor) ? '>' : ' ';
            uint32_t f = gChannels[gScroll + l].freq;
            PrintSmall(2, YY(LIST_Y + l * 8), "%c%lu.%05lu", m, f / 100000, f % 100000);
        }
        break;
    case TAB_SL:
        if (gScanCount == 0) { PrintSmall(2, YY(LIST_Y + 2), "empty"); break; }
        for (uint8_t i = 0; i < max_lines && gScroll + i < gScanCount; i++) {
            uint16_t ci = gScanList[gScroll + i];
            if (ci < gChannelCount) {
                uint32_t f = gChannels[ci].freq;
                PrintSmall(2, YY(LIST_Y + i * 8), "%lu.%05lu", f / 100000, f % 100000);
            } else PrintSmall(2, YY(LIST_Y + i * 8), "?%u", ci);
        }
        break;
    case TAB_BL:
        if (gBlackCount == 0) { PrintSmall(2, YY(LIST_Y + 2), "empty"); break; }
        for (uint8_t i = 0; i < max_lines && gScroll + i < gBlackCount; i++) {
            BlackItem *b = &gBlack[gScroll + i];
            if (b->hi == 0)
                PrintSmall(2, YY(LIST_Y + i * 8), "%lu.%05lu", b->lo / 100000, b->lo % 100000);
            else
                PrintSmall(2, YY(LIST_Y + i * 8), "%lu-%lu", b->lo / 100000, b->hi / 100000);
        }
        break;
    }
}

void UI_Draw(void) {
    draw_tabs();
    draw_status();
    draw_list();
    ST7565_Blit();
    gIsScanning = 0;
}

void UI_HandleKey(KEY_Code_t key, KEY_State_t state) {
    if (state != KEY_RELEASED) return;
    uint8_t max_lines = LIST_H / 8;

    switch (key) {
    case KEY_MENU:
        gTab = (gTab + 1) % TAB_NUM; gScroll = 0; gCursor = 0; break;

    case KEY_UP:
        if (gTab == TAB_RANGE) { if (gCursor > 0) gCursor--; }
        else if (gTab == TAB_LOOT && gLootCount > 0)
            { if (gCursor > 0) gCursor--; else if (gScroll > 0) gScroll--; }
        else if (gTab == TAB_CH && gChannelCount > 0)
            { if (gCursor > 0) gCursor--; else if (gScroll > 0) gScroll--; }
        else if (gTab == TAB_BL && gBlackCount > 0)
            { if (gCursor > 0) gCursor--; else if (gScroll > 0) gScroll--; }
        break;

    case KEY_DOWN:
        if (gTab == TAB_RANGE) { if (gCursor + 1 < PCOUNT) gCursor++; }
        else if (gTab == TAB_LOOT && gLootCount > 0)
            { if (gScroll + gCursor + 1 < gLootCount) { if (gCursor + 1 < max_lines) gCursor++; else gScroll++; } }
        else if (gTab == TAB_CH && gChannelCount > 0)
            { if (gScroll + gCursor + 1 < gChannelCount) { if (gCursor + 1 < max_lines) gCursor++; else gScroll++; } }
        else if (gTab == TAB_BL && gBlackCount > 0)
            { if (gScroll + gCursor + 1 < gBlackCount) { if (gCursor + 1 < max_lines) gCursor++; else gScroll++; } }
        break;

    case KEY_SIDE1:
        if (gTab == TAB_CH && gChannelCount > 0) APP_Listen(gScroll + gCursor);
        else if (gTab == TAB_LOOT && gLootCount > 0 && gBlackCount < BL_MAX) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gLootCount) { gBlack[gBlackCount].lo = gLoot[idx].freq; gBlack[gBlackCount].hi = 0; gBlackCount++; }
        }
        break;

    case KEY_SIDE2:
        if (gTab == TAB_RANGE && gCursor < PCOUNT)
            APP_StartScan(gPresets[gCursor][0], gPresets[gCursor][1], gPresets[gCursor][2]);
        else if (gTab == TAB_LOOT && gLootCount > 0) {
            for (uint8_t i = 0; i < gLootCount && gChannelCount < MAX_CHANNELS; i++) {
                uint32_t f = gLoot[i].freq; bool dup = 0;
                for (uint16_t c = 0; c < gChannelCount; c++)
                    if (gChannels[c].freq == f && gChannels[c].freq_end == 0) { dup = 1; break; }
                if (dup) continue;
                gChannels[gChannelCount].freq = f; gChannels[gChannelCount].freq_end = 0;
                gChannels[gChannelCount].squelch = 3; gChannels[gChannelCount].step_khz = 0;
                gChannels[gChannelCount].flags = 0; gChannels[gChannelCount].name[0] = 0;
                gChannelCount++;
            }
            ch_save();
        } else if (gTab == TAB_SL && gScanCount > 0) APP_StartScanList(gScanList, gScanCount);
        break;

    case KEY_EXIT:
        APP_Stop();
        if (gTab == TAB_CH && gChannelCount > 0) {
            uint16_t ci = gScroll + gCursor;
            if (ci < gChannelCount) {
                memmove(&gChannels[ci], &gChannels[ci + 1], (gChannelCount - ci - 1) * sizeof(ScanEntry));
                gChannelCount--; ch_save();
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gChannelCount) gScroll--;
            }
        } else if (gTab == TAB_BL && gBlackCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gBlackCount) {
                memmove(&gBlack[idx], &gBlack[idx + 1], (gBlackCount - idx - 1) * sizeof(BlackItem));
                gBlackCount--;
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gBlackCount) gScroll--;
            }
        } else if (gTab == TAB_LOOT && gLootCount > 0) {
            uint16_t idx = gScroll + gCursor;
            if (idx < gLootCount) {
                memmove(&gLoot[idx], &gLoot[idx + 1], (gLootCount - idx - 1) * sizeof(LootItem));
                gLootCount--;
                if (gCursor > 0) gCursor--;
                if (gScroll > 0 && gScroll >= gLootCount) gScroll--;
            }
        }
        break;

    default: break;
    }
}
