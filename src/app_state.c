/**
 * Phoenix SDR Controller - Application State Implementation
 * 
 * Central state management for the application.
 */

#include "app_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Static buffer for frequency formatting */
static char s_freq_buffer[32];

/* Tuning step values in order */
static const tuning_step_t s_tuning_steps[] = {
    STEP_1HZ, STEP_10HZ, STEP_100HZ, STEP_1KHZ,
    STEP_5KHZ, STEP_10KHZ, STEP_100KHZ, STEP_1MHZ
};
static const int s_num_steps = sizeof(s_tuning_steps) / sizeof(s_tuning_steps[0]);

/* Tuning step strings */
static const char* s_step_strings[] = {
    "1 Hz", "10 Hz", "100 Hz", "1 kHz",
    "5 kHz", "10 kHz", "100 kHz", "1 MHz"
};

/*
 * Create application state
 */
app_state_t* app_state_create(void)
{
    app_state_t* state = (app_state_t*)calloc(1, sizeof(app_state_t));
    if (!state) {
        LOG_ERROR("Failed to allocate app_state_t");
        return NULL;
    }
    
    app_state_reset(state);
    return state;
}

/*
 * Destroy application state
 */
void app_state_destroy(app_state_t* state)
{
    if (state) {
        free(state);
    }
}

/*
 * Reset state to defaults
 */
void app_state_reset(app_state_t* state)
{
    if (!state) {
        return;
    }
    
    /* Connection defaults */
    strncpy(state->server_host, "127.0.0.1", sizeof(state->server_host) - 1);
    state->server_port = DEFAULT_PORT;
    
    /* SDR state defaults */
    state->frequency = 15000000;      /* 15 MHz */
    state->gain = 40;                 /* 40 dB reduction */
    state->lna = 4;                   /* Middle LNA state */
    state->agc = AGC_OFF;
    state->sample_rate = 2000000;     /* 2 MSPS */
    state->bandwidth = 200;           /* 200 kHz */
    state->antenna = ANTENNA_A;
    state->bias_t = false;
    state->notch = false;
    state->streaming = false;
    state->overload = false;
    
    /* UI state defaults */
    state->ui_mode = UI_MODE_MAIN;
    state->tuning_step = STEP_1KHZ;
    state->selected_digit = -1;
    state->freq_input_active = false;
    state->freq_input_buffer[0] = '\0';
    state->dc_offset_enabled = false;
    
    /* Connection status */
    state->conn_state = CONN_DISCONNECTED;
    strncpy(state->status_message, "Disconnected", sizeof(state->status_message) - 1);
    
    /* Timing */
    state->last_status_update = 0;
    state->last_keepalive = 0;
    
    /* Flags */
    state->needs_status_update = false;
    state->needs_reconnect = false;
    state->quit_requested = false;
    
    LOG_DEBUG("App state reset to defaults");
}

/*
 * Update state from SDR status
 * 
 * NOTE: We only update status indicators (streaming, overload) from the server.
 * User-controlled settings (gain, LNA, freq, etc.) are NOT updated from server
 * because the UI sliders are the source of truth - we tell the server what
 * the settings should be, not the other way around.
 */
void app_state_update_from_sdr(app_state_t* state, const sdr_status_t* sdr_status)
{
    if (!state || !sdr_status) {
        return;
    }
    
    /* Only update status indicators, not user-controlled settings */
    state->streaming = sdr_status->streaming;
    state->overload = sdr_status->overload;
    
    LOG_DEBUG("App state updated from SDR status (streaming=%d, overload=%d)",
              state->streaming, state->overload);
}

/*
 * Format frequency for display (returns static buffer)
 * Format: "XXX.XXX.XXX Hz" or "XX.XXX.XXX MHz" etc.
 */
const char* app_format_frequency(int64_t freq_hz)
{
    if (freq_hz >= 1000000000) {
        /* GHz range */
        snprintf(s_freq_buffer, sizeof(s_freq_buffer), 
                 "%lld.%03lld.%03lld.%03lld Hz",
                 freq_hz / 1000000000,
                 (freq_hz / 1000000) % 1000,
                 (freq_hz / 1000) % 1000,
                 freq_hz % 1000);
    } else if (freq_hz >= 1000000) {
        /* MHz range */
        snprintf(s_freq_buffer, sizeof(s_freq_buffer),
                 "%lld.%03lld.%03lld Hz",
                 freq_hz / 1000000,
                 (freq_hz / 1000) % 1000,
                 freq_hz % 1000);
    } else if (freq_hz >= 1000) {
        /* kHz range */
        snprintf(s_freq_buffer, sizeof(s_freq_buffer),
                 "%lld.%03lld Hz",
                 freq_hz / 1000,
                 freq_hz % 1000);
    } else {
        /* Hz range */
        snprintf(s_freq_buffer, sizeof(s_freq_buffer), "%lld Hz", freq_hz);
    }
    
    return s_freq_buffer;
}

