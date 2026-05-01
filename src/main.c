#include <string.h>
#include "driver/backlight.h"
#include "driver/debug.h"
#include "driver/gpio.h"
#include "driver/hrtime.h"
#include "driver/st7565.h"
#include "driver/systick.h"
#include "driver/usb_cdc.h"
#include "driver/bk4829.h"
#include "helper/measurements.h"
#include "helper/storage.h"
#include "inc/radio_types.h"
#include "ui/graphics.h"
#include "board.h"

#define OUT(fmt,...) do { char _b[80]; int _n = snprintf(_b,80,fmt "\n",##__VA_ARGS__); USB_CDC_Write((uint8_t*)_b,_n); } while(0)

static ParamSet gPS;
static LootItem gLoot[LOOT_MAX];
static uint8_t gLootCount;
static uint16_t gLootTotal;
static ScanEntry gChannels[MAX_CHANNELS];
static uint16_t gChannelCount;
static uint16_t gScanList[MAX_CHANNELS];
static uint16_t gScanCount;
static RxProfile gRxProfiles[MAX_RX_PROFILES];
static BlackItem gBlack[BL_MAX];
static uint8_t gBlackCount;

// ── helpers ─────────────────────────────────────────────────────────────
static bool black_has(uint32_t f) {
    for (uint8_t i = 0; i < gBlackCount; i++) {
        if (gBlack[i].hi == 0) { if (gBlack[i].lo == f) return 1; }
        else { if (f >= gBlack[i].lo && f <= gBlack[i].hi) return 1; }
    }
    return 0;
}

static void ch_save(void) {
    ChannelsFile cf; cf.count = gChannelCount;
    memcpy(cf.e, gChannels, gChannelCount * sizeof(ScanEntry));
    Storage_Save("channels.ch", 0, &cf, sizeof(cf));
}

static void add_channel(uint32_t freq, uint8_t sql) {
    if (gChannelCount >= MAX_CHANNELS) return;
    gChannels[gChannelCount].freq = freq;
    gChannels[gChannelCount].dwell_ms = 500;
    gChannels[gChannelCount].squelch = sql;
    gChannels[gChannelCount].rx_profile = 0;
    gChannels[gChannelCount].flags = 0;
    gChannelCount++;
    ch_save();
}

static void scan_freq(uint32_t f, uint8_t sql, int wl) {
    if (black_has(f)) return;
    BK4819_TuneTo(f, 0);
    BK4819_Squelch(sql, f, 1, 1);
    SYSTICK_DelayUs(10000);
    uint16_t r = BK4819_GetRSSI();
    uint8_t n = BK4819_GetNoise();
    if (n > 46) return;
    LootItem l;
    l.freq = f; l.rssi_peak = r; l.glitch_min = n;
    l.seen_ago = 0; l.hit_count = 1; l.rx_profile = 0; l.ctcss = 0;
    l.flags = LOOT_CONFIRMED;
    if (BK4819_IsSquelchOpen()) {
        uint16_t cf; uint32_t dc;
        BK4819_CssScanResult_t css = BK4819_GetCxCSSScanResult(&dc, &cf);
        if (css == BK4819_CSS_RESULT_CTCSS) { l.ctcss = cf / 10; l.flags |= LOOT_HAS_CTCSS; }
        else if (css == BK4819_CSS_RESULT_CDCSS) { l.ctcss = 0xFF; l.flags |= LOOT_HAS_DCS; }
    }
    uint32_t idx = gLootTotal++;
    Storage_Save("loot.tmp", 0, &gLootTotal, 4);
    Storage_Save("loot.tmp", 1 + idx, &l, sizeof(LootItem));
    if (gLootCount >= LOOT_MAX) memmove(gLoot, gLoot + 1, (LOOT_MAX - 1) * sizeof(LootItem));
    else gLootCount++;
    gLoot[gLootCount - 1] = l;
    if (wl) { add_channel(f, sql); }
    OUT("  %lu.%05lu %ddBm%s", f / 100000, f % 100000, Rssi2DBm(r), wl ? " +W" : "");
}

static int kv_int(const char *p, const char *k) {
    while (*p) {
        const char *a = p, *b = k;
        while (*a && *a == *b && *b) { a++; b++; }
        if (*b == 0 && *a == '=') { a++; int v = 0; while (*a >= '0' && *a <= '9') v = v*10+*a++ - '0'; return v; }
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
    }
    return -1;
}

