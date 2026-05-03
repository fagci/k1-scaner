#include <string.h>
#include "driver/backlight.h"
#include "driver/debug.h"
#include "driver/gpio.h"
#include "driver/hrtime.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/systick.h"
#include "driver/usb_cdc.h"
#include "driver/bk4829.h"
#include "driver/lfs.h"
#include "helper/storage.h"
#include "inc/radio_types.h"
#include "ui/graphics.h"
#include "board.h"
#include "app/app.h"
#include "app/ui.h"
#include "app/cmd.h"

LootItem gLoot[LOOT_MAX];
uint8_t gLootCount;
uint32_t gLootTotal;
ScanEntry gChannels[MAX_CHANNELS];
uint16_t gChannelCount;
uint16_t gScanList[MAX_CHANNELS];
uint16_t gScanCount;
RxProfile gRxProfiles[MAX_RX_PROFILES];
BlackItem gBlack[BL_MAX];
uint8_t gBlackCount;

void loot_tmp_save_raw(uint32_t total, uint32_t idx, const LootItem *l) {
    Storage_Save("loot.tmp", 0, &total, 4);
    if (idx > 0) Storage_Save("loot.tmp", 1 + idx, (void*)l, sizeof(LootItem));
}

void ch_save(void) {
    ChannelsFile cf; cf.count = gChannelCount;
    memcpy(cf.e, gChannels, gChannelCount * sizeof(ScanEntry));
    Storage_Save("channels.ch", 0, &cf, sizeof(cf));
}

int main(void) {
    SYSTICK_Init(); HRTIME_Init(); BOARD_Init();
    USB_CDC_Init(); DBG_Init();
    BACKLIGHT_SetBrightness(8); GPIO_TurnOnBacklight();
    ST7565_Init(); ST7565_SetContrast(8); ST7565_FillScreen(0); ST7565_Blit();
    keyboard_init(UI_HandleKey);
    BK4819_Init(); BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, 1);
    BK4819_RX_TurnOn(); BK4819_SetAFC(0); BK4819_SetAGC(1, 1);
    BK4819_SetFilterBandwidth(BK4819_FILTER_BW_12k); BK4819_SelectFilterEx(FILTER_UHF);
    PY25Q16_Init(); fs_init();

    ChannelsFile cf;
    if (Storage_Load("channels.ch", 0, &cf, sizeof(cf)) && cf.count <= MAX_CHANNELS)
        { gChannelCount = cf.count; memcpy(gChannels, cf.e, gChannelCount * sizeof(ScanEntry)); }
    ScanListIdx sl;
    if (Storage_Load("scanlist.sl", 0, &sl, sizeof(sl)) && sl.count <= MAX_CHANNELS)
        { gScanCount = sl.count; memcpy(gScanList, sl.idx, gScanCount * 2); }
    if (Storage_Load("blacklist.sl", 0, gBlack, sizeof(gBlack)))
        gBlackCount = sizeof(gBlack) / sizeof(BlackItem);

    APP_Init(); UI_Init();

    char line[80]; uint8_t lpos = 0;

    for (;;) {
        USB_CDC_Poll();
        keyboard_tick_1ms();

        uint8_t buf[64]; uint32_t n = USB_CDC_Read(buf, 64);
        for (uint32_t i = 0; i < n; i++) {
            char c = buf[i];
            if (c == '\r' || c == '\n') {
                if (lpos == 0) continue;
                line[lpos] = 0; lpos = 0;
                CMD_Process(line);
            } else if (lpos < sizeof(line) - 1) {
                line[lpos++] = c;
            }
        }

        APP_Tick();
        UI_Draw();
        __WFI();
    }
}
