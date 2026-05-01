#include "debug.h"
#include "usb_cdc.h"
#include "systick.h"
#include "../external/printf/printf.h"
#include <stdarg.h>

// ── timestamp ───────────────────────────────────────────────────────────
void DBG_Init(void) {}

uint32_t DBG_GetUptimeMs(void) { return Now(); }

// ── форматированный вывод ──────────────────────────────────────────────
static char out_buf[128];

void DBG_Print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    uint32_t len = vsnprintf(out_buf, sizeof(out_buf), fmt, ap);
    va_end(ap);
    if (len > sizeof(out_buf)) len = sizeof(out_buf) - 1;
    USB_CDC_Write((uint8_t *)out_buf, len);
}

void DBG_Write(const uint8_t *data, uint32_t len) {
    USB_CDC_Write(data, len);
}

// ── HardFault ──────────────────────────────────────────────────────────
// вызывается из HardFault_Handler, sp = MSP после сбоя
void DBG_HardFaultDump(uint32_t *sp) {
    USB_CDC_WriteString("\n=== HARD FAULT ===\n");

    char b[64];
    // stacked registers (Cortex-M0+: r0-r3, r12, lr, pc, psr)
    uint32_t r0  = sp[0];
    uint32_t r1  = sp[1];
    uint32_t r2  = sp[2];
    uint32_t r3  = sp[3];
    uint32_t r12 = sp[4];
    uint32_t lr  = sp[5];
    uint32_t pc  = sp[6];
    uint32_t psr = sp[7];

    snprintf(b, sizeof(b), "R0: 0x%08lX  R1: 0x%08lX\n", r0, r1);
    DBG_Write((uint8_t *)b, strlen(b));
    snprintf(b, sizeof(b), "R2: 0x%08lX  R3: 0x%08lX\n", r2, r3);
    DBG_Write((uint8_t *)b, strlen(b));
    snprintf(b, sizeof(b), "R12:0x%08lX  LR: 0x%08lX\n", r12, lr);
    DBG_Write((uint8_t *)b, strlen(b));
    snprintf(b, sizeof(b), "PC: 0x%08lX  PSR:0x%08lX\n", pc, psr);
    DBG_Write((uint8_t *)b, strlen(b));
    snprintf(b, sizeof(b), "SP: 0x%08lX\n", (uint32_t)sp);
    DBG_Write((uint8_t *)b, strlen(b));

    // дамп стека вокруг PC
    uint32_t *p = (uint32_t *)(pc & ~3);
    snprintf(b, sizeof(b), "\nCode around PC:\n");
    DBG_Write((uint8_t *)b, strlen(b));
    for (int i = -4; i <= 4; i++) {
        uint32_t v;
        __asm volatile("" : : : "memory");
        v = *(volatile uint32_t *)((uint8_t *)p + i * 4);
        snprintf(b, sizeof(b), "  %c0x%08lX: 0x%08lX\n",
                 i == 0 ? '>' : ' ', (uint32_t)((uint8_t *)p + i * 4), v);
        DBG_Write((uint8_t *)b, strlen(b));
    }

    // watermark стека
    extern uint32_t _ebss;
    uint32_t stack_bottom = (uint32_t)&_ebss + 0x400; // ~1KB for stack
    if ((uint32_t)sp < stack_bottom) {
        DBG_Write((uint8_t *)"STACK LOW!\n", 11);
    }
    DBG_Write((uint8_t *)"=== END ===\n", 12);
}

// ── память ─────────────────────────────────────────────────────────────
extern uint32_t _ebss;
extern uint32_t _estack;

uint32_t DBG_GetStackWatermark(void) {
    // ищем последний ненулевой байт от ebss до estack
    uint32_t *p = (uint32_t *)&_ebss;
    uint32_t *end = (uint32_t *)&_estack;
    while (p < end && *p == 0) p++;
    return (uint32_t)(end - p) * 4;
}

uint32_t DBG_GetFreeHeap(void) {
    return 0; // no malloc
}
