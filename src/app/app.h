#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum { APP_IDLE, APP_SCAN, APP_LISTEN } AppState;

typedef struct {
    uint8_t noise_threshold;
    uint8_t scan_action;  // 0=skip, 1=WL auto, 2=BL auto, 3=listen
    uint16_t dwell_ms;
} ScanSettings;

void APP_Init(void);
void APP_Tick(void);
AppState APP_GetState(void);
void APP_Stop(void);
void APP_SetSettings(const ScanSettings *s);
ScanSettings *APP_GetSettings(void);

// Запуск полного сценария: все диапазоны + SL + BL-исключения
void APP_StartFullScan(void);

void APP_Listen(uint16_t ch_index);

// Текущая частота сканирования (для UI)
extern uint32_t gScanCurrentFreq;
extern uint32_t gScanProgress;  // 0..1000

#endif
