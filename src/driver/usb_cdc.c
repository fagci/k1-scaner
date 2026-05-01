#include "usb_cdc.h"
#include "usb_config.h"
#include "../external/CherryUSB/core/usbd_core.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_bus.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_rcc.h"
#include "usbd_cdc_if.h"
#include <string.h>

#define TX_BUF_SIZE 128

static bool connected;
static uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint16_t tx_head, tx_tail;

void USB_CDC_Init(void) {
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USBD);

    cdc_acm_rx_buf_t rx = { .buf = cdc_rx_buf, .size = sizeof(cdc_rx_buf), .write_pointer = &cdc_rx_wp };
    cdc_acm_init(rx);

    NVIC_SetPriority(USBD_IRQn, 3);
    NVIC_EnableIRQ(USBD_IRQn);
}

bool USB_CDC_IsReady(void) { return connected; }

void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr) {
    (void)intf;
    connected = dtr;
}

void USB_CDC_Poll(void) {
    if (!connected || ep_tx_busy) return;
    uint16_t h = tx_head, t = tx_tail;
    uint16_t avail = (h >= t) ? (h - t) : (TX_BUF_SIZE - t + h);
    if (avail == 0) return;
    uint16_t chunk = avail < 64 ? avail : 64;
    uint8_t tmp[64];
    for (uint16_t i = 0; i < chunk; i++) {
        tmp[i] = tx_buf[t];
        if (++t >= TX_BUF_SIZE) t = 0;
    }
    tx_tail = t;
    ep_tx_busy = true;
    usbd_ep_start_write(CDC_IN_EP, tmp, chunk);
}

void USB_CDC_Write(const uint8_t *data, uint32_t len) {
    if (!connected || !len) return;
    for (uint32_t i = 0; i < len; i++) {
        uint16_t next = (tx_head + 1) % TX_BUF_SIZE;
        if (next == tx_tail) break;  // buffer full, drop
        tx_buf[tx_head] = data[i];
        tx_head = next;
    }
}

void USB_CDC_WriteString(const char *str) {
    USB_CDC_Write((const uint8_t *)str, strlen(str));
}

uint32_t USB_CDC_Read(uint8_t *buf, uint32_t maxlen) {
    uint32_t wp = cdc_rx_wp;
    if (wp == 0) return 0;
    uint32_t cnt = wp < maxlen ? wp : maxlen;
    memcpy(buf, cdc_rx_buf, cnt);
    uint32_t rem = wp - cnt;
    if (rem) memmove(cdc_rx_buf, cdc_rx_buf + cnt, rem);
    cdc_rx_wp = rem;
    return cnt;
}
