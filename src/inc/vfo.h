#ifndef VFO_H
#define VFO_H

#include "../driver/bk4829.h"
#include "common.h"

#define MAX_VFOS 4

typedef struct {
  uint16_t scanlists;
  uint16_t channel;
  char name[10];
  uint32_t rxF : 27;
  int32_t ppm : 5;
  uint32_t txF : 27;
  OffsetDirection offsetDir : 2;
  bool allowTx : 1;
  uint8_t reserved2 : 2;
  Step step : 4;
  uint8_t modulation : 4;
  uint8_t bw : 4;
  Radio radio : 2;
  TXOutputPower power : 2;
  uint8_t mic : 4; // Mic gain (0-15)
  uint8_t scrambler : 4;
  Squelch squelch;
  CodeRXTX code;
  bool fixedBoundsMode : 1;
  bool isChMode : 1;
  uint8_t gainIndex : 5;
  uint8_t deviation; // Deviation setting (0-255, stored as value*10 when used)
  uint32_t upconverter; // Upconverter frequency shift (full 32-bit)
} __attribute__((packed)) VFO;

// Параметры
typedef enum {
  PARAM_RADIO,
  PARAM_PRECISE_F_CHANGE,
  PARAM_STEP,
  PARAM_POWER,
  PARAM_SQUELCH_TYPE,
  PARAM_SQUELCH_VALUE,
  PARAM_GAIN,
  PARAM_VOLUME,
  PARAM_BANDWIDTH,

  PARAM_TX_OFFSET,
  PARAM_TX_OFFSET_DIR,
  PARAM_TX_STATE,
  PARAM_TX_FREQUENCY,
  PARAM_TX_FREQUENCY_FACT,
  PARAM_TX_POWER,
  PARAM_TX_POWER_AMPLIFIER,

  PARAM_RX_CODE,
  PARAM_TX_CODE,

  PARAM_AFC,
  PARAM_AFC_SPD,
  PARAM_AF_RX_300,
  PARAM_AF_RX_3K,
  PARAM_AF_TX_300,
  PARAM_AF_TX_3K,
  PARAM_DEV,
  PARAM_MIC,
  PARAM_AGC,
  PARAM_XTAL,
  PARAM_SCRAMBLER,
  PARAM_FILTER,
  PARAM_UPCONVERTER,

  PARAM_RSSI,
  PARAM_NOISE,
  PARAM_GLITCH,
  PARAM_SNR,

  PARAM_MODULATION,

  // IMPORTANT FOR SI47xx (know modulation before f set)
  PARAM_FREQUENCY_FACT,
  PARAM_FREQUENCY,

  PARAM_COUNT,
} ParamType;

typedef enum {
  TX_UNKNOWN,
  TX_ON,
  TX_VOL_HIGH,
  TX_BAT_LOW,
  TX_DISABLED,
  TX_DISABLED_UPCONVERTER,
  TX_POW_OVERDRIVE,
} TXStatus;

typedef enum {
  RADIO_SCAN_STATE_IDLE,      // Сканирование не активно
  RADIO_SCAN_STATE_SWITCHING, // Сканирование не активно
  RADIO_SCAN_STATE_WARMUP, // Ожидание стабилизации после переключения
  RADIO_SCAN_STATE_MEASURING, // Выполнение замера
  RADIO_SCAN_STATE_DECISION // Принятие решения о переключении
} RadioScanState;
/* ── Уровни питания приёмника ───────────────────────────────────────────────
 */

typedef enum {
  /** Чип не инициализирован / полностью обесточен. */
  RXPWR_OFF = 0,

  /**
   * BK4819: REG_37 power-down (≈3 мА экономии vs IDLE).
   * BK1080/SI4732: не используется (у них нет soft power-down).
   */
  RXPWR_SLEEP,

  /**
   * Чип инициализирован, частота настроена, но DSP остановлен (REG30=0).
   * Используется, когда другой приёмник активен, но этот может
   * понадобиться в ближайшее время (scan, dual-watch).
   */
  RXPWR_IDLE,

  /**
   * Чип полностью активен: принимает сигнал, возможно воспроизведение.
   */
  RXPWR_ACTIVE,
} ReceiverPowerState;

/* ── Состояние всего коммутатора ─────────────────────────────────────────── */

