#ifndef USBD_CDC_IF_H
#define USBD_CDC_IF_H

#include "usb_config.h"

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02

extern uint8_t cdc_rx_buf[128];
extern volatile uint32_t cdc_rx_wp;
extern volatile bool ep_tx_busy;

#endif
