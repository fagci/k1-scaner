#ifndef DRIVER_PY25Q16_H
#define DRIVER_PY25Q16_H

#include <stdbool.h>
#include <stdint.h>

extern bool gEepromWrite;

void PY25Q16_Init();
void PY25Q16_ReadBuffer(uint32_t Address, void *pBuffer, uint32_t Size);
void PY25Q16_WriteBuffer(uint32_t Address, const void *pBuffer, uint32_t Size,
                         bool Append);
void PY25Q16_SectorErase(uint32_t Address);
void PY25Q16_FullErase();

static uint8_t PY25Q16_ReadStatus(void);
static void PY25Q16_WaitBusy(void);

void test_flash_basic(void);
bool test_flash_simple(void);

#endif
