/*
 * aff.c - Automatic Frequency Following Implementation
 * 
 * Monitors sync timing delta and applies conservative frequency corrections.
 */

#include "aff.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*============================================================================
 * Internal Types
 *============================================================================*/

struct aff_state {
    bool enabled;
    int interval_index;
    
    /* Rolling sample window */
    float samples[AFF_SAMPLE_COUNT];
    int sample_head;
    int sample_count;
    
    /* Current carrier frequency */
    int64_t carrier_hz;
    
    /* Timing */
    uint32_t last_update_ms;
    uint32_t interval_start_ms;
    bool interval_elapsed;
    
    /* Calculated values */
    float mean_delta_ms;
    float drift_hz;
    
    /* Pending adjustment */
    bool adjustment_ready;
    int adjustment_hz;
};

/* Interval values in seconds */
static const int interval_values[AFF_INTERVAL_COUNT] = {
    30, 45, 60, 90, 120
};

/* Interval display strings */
static const char* interval_strings[AFF_INTERVAL_COUNT] = {
    "30s", "45s", "60s", "90s", "120s"
};

/*============================================================================
 * Helpers
 *============================================================================*/

static uint32_t get_time_ms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

static float calculate_mean(const float* samples, int count)
{
    if (count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += samples[i];
    }
    return sum / count;
}

static float delta_ms_to_hz(float delta_ms, int64_t carrier_hz)
{
    /* delta_ms is timing error over 60 second interval
     * Fractional error = delta_ms / 60000
     * Hz offset = carrier_hz * (delta_ms / 60000)
     */
    if (carrier_hz == 0) return 0.0f;
    return (float)carrier_hz * (delta_ms / 60000.0f);
}

/*============================================================================
 * Public API
 *============================================================================*/

aff_state_t* aff_create(void)
{
    aff_state_t* aff = (aff_state_t*)calloc(1, sizeof(aff_state_t));
    if (!aff) {
        LOG_ERROR("Failed to allocate aff_state_t");
        return NULL;
    }
    
    aff->enabled = false;
    aff->interval_index = AFF_INTERVAL_60S;  /* Default 60 seconds */
    aff->sample_head = 0;
    aff->sample_count = 0;
    aff->interval_start_ms = get_time_ms();
    aff->interval_elapsed = false;
    aff->adjustment_ready = false;
    
    LOG_INFO("AFF module created");
    return aff;
}

void aff_destroy(aff_state_t* aff)
{
    if (aff) {
        free(aff);
        LOG_INFO("AFF module destroyed");
    }
}

void aff_set_enabled(aff_state_t* aff, bool enabled)
{
    if (!aff) return;
    
    if (enabled && !aff->enabled) {
        /* Enabling - reset state */
        aff_reset(aff);
        LOG_INFO("AFF enabled");
    } else if (!enabled && aff->enabled) {
        LOG_INFO("AFF disabled");
    }
    
    aff->enabled = enabled;
}

bool aff_is_enabled(aff_state_t* aff)
{
    return aff ? aff->enabled : false;
}

void aff_set_interval(aff_state_t* aff, int interval_index)
{
    if (!aff) return;
    
    if (interval_index < 0) interval_index = 0;
    if (interval_index >= AFF_INTERVAL_COUNT) interval_index = AFF_INTERVAL_COUNT - 1;
    
    if (aff->interval_index != interval_index) {
        aff->interval_index = interval_index;
        /* Reset timing when interval changes */
        aff->interval_start_ms = get_time_ms();
        aff->interval_elapsed = false;
        LOG_INFO("AFF interval set to %s", interval_strings[interval_index]);
    }
}

int aff_get_interval(aff_state_t* aff)
{
    return aff ? aff->interval_index : AFF_INTERVAL_60S;
}

int aff_interval_seconds(int interval_index)
{
    if (interval_index < 0 || interval_index >= AFF_INTERVAL_COUNT) {
        return 60;
    }
    return interval_values[interval_index];
}

const char* aff_interval_string(int interval_index)
{
    if (interval_index < 0 || interval_index >= AFF_INTERVAL_COUNT) {
        return "60s";
    }
    return interval_strings[interval_index];
}

void aff_update(aff_state_t* aff, float delta_ms, int64_t carrier_hz, bool is_locked)
{
    if (!aff || !aff->enabled) return;
    
    uint32_t now = get_time_ms();
    aff->last_update_ms = now;
    aff->carrier_hz = carrier_hz;
    
    /* Only collect samples when locked */
    if (!is_locked) {
        return;
    }
    
    /* Add sample to rolling window */
    aff->samples[aff->sample_head] = delta_ms;
    aff->sample_head = (aff->sample_head + 1) % AFF_SAMPLE_COUNT;
    if (aff->sample_count < AFF_SAMPLE_COUNT) {
        aff->sample_count++;
    }
    
    /* Calculate mean delta */
    aff->mean_delta_ms = calculate_mean(aff->samples, aff->sample_count);
    
    /* Convert to Hz */
    aff->drift_hz = delta_ms_to_hz(aff->mean_delta_ms, carrier_hz);
    
    /* Check if interval has elapsed */
    int interval_ms = interval_values[aff->interval_index] * 1000;
    uint32_t elapsed = now - aff->interval_start_ms;
    
    if (elapsed >= (uint32_t)interval_ms && !aff->interval_elapsed) {
        aff->interval_elapsed = true;
        
        /* Check if drift exceeds threshold */
        if (fabsf(aff->drift_hz) >= AFF_THRESHOLD_HZ) {
            /* Calculate adjustment: ±1 Hz max */
            if (aff->drift_hz > 0) {
                aff->adjustment_hz = AFF_MAX_ADJUST_HZ;   /* Drift positive = tune higher */
            } else {
                aff->adjustment_hz = -AFF_MAX_ADJUST_HZ;  /* Drift negative = tune lower */
            }
            aff->adjustment_ready = true;
            
            LOG_INFO("AFF: drift=%.2f Hz, adjustment=%+d Hz", 
                     aff->drift_hz, aff->adjustment_hz);
        } else {
            LOG_DEBUG("AFF: drift=%.2f Hz (below threshold)", aff->drift_hz);
        }
    }
}

bool aff_get_adjustment(aff_state_t* aff, int* adjustment_hz)
{
    if (!aff || !aff->enabled || !aff->adjustment_ready) {
        if (adjustment_hz) *adjustment_hz = 0;
        return false;
    }
    
    if (adjustment_hz) {
        *adjustment_hz = aff->adjustment_hz;
    }
    
    /* Clear adjustment and reset for next interval */
    aff->adjustment_ready = false;
    aff->interval_start_ms = get_time_ms();
    aff->interval_elapsed = false;
    
    /* Clear sample buffer to start fresh */
    aff->sample_count = 0;
    aff->sample_head = 0;
    
    return true;
}

float aff_get_drift_hz(aff_state_t* aff)
{
    return aff ? aff->drift_hz : 0.0f;
}

void aff_reset(aff_state_t* aff)
{
    if (!aff) return;
    
    aff->sample_head = 0;
    aff->sample_count = 0;
    aff->mean_delta_ms = 0.0f;
    aff->drift_hz = 0.0f;
    aff->interval_start_ms = get_time_ms();
    aff->interval_elapsed = false;
    aff->adjustment_ready = false;
    aff->adjustment_hz = 0;
    
    LOG_DEBUG("AFF state reset");
}
