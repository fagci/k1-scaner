#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef struct {
  uint8_t value;
  uint8_t type : 4;
} Code;

typedef struct {
  Code rx;
  Code tx;
} CodeRXTX;

typedef struct {
  uint8_t s;
  uint8_t m;
  uint8_t e;
} PowerCalibration;

typedef struct {
  uint8_t value : 4;
  uint8_t type : 2;
} Squelch;

typedef enum {
  OFFSET_NONE,
  OFFSET_PLUS,
  OFFSET_MINUS,
  OFFSET_FREQ,
} OffsetDirection;

typedef enum {
  TX_POW_ULOW,
  TX_POW_LOW,
  TX_POW_MID,
  TX_POW_HIGH,
} TXOutputPower;

typedef enum {
  RADIO_BK4819,
  RADIO_BK1080,
  RADIO_SI4732,
} Radio;

typedef enum {
  STEP_0_02kHz,
  STEP_0_05kHz,
  STEP_0_5kHz,
  STEP_1_0kHz,

  STEP_2_5kHz,
  STEP_5_0kHz,
  STEP_6_25kHz,
  STEP_8_33kHz,
  STEP_9_0kHz,
  STEP_10_0kHz,
  STEP_12_5kHz,
  STEP_25_0kHz,
  STEP_50_0kHz,
  STEP_100_0kHz,
  STEP_500_0kHz,

  STEP_COUNT,
} Step;

typedef struct {
  uint32_t f;
  uint32_t lastTimeOpen;
  uint16_t duration;
  uint16_t timeUs;
  uint16_t rssi;
  uint8_t noise;
  uint8_t glitch;
  uint8_t snr;
  uint8_t code;
  bool isCd : 1;
  bool open : 1;
  bool blacklist : 1;
  bool whitelist : 1;
} Measurement;

typedef struct {
  uint32_t s;
  uint32_t e;
  PowerCalibration c;
} PCal;

#endif // !COMMON_H
