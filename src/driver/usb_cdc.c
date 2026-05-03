#include "usb_cdc.h"
#include "usb_config.h"
#include "../external/CherryUSB/core/usbd_core.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_bus.h"
#include "../external/PY32F071_HAL_Driver/Inc/py32f071_ll_rcc.h"
#include "usbd_cdc_if.h"
#include <string.h>

static bool connected;

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
void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr) { (void)intf; connected = dtr; }

void USB_CDC_Poll(void) {}

static void send_sync(const uint8_t *data, uint32_t len) {
    if (!connected || !len) return;
    ep_tx_busy = true;
    usbd_ep_start_write(CDC_IN_EP, data, len);
    uint32_t timeout = 50000;
    while (ep_tx_busy && --timeout);
    if (!timeout) ep_tx_busy = false;
}

void USB_CDC_Write(const uint8_t *data, uint32_t len) {
    while (len > 0) {
        uint32_t chunk = len > 64 ? 64 : len;
        send_sync(data, chunk);
        data += chunk;
        len -= chunk;
    }
}

void USB_CDC_WriteString(const char *str) {
    USB_CDC_Write((const uint8_t*)str, strlen(str));
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