static void do_scan_range(char *p, uint32_t a, uint32_t b, uint32_t step) {
    int sql = kv_int(p, "sql"); if (sql < 0) sql = 3;
    int wl = kv_int(p, "wl");
    OUT("SCAN %lu..%lu step=%lu", a / 100000, b / 100000, step / 100);
    for (uint32_t f = a; f <= b; f += step) scan_freq(f, (uint8_t)sql, wl == 1);
    OUT("DONE");
}

// парсинг "123.45678" из строки, возвращает значение в единицах *100000
static uint32_t parse_mhz(const char **pp) {
    const char *p = *pp;
    uint32_t m = 0, f = 0;
    while (*p >= '0' && *p <= '9') { m = m * 10 + (*p - '0'); p++; }
    if (*p == '.') { p++; while (*p >= '0' && *p <= '9') { f = f * 10 + (*p - '0'); p++; } }
    *pp = p;
    return m * 100000 + f;
}

static uint32_t parse_khz(const char **pp) {
    const char *p = *pp;
    uint32_t m = 0, f = 0;
    while (*p >= '0' && *p <= '9') { m = m * 10 + (*p - '0'); p++; }
    if (*p == '.') { p++; while (*p >= '0' && *p <= '9') { f = f * 10 + (*p - '0'); p++; } }
    *pp = p;
    return m * 100 + f;
}