/*
 * Format frequency with digit grouping for UI display
 * Format: "XXX XXX XXX" (groups of 3, right-aligned)
 */
void app_format_frequency_grouped(int64_t freq_hz, char* buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return;
    }
    
    /* Format as 10-digit number with leading zeros */
    char temp[16];
    snprintf(temp, sizeof(temp), "%010lld", freq_hz);
    
    /* Insert spaces for grouping: X XXX XXX XXX */
    if (buffer_size >= 14) {
        buffer[0] = temp[0];
        buffer[1] = ' ';
        buffer[2] = temp[1];
        buffer[3] = temp[2];
        buffer[4] = temp[3];
        buffer[5] = ' ';
        buffer[6] = temp[4];
        buffer[7] = temp[5];
        buffer[8] = temp[6];
        buffer[9] = ' ';
        buffer[10] = temp[7];
        buffer[11] = temp[8];
        buffer[12] = temp[9];
        buffer[13] = '\0';
    } else {
        snprintf(buffer, buffer_size, "%lld", freq_hz);
    }
}

/*
 * Parse frequency from string (supports MHz, kHz, Hz suffixes)
 * Examples: "15000000", "15M", "15MHz", "15000k", "15.5 MHz"
 */
bool app_parse_frequency(const char* str, int64_t* freq_hz)
{
    if (!str || !freq_hz) {
        return false;
    }
    
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return false;
    }
    
    /* Parse numeric part */
    char* endptr;
    double value = strtod(str, &endptr);
    
    if (endptr == str) {
        return false;  /* No number found */
    }
    
    /* Skip whitespace between number and suffix */
    while (*endptr && isspace((unsigned char)*endptr)) {
        endptr++;
    }
    
    /* Check for suffix (case insensitive) */
    double multiplier = 1.0;
    
    if (*endptr) {
        char suffix = (char)toupper((unsigned char)*endptr);
        
        if (suffix == 'G') {
            multiplier = 1000000000.0;
        } else if (suffix == 'M') {
            multiplier = 1000000.0;
        } else if (suffix == 'K') {
            multiplier = 1000.0;
        } else if (suffix == 'H') {
            multiplier = 1.0;  /* Hz, explicit */
        } else if (isdigit((unsigned char)suffix)) {
            /* No suffix, just digits - treat as Hz */
            multiplier = 1.0;
        } else {
            return false;  /* Unknown suffix */
        }
    }
    
    /* Calculate final frequency */
    double result = value * multiplier;
    
    /* Validate range */
    if (result < FREQ_MIN || result > FREQ_MAX) {
        return false;
    }
    
    *freq_hz = (int64_t)(result + 0.5);  /* Round to nearest Hz */
    return true;
}

/*
 * Get tuning step as string
 */
const char* app_get_step_string(tuning_step_t step)
{
    for (int i = 0; i < s_num_steps; i++) {
        if (s_tuning_steps[i] == step) {
            return s_step_strings[i];
        }
    }
    return "1 kHz";
}

/*
 * Find index of current step
 */
static int find_step_index(tuning_step_t step)
{
    for (int i = 0; i < s_num_steps; i++) {
        if (s_tuning_steps[i] == step) {
            return i;
        }
    }
    return 3;  /* Default to 1 kHz */
}

/*
 * Cycle to next tuning step
 */
tuning_step_t app_next_step(tuning_step_t current)
{
    int idx = find_step_index(current);
    idx = (idx + 1) % s_num_steps;
    return s_tuning_steps[idx];
}

/*
 * Cycle to previous tuning step
 */
tuning_step_t app_prev_step(tuning_step_t current)
{
    int idx = find_step_index(current);
    idx = (idx - 1 + s_num_steps) % s_num_steps;
    return s_tuning_steps[idx];
}
