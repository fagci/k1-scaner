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

    // пытаемся дождаться USB
    for (volatile int i = 0; i < 500000; i++);

    DBG_HardFaultDump(sp);

    while (1) {
        SYSTICK_DelayMs(300);
        GPIO_TogglePin(GPIO_PIN_FLASHLIGHT);
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
