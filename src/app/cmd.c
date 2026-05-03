#include "cmd.h"
#include "../inc/radio_types.h"
#include "../driver/bk4829.h"
#include "../driver/debug.h"
#include "../driver/lfs.h"
#include "../external/printf/printf.h"
#include "../external/CMSIS/Device/PY32F071/Include/py32f0xx.h"
#include "../driver/st7565.h"
#include "../driver/systick.h"
#include "../driver/usb_cdc.h"
#include "../helper/measurements.h"
#include "../helper/storage.h"
#include "../ui/graphics.h"
#include <string.h>

// ── внешние данные ─────────────────────────────────────────────────────
extern LootItem gLoot[]; extern uint8_t gLootCount; extern uint32_t gLootTotal;
extern ScanEntry gChannels[]; extern uint16_t gChannelCount;
extern uint16_t gScanList[]; extern uint16_t gScanCount;
extern RxProfile gRxProfiles[]; extern BlackItem gBlack[]; extern uint8_t gBlackCount;
extern void ch_save(void);

// ── парсеры ────────────────────────────────────────────────────────────
int parse_int(const char **pp) {
    int v = 0; while (**pp == ' ') (*pp)++;
    while (**pp >= '0' && **pp <= '9') v = v * 10 + *(*pp)++ - '0'; return v;
}
uint32_t parse_mhz(const char **pp) {
    uint32_t m = 0, f = 0, d = 0;
    while (**pp == ' ') (*pp)++;
    while (**pp >= '0' && **pp <= '9') m = m * 10 + *(*pp)++ - '0';
    if (**pp == '.') { (*pp)++; while (**pp >= '0' && **pp <= '9') { f = f * 10 + *(*pp)++ - '0'; d++; } }
    while (d < 5) f *= 10, d++;
    return m * 100000 + f;
}
uint32_t parse_khz(const char **pp) {
    uint32_t m = 0, f = 0;
    while (**pp == ' ') (*pp)++;
    while (**pp >= '0' && **pp <= '9') m = m * 10 + *(*pp)++ - '0';
    if (**pp == '.') { (*pp)++; while (**pp >= '0' && **pp <= '9') f = f * 10 + *(*pp)++ - '0'; }
    return m * 100 + f;
}
uint32_t parse_hex(const char **pp) {
    uint32_t v = 0; while (**pp == ' ') (*pp)++;
    while (**pp) {
        char c = **pp;
        if (c >= '0' && c <= '9') v = v * 16 + c - '0';
        else if (c >= 'a' && c <= 'f') v = v * 16 + c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = v * 16 + c - 'A' + 10;
        else break;
        (*pp)++;
    }
    return v;
}
static void skip_sp(const char **pp) { while (**pp == ' ') (*pp)++; }

// ── вывод ──────────────────────────────────────────────────────────────
#define OUT(s) USB_CDC_Write((uint8_t*)(s), strlen(s))
#define OUTF(fmt,...) do { char _b[64]; snprintf_(_b,64,fmt "\n",##__VA_ARGS__); int _l = strlen(_b); if(_l > 0) USB_CDC_Write((uint8_t*)_b, _l); } while(0)

// ── обработчики команд ─────────────────────────────────────────────────
typedef void (*cmd_fn)(const char *);
typedef struct { const char *name; cmd_fn fn; } Cmd;

static void cmd_h(const char *a) {
    (void)a; OUT(
        "S a b st [k=v]   range scan\n"
        "CH_ADD f         add channel\n"
        "SCAN_ADD l h st  add range\n"
        "CH_LS            list ch\n"
        "CH_RM i          rm ch\n"
        "SL_ADD i..       add to SL\n"
        "SL_WR n i..      write SL\n"
        "SL_LS            list SL\n"
        "G                scan SL\n"
        "SCAN_SL n        scan named SL\n"
        "BL_ADD f [f2]    blacklist\n"
        "BL_LS/WR/RD      bl ctrl\n"
        "LW_LS/CLR        loot\n"
        "PR_WR/LS         profiles\n"
        "LS/CAT/RM        files\n"
        "SCR              screenshot\n"
        "SPECTRUM l h st  csv dump\n"
        "REBOOT           reboot\n"
        "RESET            clear data\n"
        "U / K hex        uptime / reg\n"
        "K_WR hex val     reg write\n");
}

static void cmd_U(const char *a) { (void)a; OUTF("%lu ms", DBG_GetUptimeMs()); }

