#ifndef DRIVER_HRTIME_H
#define DRIVER_HRTIME_H

/**
 * @file hrtime.h
 * @brief High-resolution timing using free-running TIM2 (48 MHz, 20.8 ns ticks).
 *
 * Provides microsecond-resolution timestamps for precise timing in
 * RSSI scanning and other time-critical operations.
 */

#include <stdint.h>

/**
 * Initialize TIM2 as a free-running 48 MHz counter.
 * Call once during system startup.
 */
void HRTIME_Init(void);

/**
 * Get current timestamp in 48 MHz ticks (20.8 ns per tick).
 * Rolls over every ~89 seconds (2^32 / 48000000).
 * Use `HRTIME_Delta()` for safe elapsed-time calculation across rollover.
 */
uint32_t HRTIME_Now(void);

/**
 * Calculate elapsed ticks between two timestamps, handling rollover.
 * @param start Previous timestamp from HRTIME_Now()
 * @return Elapsed ticks since start (correctly handles 32-bit wrap)
 */
static inline uint32_t HRTIME_Delta(uint32_t start) {
  return HRTIME_Now() - start;
}

/**
 * Check if elapsed ticks >= target, handling rollover.
 * @param start Previous timestamp
 * @param target Minimum elapsed ticks
 * @return true if elapsed >= target
 */
static inline bool HRTIME_Elapsed(uint32_t start, uint32_t target) {
  return (HRTIME_Now() - start) >= target;
}

/**
 * Busy-wait delay with microsecond precision.
 * @param us Microseconds to delay (uses 48 MHz TIM2 counter)
 */
void HRTIME_DelayUs(uint32_t us);

/**
 * Convert microseconds to TIM2 ticks (48 MHz = 48 ticks per us).
 */
static inline uint32_t HRTIME_UsToTicks(uint32_t us) {
  return us * 48u;
}

/**
 * Convert TIM2 ticks to microseconds.
 */
static inline uint32_t HRTIME_TicksToUs(uint32_t ticks) {
  return ticks / 48u;
}

#endif // DRIVER_HRTIME_H
