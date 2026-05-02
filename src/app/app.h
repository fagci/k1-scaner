#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum { APP_IDLE, APP_SCAN, APP_LISTEN } AppState;

typedef struct {
    uint8_t noise_threshold;  // 0..255, шум ниже = сигнал
    uint8_t scan_action;      // 0=skip, 1=WL auto, 2=BL auto, 3=listen
    uint16_t dwell_ms;
} ScanSettings;

void APP_Init(void);
void APP_Tick(void);
AppState APP_GetState(void);
void APP_Stop(void);
void APP_SetSettings(const ScanSettings *s);
ScanSettings *APP_GetSettings(void);

void APP_StartScan(uint32_t lo, uint32_t hi, uint32_t step);
void APP_StartScanList(const uint16_t *list, uint16_t count);
void APP_Listen(uint16_t ch_index);

extern uint32_t gLastFreq;
extern uint32_t gListenFreq;
extern bool gListenAudioOn;

#endif