static void cmd_RESET(const char *a) {
    (void)a;
    gChannelCount = 0; gScanCount = 0; gLootCount = 0; gLootTotal = 0; gBlackCount = 0;
    ch_save();
    ScanListIdx sl; sl.count = 0; Storage_Save("scanlist.sl", 0, &sl, sizeof(sl));
    uint32_t z = 0; Storage_Save("loot.tmp", 0, &z, 4);
    Storage_Save("blacklist.sl", 0, &z, 4);
    OUT("OK\n");
}

static void cmd_REBOOT(const char *a) {
    (void)a; OUT("OK\n"); SYSTICK_DelayMs(100); NVIC_SystemReset();
}

static void cmd_CH_ADD(const char *a) {
    uint32_t f = parse_mhz(&a);
    if (gChannelCount < MAX_CHANNELS) {
        ScanEntry *e = &gChannels[gChannelCount];
        e->freq = f; e->freq_end = 0; e->squelch = 3; e->step_khz = 0;
        e->flags = 0; e->name[0] = 0; gChannelCount++; ch_save();
    }
    OUTF("CH%u", gChannelCount - 1);
}

static void cmd_SCAN_ADD(const char *a) {
    uint32_t lo = parse_mhz(&a), hi = parse_mhz(&a), st = parse_khz(&a);
    if (gChannelCount < MAX_CHANNELS) {
        ScanEntry *e = &gChannels[gChannelCount];
        e->freq = lo; e->freq_end = hi; e->squelch = 3; e->step_khz = (uint8_t)(st / 100);
        e->flags = 0; e->name[0] = 0; gChannelCount++; ch_save();
    }
    OUT("OK\n");
}

static void cmd_CH_LS(const char *a) { (void)a;
    OUTF("%u channels", gChannelCount);
    for (uint16_t i = 0; i < gChannelCount; i++) {
        ScanEntry *e = &gChannels[i];
        if (e->freq_end)
            OUTF("  %u: %lu.%05lu-%lu.%05lu", i, e->freq / 100000, e->freq % 100000, e->freq_end / 100000, e->freq_end % 100000);
        else
            OUTF("  %u: %lu.%05lu", i, e->freq / 100000, e->freq % 100000);
    }
}

static void cmd_CH_RM(const char *a) {
    int idx = parse_int(&a);
    if (idx >= 0 && idx < gChannelCount) {
        memmove(&gChannels[idx], &gChannels[idx + 1], (gChannelCount - idx - 1) * sizeof(ScanEntry));
        gChannelCount--; ch_save();
    }
    OUT("OK\n");
}

static void cmd_SL_ADD(const char *a) {
    while (*a && gScanCount < MAX_CHANNELS) {
        int idx = parse_int(&a);
        if (idx < gChannelCount) gScanList[gScanCount++] = idx;
    }
    ScanListIdx sl; sl.count = gScanCount;
    memcpy(sl.idx, gScanList, gScanCount * 2);
    Storage_Save("scanlist.sl", 0, &sl, sizeof(sl));
    OUTF("SL %u", gScanCount);
}

static void cmd_SL_WR(const char *a) {
    skip_sp(&a);
    char name[32] = "scanlist_"; int ni = 9;
    while (*a && *a != ' ' && ni < 30) { name[ni++] = *a++; } name[ni] = 0;

    uint16_t tmp[MAX_CHANNELS], tc = 0;
    while (*a && tc < MAX_CHANNELS) {
        int idx = parse_int(&a);
        if (idx < gChannelCount) tmp[tc++] = idx;
    }
    ScanListIdx sl; sl.count = tc; memcpy(sl.idx, tmp, tc * 2);
    Storage_Save(name, 0, &sl, sizeof(sl));
    OUTF("%s %u", name, tc);
}

static void cmd_SL_LS(const char *a) { (void)a;
    OUTF("active SL %u", gScanCount);
    for (uint16_t i = 0; i < gScanCount; i++) {
        uint16_t ci = gScanList[i];
        if (ci < gChannelCount)
            OUTF("  %u->%lu.%05lu", i, gChannels[ci].freq / 100000, gChannels[ci].freq % 100000);
    }
}

static void cmd_G(const char *a) { (void)a;
    if (gScanCount == 0) { OUT("NO SL\n"); return; }
    OUT("DONE\n");
}

static void cmd_SCAN_SL(const char *a) {
    skip_sp(&a);
    char name[32] = "scanlist_"; int ni = 9;
    while (*a && *a != ' ' && ni < 30) { name[ni++] = *a++; } name[ni] = 0;

    ScanListIdx sl; sl.count = 0;
    Storage_Load(name, 0, &sl, sizeof(sl));
    if (sl.count == 0) { OUTF("NO %s", name); return; }
    OUT("DONE\n");
}