typedef struct {
  ReceiverPowerState bk4819;
  ReceiverPowerState bk1080;
  ReceiverPowerState si4732;

  /** Какой чип сейчас подаёт звук на динамик (RADIO_BK4819 / RADIO_BK1080 /
   *  RADIO_SI4732 / 0xFF = никто). */
  uint8_t audio_source; /* Radio или 0xFF */
} RadioSwitchCtx;

// Диапазон частот и его параметры
typedef struct {
  uint32_t min_freq;
  uint32_t max_freq;
  uint16_t available_bandwidths[10]; // Доступные полосы (кГц)
  uint8_t available_mods[6];         // Доступные модуляции
  uint8_t num_available_mods;
  uint8_t num_available_bandwidths;
} FreqBand;

// Контекст VFO
typedef struct {

  struct {
    uint32_t frequency : 27; // Частота передачи (может отличаться от RX)
    TXStatus last_error : 3;
    uint8_t power_level; // Уровень мощности
    bool dirty : 1;      // Флаг изменения параметров TX
    bool is_active : 1;  // true, если идёт передача
    OffsetDirection offsetDirection : 4;
    bool pa_enabled : 1;
    Code code;
  } __attribute__((packed)) tx_state;

  char name[10];
  bool dirty[PARAM_COUNT]; // Флаги изменений

  const FreqBand *current_band; // Активный диапазон
  uint32_t last_save_time; // Время последнего сохранения
  uint32_t frequency : 27;   // Текущая частота
  uint32_t upconverter : 27; // Upconverter frequency shift
  uint16_t dev;
  uint8_t volume; // Громкость
  uint8_t afc;
  uint8_t afc_speed;
  int8_t af_rx_300; // RX 300Hz AF response gain (-4..+4 dB, 0=0dB)
  int8_t af_rx_3k;  // RX 3kHz AF response gain (-4..+4 dB, 0=0dB)
  int8_t af_tx_300; // TX 300Hz AF response gain (-4..+4 dB, 0=0dB)
  int8_t af_tx_3k;  // TX 3kHz AF response gain (-4..+4 dB, 0=0dB)
  uint8_t scrambler;
  Squelch squelch;
  Code code;
  Step step : 5;
  ModulationType modulation : 3; // Текущая модуляция
  uint8_t bandwidth : 4;         // Полоса пропускания
  uint8_t bandwidth_index : 4;
  uint8_t mic : 4;
  uint8_t agc;
  Radio radio_type : 4;
  uint8_t gain : 5;
  uint8_t modulation_index : 3;
  XtalMode xtal : 2;
  Filter filter : 2;
  TXOutputPower power : 2;
  bool preciseFChange : 1;
  bool fixed_bounds : 1;

  bool save_to_eeprom; // Флаг необходимости сохранения в EEPROM
} __attribute__((packed)) VFOContext;

// Channel/VFO mode
typedef enum { MODE_VFO, MODE_CHANNEL } VFOMode;

// Extended VFO context
typedef struct {
  uint32_t last_activity_time; // for multiwatch
  uint16_t channel_index;      // Channel index if in channel mode
  uint16_t vfo_ch_index;       // MR index of VFO
  Measurement msm;
  VFOContext context; // Existing VFO context
  VFOMode mode;       // VFO or channel mode
  bool is_active;     // Whether this is the active VFO
  bool is_open;
} ExtendedVFOContext;

typedef struct {
  bool bk4819_enabled;
  bool bk1080_enabled;
  bool si4732_enabled;
} RadioHardwareState;

// Global radio state
typedef struct {
  ExtendedVFOContext vfos[MAX_VFOS]; // Array of VFOs
  RadioScanState scan_state; // Состояние сканирования
  RadioHardwareState hw_state;
  RadioSwitchCtx rx_switch;
  uint32_t last_scan_time;   // Last scan time
  uint8_t num_vfos;          // Number of configured VFOs
  uint8_t active_vfo_index;  // Currently active VFO
  uint8_t primary_vfo_index; //
  uint8_t last_active_vfo; // Последний активный VFO с активностью
  bool audio_routing_enabled; // Флаг управления аудио маршрутизацией
  bool multiwatch_enabled; // Whether multiwatch is enabled
} RadioState;

#endif // !VFO
