#include "../external/CherryUSB/core/usbd_core.h"
#include "../external/CherryUSB/class/cdc/usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usb_config.h"

#define CDC_INT_EP 0x83

#define USBD_VID       0x36b7
#define USBD_PID       0xFFFF
#define USBD_MAX_POWER 100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

USB_MEM_ALIGNX uint8_t cdc_usb_buf[128];
USB_MEM_ALIGNX uint8_t cdc_rx_buf[128];
volatile uint32_t cdc_rx_wp;

static cdc_acm_rx_buf_t client_rx_buf = {0};
volatile bool ep_tx_busy;

static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, 0x02),
    USB_LANGID_INIT(USBD_LANGID_STRING),
    0x0A, USB_DESCRIPTOR_TYPE_STRING, 'k',0, '1',0, '-',0, 's',0, 'c',0,
    0x1C, USB_DESCRIPTOR_TYPE_STRING, 'k',0, '1',0, '-',0, 's',0, 'c',0, 'a',0, 'n',0, ' ',0, 'C',0, 'D',0, 'C',0,
    0x16, USB_DESCRIPTOR_TYPE_STRING, '0',0, '0',0, '0',0, '1',0,
    0x00
};

static void cdc_out_cb(uint8_t ep, uint32_t nbytes) {
    (void)ep;
    cdc_acm_rx_buf_t *rx = &client_rx_buf;
    if (nbytes && rx->buf) {
        uint32_t wp = *rx->write_pointer;
        for (uint32_t i = 0; i < nbytes; i++) {
            if (wp >= rx->size) wp = 0;
            rx->buf[wp++] = cdc_usb_buf[i];
        }
        *rx->write_pointer = wp;
    }
    usbd_ep_start_read(CDC_OUT_EP, cdc_usb_buf, sizeof(cdc_usb_buf));
}

static void cdc_in_cb(uint8_t ep, uint32_t nbytes) {
    (void)ep;
    if ((nbytes % 64) == 0 && nbytes)
        usbd_ep_start_write(CDC_IN_EP, NULL, 0);
    else
        ep_tx_busy = false;
}

void usbd_configure_done_callback(void) {
    usbd_ep_start_read(CDC_OUT_EP, cdc_usb_buf, sizeof(cdc_usb_buf));
}

static struct usbd_endpoint cdc_out_ep = { CDC_OUT_EP, cdc_out_cb };
static struct usbd_endpoint cdc_in_ep  = { CDC_IN_EP,  cdc_in_cb };
static struct usbd_interface intf0, intf1;

void cdc_acm_init(cdc_acm_rx_buf_t rx_buf) {
    memcpy(&client_rx_buf, &rx_buf, sizeof(cdc_acm_rx_buf_t));
    *client_rx_buf.write_pointer = 0;

    usbd_desc_register(cdc_descriptor);
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf0));
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf1));
    usbd_add_endpoint(&cdc_out_ep);
    usbd_add_endpoint(&cdc_in_ep);
    usbd_initialize();
}