static void cmd_LOOP(const char *a) {
    skip_sp(&a);
    char name[32] = "scanlist_"; int ni = 9;
    while (*a && *a != ' ' && ni < 30) { name[ni++] = *a++; } name[ni] = 0;

    ScanListIdx sl; sl.count = 0;
    Storage_Load(name, 0, &sl, sizeof(sl));
    if (sl.count == 0) { OUTF("NO %s", name); return; }
    while (1) {
        for (uint16_t si = 0; si < sl.count; si++) {
            uint16_t ci = sl.idx[si];
            if (ci < gChannelCount) { /* scan_entry placeholder */ }
        }
    }
}

static void cmd_BL_ADD(const char *a) {
    uint32_t a1 = parse_mhz(&a);
    const char *p = a; while (*p == ' ') p++;
    uint32_t a2 = 0;
    if (*p >= '0' && *p <= '9') a2 = parse_mhz(&a);
    if (gBlackCount < BL_MAX) {
        gBlack[gBlackCount].lo = a1; gBlack[gBlackCount].hi = (a2 > a1) ? a2 : 0;
        gBlackCount++;
    }
    OUT("OK\n");
}

static void cmd_BL_LS(const char *a) { (void)a;
    for (uint8_t i = 0; i < gBlackCount; i++) {
        if (gBlack[i].hi == 0)
            OUTF("  %lu.%05lu", gBlack[i].lo / 100000, gBlack[i].lo % 100000);
        else
            OUTF("  %lu.%05lu..%lu.%05lu", gBlack[i].lo / 100000, gBlack[i].lo % 100000, gBlack[i].hi / 100000, gBlack[i].hi % 100000);
    }
}

static void cmd_BL_WR(const char *a) { (void)a;
    uint16_t cnt = gBlackCount;
    Storage_Save("blacklist.sl", 0, &cnt, 2);
    if (cnt) Storage_Save("blacklist.sl", 1, gBlack, cnt * sizeof(BlackItem));
    OUT("OK\n");
}

static void cmd_BL_RD(const char *a) { (void)a;
    uint16_t cnt = 0;
    Storage_Load("blacklist.sl", 0, &cnt, 2);
    if (cnt > 0 && cnt <= BL_MAX) {
        Storage_Load("blacklist.sl", 1, gBlack, cnt * sizeof(BlackItem));
        gBlackCount = (uint8_t)cnt;
    } else gBlackCount = 0;
    OUTF("%u", gBlackCount);
}

static void cmd_LW_LS(const char *a) { (void)a;
    OUTF("total=%u", gLootTotal);
    for (uint8_t i = 0; i < gLootCount; i++)
        OUTF("  %u: %lu.%05lu %ddBm", i, gLoot[i].freq / 100000, gLoot[i].freq % 100000, Rssi2DBm(gLoot[i].rssi_peak));
}

static void cmd_LW_CLR(const char *a) { (void)a;
    gLootCount = 0; gLootTotal = 0;
    uint32_t z = 0; Storage_Save("loot.tmp", 0, &z, 4);
    OUT("OK\n");
}

static void cmd_PR_WR(const char *a) {
    int idx = parse_int(&a); uint32_t m = parse_mhz(&a);
    int gain = parse_int(&a); int sql = parse_int(&a); int mod = parse_int(&a);
    if (idx >= 0 && idx < MAX_RX_PROFILES) {
        RxProfile *r = &gRxProfiles[idx];
        r->freq_anchor = m; r->gain_index = (uint8_t)gain;
        r->squelch_level = (uint8_t)sql; r->modulation = (uint8_t)mod;
        r->bw = 4; r->agc_enable = 1;
    }
    OUT("OK\n");
}

static void cmd_PR_LS(const char *a) { (void)a;
    for (uint8_t i = 0; i < MAX_RX_PROFILES; i++) {
        char b[32]; int n = snprintf_(b, 32, "%u %u\n", (unsigned)i, (unsigned)gRxProfiles[i].freq_anchor);
        USB_CDC_Write((uint8_t*)b, n > 0 ? n : 0);
    }
}

static void cmd_K(const char *a) {
    uint32_t reg = parse_hex(&a);
    OUTF("R%02lX=0x%04X", reg, BK4819_ReadRegister((BK4819_REGISTER_t)reg));
}

static void cmd_K_WR(const char *a) {
    uint32_t reg = parse_hex(&a); skip_sp(&a);
    uint32_t val = parse_hex(&a);
    BK4819_WriteRegister((BK4819_REGISTER_t)reg, (uint16_t)val);
    OUTF("R%02lX=0x%04lX", reg, val);
}

