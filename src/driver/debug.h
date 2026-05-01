#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

void DBG_Init(void);
void DBG_Print(const char *fmt, ...);
void DBG_Write(const uint8_t *data, uint32_t len);
uint32_t DBG_GetUptimeMs(void);

// HardFault
void DBG_HardFaultDump(uint32_t *sp);

// Memory
uint32_t DBG_GetStackWatermark(void);
uint32_t DBG_GetFreeHeap(void);

#endif
