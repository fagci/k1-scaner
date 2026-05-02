#include "misc.h"
#include "driver/debug.h"
#include "driver/gpio.h"
#include "driver/systick.h"
#include "driver/usb_cdc.h"
#include "external/printf/printf.h"

char IsPrintable(char ch) { return (ch < 32 || 126 < ch) ? ' ' : ch; }

unsigned int SQRT16(unsigned int value) {
    unsigned int shift = 16;
    unsigned int bit = 1u << --shift;
    unsigned int sqrti = 0;
    while (bit) {
        const unsigned int temp = ((sqrti << 1) | bit) << shift--;
        if (value >= temp) { value -= temp; sqrti |= bit; }
        bit >>= 1;
    }
    return sqrti;
}

void _putchar(char c) { USB_CDC_Write((uint8_t *)&c, 1); }
void _init() {}

void HardFault_Handler(void) {
    uint32_t stacked_sp;
    __asm volatile("mrs %0, msp" : "=r"(stacked_sp));
    uint32_t *sp = (uint32_t *)stacked_sp;
    uint32_t pc = sp[6];

    // инициализируем LCD напрямую, если не инициализирован
    *((volatile uint32_t *)0x40021018) |= (1 << 18); // RCC APB2 SPI1EN
    *((volatile uint32_t *)0x4002100C) |= (1 << 28); // RCC IOPCEN
    *((volatile uint32_t *)0x40021000) |= (1 << 20); // RCC IOPAEN
    *((volatile uint32_t *)0x40021004) |= (1 << 23); // RCC IOPBEN

    // выводим PC на экран через графику (упрощённо — просто моргаем)
    // моргаем PC кодом: длинная пауза = старшие биты, короткая = младшие
    for (int r = 0; r < 3; r++) {
        uint16_t mask = (pc >> 16) >> (r * 8);
        for (int b = 0; b < 8; b++) {
            GPIO_TogglePin(GPIO_PIN_FLASHLIGHT);
            if (mask & (1 << (7 - b))) {
                SYSTICK_DelayMs(400);  // 1
            } else {
                SYSTICK_DelayMs(100);  // 0
            }
            GPIO_TogglePin(GPIO_PIN_FLASHLIGHT);
            SYSTICK_DelayMs(200);
        }
        SYSTICK_DelayMs(1000); // разделитель байтов
    }

    while (1) {
        GPIO_TogglePin(GPIO_PIN_FLASHLIGHT);
        SYSTICK_DelayMs(500);
    }
}

void mhzToS(char *buf, uint32_t f) {
    sprintf(buf, "%u.%05u", f / MHZ, f % MHZ);
}

// syscall stubs
void _exit(int s) { (void)s; while(1); }
int _kill(int p, int s) { (void)p; (void)s; return -1; }
int _getpid(void) { return 1; }
int _close(int f) { (void)f; return -1; }
int _fstat(int f, void *s) { (void)f; (void)s; return 0; }
int _isatty(int f) { (void)f; return 1; }
int _lseek(int f, int p, int d) { (void)f; (void)p; (void)d; return -1; }
int _read(int f, char *p, int l) { (void)f; (void)p; (void)l; return 0; }
int _write(int f, char *p, int l) { (void)f; (void)p; (void)l; USB_CDC_Write((uint8_t *)p, l); return l; }
void *_sbrk(int incr) {
    extern uint32_t _ebss;
    static unsigned char *heap = (unsigned char *)&_ebss;
    unsigned char *prev = heap;
    heap += incr;
    return prev;
}
