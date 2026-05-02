#ifndef UI_H
#define UI_H

#include "../driver/keyboard.h"
#include <stdint.h>
#include <stdbool.h>

void UI_Init(void);
void UI_Draw(void);
void UI_DrawScanProgress(uint32_t freq);
void UI_HandleKey(KEY_Code_t key, KEY_State_t state);
void ui_draw_scan_progress(uint32_t freq); // для app.c

#endif
