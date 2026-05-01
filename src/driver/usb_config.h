#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <stddef.h>

#define CONFIG_USB_PRINTF(...)

extern void *usb_malloc(size_t size);
extern void usb_free(void *ptr);

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_ERROR
#endif

#define CONFIG_USB_PRINTF_COLOR_ENABLE

#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256

#include "py32f0xx.h"

#define USBD_IRQn       USB_IRQn
#define USBD_IRQHandler USB_IRQHandler

typedef struct {
    uint8_t *buf;
    const uint32_t size;
    volatile uint32_t *write_pointer;
} cdc_acm_rx_buf_t;

void cdc_acm_init(cdc_acm_rx_buf_t rx_buf);
void cdc_acm_data_send_with_dtr(const uint8_t *buf, uint32_t size);
void cdc_acm_data_send_with_dtr_async(const uint8_t *buf, uint32_t size);

#endif