int main(void) {
    SYSTICK_Init(); HRTIME_Init(); BOARD_Init();
    BACKLIGHT_SetBrightness(8); GPIO_TurnOnBacklight();
    ST7565_Init(); ST7565_SetContrast(8); ST7565_FillScreen(0); ST7565_Blit();
    USB_CDC_Init(); DBG_Init();
    BK4819_Init(); BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, 1);
    BK4819_RX_TurnOn(); BK4819_SetAFC(0); BK4819_SetAGC(1, 1);
    BK4819_SetFilterBandwidth(BK4819_FILTER_BW_12k); BK4819_SelectFilterEx(FILTER_UHF);
    ChannelsFile cf;
    if (Storage_Load("channels.ch", 0, &cf, sizeof(cf)) && cf.count <= MAX_CHANNELS)
        { gChannelCount = cf.count; memcpy(gChannels, cf.e, gChannelCount * sizeof(ScanEntry)); }
    ScanListIdx sl;
    if (Storage_Load("scanlist.sl", 0, &sl, sizeof(sl)) && sl.count <= MAX_CHANNELS)
        { gScanCount = sl.count; memcpy(gScanList, sl.idx, gScanCount * 2); }
    if (Storage_Load("blacklist.sl", 0, gBlack, sizeof(gBlack)))
        gBlackCount = sizeof(gBlack) / sizeof(BlackItem);
    OUT("ready ch=%u", gChannelCount);
    uint32_t tScr = 0;
    char line[80]; uint8_t lpos = 0;
    for (;;) {
        USB_CDC_Poll();
        uint8_t buf[64];
        uint32_t n = USB_CDC_Read(buf, 64);
        for (uint32_t i = 0; i < n; i++) {
            char c = buf[i];
            if (c == '\r' || c == '\n') {
                if (lpos == 0) continue;
                line[lpos] = 0; lpos = 0;

                if (memcmp(line, "SCAN_SL", 7) == 0) {
                    // SCAN_SL <name>
                    char name[32] = "scanlist_";
                    const char *pp = line + 7; while (*pp == ' ') pp++;
                    int ni = 9; while (*pp && *pp != ' ' && ni < 30) name[ni++] = *pp++;
                    name[ni] = 0;
                    ScanListIdx sl2; sl2.count = 0;
                    Storage_Load(name, 0, &sl2, sizeof(sl2));
                    if (sl2.count == 0) OUT("NO %s", name);
                    else {
                        OUT("SCAN %s %u", name, sl2.count);
                        for (uint16_t si = 0; si < sl2.count; si++) {
                            uint16_t ci = sl2.idx[si];
                            if (ci < gChannelCount) scan_freq(gChannels[ci].freq, gChannels[ci].squelch, 0);
                        }
                        OUT("DONE");
                    }
                } else if (line[0] == 'S') {
                    const char *pp = line + 1; while (*pp == ' ') pp++;
                    uint32_t a = parse_mhz(&pp);
                    while (*pp == ' ') pp++;
                    uint32_t b = parse_mhz(&pp);
                    while (*pp == ' ') pp++;
                    uint32_t step = parse_khz(&pp);
                    if (step == 0) step = 2500;
                    do_scan_range((char*)pp, a, b, step);
                } else if (memcmp(line, "CH_ADD", 6) == 0) {
                    const char *pp = line + 6; while (*pp == ' ') pp++;
                    uint32_t f = parse_mhz(&pp);
                    add_channel(f, 3);
                    OUT("CH%u %lu.%05lu", gChannelCount - 1, f / 100000, f % 100000);
                } else if (memcmp(line, "CH_LS", 5) == 0) {
                    OUT("%u channels", gChannelCount);
                    for (uint16_t i = 0; i < gChannelCount; i++)
                        OUT("  %u: %lu.%05lu", i, gChannels[i].freq / 100000, gChannels[i].freq % 100000);
                } else if (memcmp(line, "CH_RM", 5) == 0) {
                    int idx = 0; sscanf(line + 5, "%d", &idx);
                    if (idx >= 0 && idx < gChannelCount) {
                        memmove(&gChannels[idx], &gChannels[idx + 1], (gChannelCount - idx - 1) * sizeof(ScanEntry));
                        gChannelCount--; ch_save(); OUT("OK");
                    }
                } else if (memcmp(line, "SL_ADD", 6) == 0) {
                    const char *pp = line + 6;
                    while (*pp && gScanCount < MAX_CHANNELS) {
                        while (*pp == ' ') pp++;
                        int idx = 0; while (*pp >= '0' && *pp <= '9') idx = idx*10 + *pp++ - '0';
                        if (idx < gChannelCount) gScanList[gScanCount++] = idx;
                    }
                    ScanListIdx sl2; sl2.count = gScanCount; memcpy(sl2.idx, gScanList, gScanCount * 2);
                    Storage_Save("scanlist.sl", 0, &sl2, sizeof(sl2));
                    OUT("SL %u", gScanCount);
                } else if (memcmp(line, "SL_WR", 5) == 0) {
                    char name[32] = "scanlist_";
                    const char *pp = line + 5; while (*pp == ' ') pp++;
                    int ni = 9; while (*pp && *pp != ' ' && ni < 30) name[ni++] = *pp++;
                    name[ni] = 0;
                    uint16_t tmp[MAX_CHANNELS], tc = 0;
                    while (*pp && tc < MAX_CHANNELS) {
                        while (*pp == ' ') pp++;
                        int idx = 0; while (*pp >= '0' && *pp <= '9') idx = idx*10 + *pp++ - '0';
                        if (idx < gChannelCount) tmp[tc++] = idx;
                    }
                    ScanListIdx sl2; sl2.count = tc; memcpy(sl2.idx, tmp, tc * 2);
                    Storage_Save(name, 0, &sl2, sizeof(sl2));
                    OUT("%s %u", name, tc);
                } else if (memcmp(line, "SL_LS", 5) == 0) {
                    OUT("active %u items", gScanCount);
                    for (uint16_t i = 0; i < gScanCount; i++)
                        OUT("  %u->%lu.%05lu", i, gChannels[gScanList[i]].freq / 100000, gChannels[gScanList[i]].freq % 100000);
                } else if (memcmp(line, "BL_ADD", 6) == 0) {
                    const char *pp = line + 6; while (*pp == ' ') pp++;
                    uint32_t a = parse_mhz(&pp);
                    while (*pp == ' ') pp++;
                    uint32_t b = parse_mhz(&pp);
                    if (gBlackCount < BL_MAX) {
                        gBlack[gBlackCount].lo = a; gBlack[gBlackCount].hi = (b > a) ? b : 0;
                        gBlackCount++;
                        if (b > a) OUT("BL RANGE %lu.%05lu..%lu.%05lu", a / 100000, a % 100000, b / 100000, b % 100000);
                        else OUT("BL %lu.%05lu", a / 100000, a % 100000);
                    }
                } else if (memcmp(line, "BL_LS", 5) == 0) {
                    for (uint8_t i = 0; i < gBlackCount; i++) {
                        if (gBlack[i].hi == 0) OUT("  %lu.%05lu", gBlack[i].lo / 100000, gBlack[i].lo % 100000);
                        else OUT("  %lu.%05lu..%lu.%05lu", gBlack[i].lo / 100000, gBlack[i].lo % 100000, gBlack[i].hi / 100000, gBlack[i].hi % 100000);
                    }
                } else if (memcmp(line, "BL_WR", 5) == 0) {
                    Storage_Save("blacklist.sl", 0, gBlack, gBlackCount * sizeof(BlackItem)); OUT("OK");
                } else if (memcmp(line, "BL_RD", 5) == 0) {
                    if (Storage_Load("blacklist.sl", 0, gBlack, sizeof(gBlack))) gBlackCount = sizeof(gBlack) / sizeof(BlackItem); else gBlackCount = 0; OUT("%u", gBlackCount);
                } else if (memcmp(line, "LW_LS", 5) == 0) {
                    OUT("total=%u", gLootTotal);
                    for (uint8_t i = 0; i < gLootCount; i++) OUT("  %u: %lu.%05lu %ddBm", i, gLoot[i].freq / 100000, gLoot[i].freq % 100000, Rssi2DBm(gLoot[i].rssi_peak));
                } else if (memcmp(line, "LW_CLR", 6) == 0) {
                    gLootCount = 0; gLootTotal = 0; uint32_t z = 0; Storage_Save("loot.tmp", 0, &z, 4); OUT("OK");
                } else if (memcmp(line, "PR_WR", 5) == 0) {
                    int idx = 0, gain = 0, sql = 0, mod = 0; uint32_t m = 0, f = 0;
                    sscanf(line + 5, "%d %lu.%lu %d %d %d", &idx, &m, &f, &gain, &sql, &mod);
                    if (idx >= 0 && idx < MAX_RX_PROFILES) {
                        RxProfile *r = &gRxProfiles[idx];
                        r->freq_anchor = m * 100000 + f; r->gain_index = gain; r->squelch_level = sql; r->modulation = mod; r->bw = 4; r->agc_enable = 1;
                        OUT("OK");
                    }
                } else if (memcmp(line, "PR_LS", 5) == 0) {
                    for (uint8_t i = 0; i < MAX_RX_PROFILES; i++) if (gRxProfiles[i].freq_anchor) OUT("%u %lu", i, gRxProfiles[i].freq_anchor);
                } else if (line[0] == 'U') { OUT("%lu ms", DBG_GetUptimeMs());
                } else if (line[0] == 'K') { uint32_t reg = 0; sscanf(line + 1, "%lx", &reg); OUT("R%02lX=0x%04X", reg, BK4819_ReadRegister((BK4819_REGISTER_t)reg));
                } else if (line[0] == 'h' || line[0] == '?') {
                    USB_CDC_WriteString(
                        "S a b st [k=v] range scan\n"
                        "CH_ADD f       add channel\n"
                        "CH_LS          list channels\n"
                        "CH_RM i        remove channel\n"
                        "SL_ADD i..     add to active SL\n"
                        "SL_WR n i..    write named SL\n"
                        "SL_LS          list active SL\n"
                        "SCAN_SL n      scan named SL\n"
                        "BL_ADD f [f2]  blacklist freq/range\n"
                        "BL_LS          list blacklist\n"
                        "BL_WR          save blacklist\n"
                        "BL_RD          load blacklist\n"
                        "LW_LS          list loot\n"
                        "LW_CLR         clear loot\n"
                        "PR_WR i f g s m  profile write\n"
                        "PR_LS          list profiles\n"
                        "U / K hex      uptime / regread\n");
                }
            } else if (lpos < sizeof(line) - 1) { line[lpos++] = c; }
        }
        if (Now() - tScr > 1000) {
            tScr = Now(); FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, C_CLEAR);
            PrintSmallEx(LCD_XCENTER, 5, POS_C, C_FILL, "k1-scaner");
            PrintSmallEx(LCD_XCENTER, LCD_HEIGHT, POS_C, C_FILL, "ch:%u", gChannelCount);
            ST7565_Blit();
        }
        __WFI();
    }
}
