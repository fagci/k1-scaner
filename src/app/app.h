#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum { APP_IDLE, APP_SCAN, APP_LISTEN } AppState;

void APP_Init(void);
void APP_Tick(void);
AppState APP_GetState(void);
void APP_Stop(void);

void APP_StartScan(uint32_t lo, uint32_t hi, uint32_t step);
void APP_StartScanList(const uint16_t *list, uint16_t count);
void APP_Listen(uint16_t ch_index);

#endif