static void cmd_LS(const char *a) { (void)a;
    lfs_dir_t dir; struct lfs_info info;
    if (lfs_dir_open(&gLfs, &dir, "/") == 0) {
        while (lfs_dir_read(&gLfs, &dir, &info) > 0) {
            if (info.type == LFS_TYPE_DIR) continue;
            OUTF("%s %lu", info.name, info.size);
        }
        lfs_dir_close(&gLfs, &dir);
    }
}

static uint8_t gFileBuf[256];
static void cmd_CAT(const char *a) {
    skip_sp(&a);
    lfs_file_t f;
    struct lfs_file_config cfg = { .buffer = gFileBuf, .attr_count = 0 };
    if (lfs_file_opencfg(&gLfs, &f, a, LFS_O_RDONLY, &cfg) < 0) { OUT("NOT FOUND\n"); return; }
    uint8_t buf[64]; int r;
    while ((r = lfs_file_read(&gLfs, &f, buf, 64)) > 0) USB_CDC_Write(buf, r);
    lfs_file_close(&gLfs, &f);
    OUT("\n");
}

static void cmd_RM(const char *a) {
    skip_sp(&a);
    if (lfs_remove(&gLfs, a) == 0) OUT("OK\n"); else OUT("ERR\n");
}

static void cmd_SCR(const char *a) { (void)a;
    USB_CDC_WriteString("P1\n128 64\n");
    char buf[260];
    for (uint8_t y = 0; y < LCD_HEIGHT; y++) {
        char *p = buf;
        for (uint8_t x = 0; x < LCD_WIDTH; x++) {
            if (x) *p++ = ' ';
            *p++ = GetPixel(x, y) ? '1' : '0';
        }
        *p++ = '\n';
        USB_CDC_Write((uint8_t*)buf, p - buf);
    }
}

static void cmd_SPECTRUM(const char *a) {
    uint32_t lo = parse_mhz(&a); uint32_t hi = parse_mhz(&a);
    uint32_t step = parse_khz(&a); if (step == 0) step = 2500;
    BK4819_Squelch(3, lo, 1, 1);
    USB_CDC_WriteString("freq,rssi,noise,glitch\n");
    char buf[40];
    for (uint32_t f = lo; f <= hi; f += step) {
        BK4819_TuneTo(f, 0); SYSTICK_DelayUs(8000);
        uint16_t rssi = BK4819_GetRSSI();
        uint8_t noise = BK4819_GetNoise();
        uint8_t glitch = BK4819_GetGlitch();
        int n = snprintf_(buf, 40, "%lu,%u,%u,%u\n", f, rssi, noise, glitch);
        if (n > 0) { USB_CDC_Write((uint8_t*)buf, n); USB_CDC_Poll(); }
    }
}

static const Cmd cmds[] = {
    {"SCAN_ADD", cmd_SCAN_ADD}, {"SCAN_SL", cmd_SCAN_SL},
    {"CH_ADD", cmd_CH_ADD}, {"CH_LS", cmd_CH_LS}, {"CH_RM", cmd_CH_RM},
    {"SL_ADD", cmd_SL_ADD}, {"SL_WR", cmd_SL_WR}, {"SL_LS", cmd_SL_LS},
    {"BL_ADD", cmd_BL_ADD}, {"BL_LS", cmd_BL_LS}, {"BL_WR", cmd_BL_WR}, {"BL_RD", cmd_BL_RD},
    {"LW_LS", cmd_LW_LS}, {"LW_CLR", cmd_LW_CLR},
    {"PR_WR", cmd_PR_WR}, {"PR_LS", cmd_PR_LS},
    {"LOOP", cmd_LOOP}, {"K_WR", cmd_K_WR},
    {"LS", cmd_LS}, {"CAT", cmd_CAT}, {"RM", cmd_RM},
    {"SCR", cmd_SCR}, {"SPECTRUM", cmd_SPECTRUM},
    {"G", cmd_G}, {"U", cmd_U}, {"K", cmd_K},
    {"REBOOT", cmd_REBOOT}, {"RESET", cmd_RESET},
    {"h", cmd_h}, {"?", cmd_h},
};

void CMD_Process(const char *line) {
    for (uint32_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        int len = strlen(cmds[i].name);
        if (len > 0 && memcmp(line, cmds[i].name, len) == 0 &&
            (line[len] == ' ' || line[len] == 0 || line[len] == '\r')) {
            cmds[i].fn(line + len);
            return;
        }
    }
    OUT("?\n");
}
