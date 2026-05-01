#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stdbool.h>

void USB_CDC_Init(void);
void USB_CDC_Poll(void);
bool USB_CDC_IsReady(void);
void USB_CDC_Write(const uint8_t *data, uint32_t len);
void USB_CDC_WriteString(const char *str);
uint32_t USB_CDC_Read(uint8_t *buf, uint32_t maxlen);

#endif
