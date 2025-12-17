/*
 * aff.h - Automatic Frequency Following for Phoenix SDR Controller
 * 
 * Monitors sync timing delta to detect frequency drift and applies
 * small corrections to keep the receiver locked to the carrier.
 * 
 * Algorithm:
 * - Collects delta_ms samples from SYNC telemetry
 * - Computes rolling average drift in Hz
 * - After settling interval, if |drift| >= threshold:
 *   - Applies ±1 Hz adjustment (never more)
 *   - Resets interval timer
 */

#ifndef AFF_H
#define AFF_H

#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * Constants
 *============================================================================*/

/* Interval options in seconds */
#define AFF_INTERVAL_30S    0
#define AFF_INTERVAL_45S    1
#define AFF_INTERVAL_60S    2
#define AFF_INTERVAL_90S    3
#define AFF_INTERVAL_120S   4
#define AFF_INTERVAL_COUNT  5

/* Algorithm parameters */
#define AFF_THRESHOLD_HZ    0.5f    /* Minimum drift to trigger adjustment */
#define AFF_MAX_ADJUST_HZ   1       /* Maximum adjustment per cycle (Hz) */
#define AFF_SAMPLE_COUNT    10      /* Rolling window size */

/*============================================================================
 * Types
 *============================================================================*/

typedef struct aff_state aff_state_t;

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * Create AFF state
 * @return Allocated state or NULL on failure
 */
aff_state_t* aff_create(void);

/**
 * Destroy AFF state
 */
void aff_destroy(aff_state_t* aff);

/**
 * Enable/disable AFF
 */
void aff_set_enabled(aff_state_t* aff, bool enabled);

/**
 * Check if AFF is enabled
 */
bool aff_is_enabled(aff_state_t* aff);

/**
 * Set adjustment interval (AFF_INTERVAL_xxx)
 */
void aff_set_interval(aff_state_t* aff, int interval_index);

/**
 * Get current interval index
 */
int aff_get_interval(aff_state_t* aff);

/**
 * Get interval in seconds for a given index
 */
int aff_interval_seconds(int interval_index);

/**
 * Get interval as display string
 */
const char* aff_interval_string(int interval_index);

/**
 * Update AFF with new sync data
 * Call this when SYNC telemetry arrives
 * 
 * @param aff           AFF state
 * @param delta_ms      Timing delta from SYNC packet
 * @param carrier_hz    Current carrier frequency
 * @param is_locked     true if sync state is LOCKED
 */
void aff_update(aff_state_t* aff, float delta_ms, int64_t carrier_hz, bool is_locked);

/**
 * Check if an adjustment is ready
 * Call periodically (e.g., in main loop)
 * 
 * @param aff           AFF state
 * @param adjustment_hz Output: Hz to add to frequency (+1, -1, or 0)
 * @return              true if adjustment should be applied
 */
bool aff_get_adjustment(aff_state_t* aff, int* adjustment_hz);

/**
 * Get current measured drift in Hz (for display)
 * This is the rolling average, updated continuously
 */
float aff_get_drift_hz(aff_state_t* aff);

/**
 * Reset AFF state (call when user manually changes frequency)
 */
void aff_reset(aff_state_t* aff);

#endif /* AFF_H */
