/**
 * Phoenix SDR Controller - Application State
 * 
 * Central state management for the application.
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include "common.h"
#include "sdr_protocol.h"

/* UI mode/page */
typedef enum {
    UI_MODE_MAIN = 0,
    UI_MODE_SETTINGS,
    UI_MODE_ABOUT
} ui_mode_t;

/* Tuning step sizes */
typedef enum {
    STEP_1HZ = 1,
    STEP_10HZ = 10,
    STEP_100HZ = 100,
    STEP_1KHZ = 1000,
    STEP_5KHZ = 5000,
    STEP_10KHZ = 10000,
    STEP_100KHZ = 100000,
    STEP_1MHZ = 1000000
} tuning_step_t;

/* Application state */
typedef struct {
    /* Connection */
    char server_host[256];
    int server_port;
    
    /* SDR state (local copy for UI) */
    int64_t frequency;
    int gain;
    int lna;
    agc_mode_t agc;
    int sample_rate;
    int bandwidth;
    antenna_port_t antenna;
    bool bias_t;
    bool notch;
    bool streaming;
    bool overload;
    
    /* Memory presets */
    sdr_preset_t presets[NUM_PRESETS];
    
    /* UI state */
    ui_mode_t ui_mode;
    tuning_step_t tuning_step;
    int selected_digit;  /* 0-9, for frequency digit selection */
    bool freq_input_active;
    char freq_input_buffer[32];
    bool dc_offset_enabled;  /* +450 Hz DC offset toggle */
    
    /* Connection status */
    connection_state_t conn_state;
    char status_message[256];
    
    /* Timing */
    uint32_t last_status_update;
    uint32_t last_keepalive;
    
    /* Flags */
    bool needs_status_update;
    bool needs_reconnect;
    bool quit_requested;
    
} app_state_t;

/* Create application state */
app_state_t* app_state_create(void);

/* Destroy application state */
void app_state_destroy(app_state_t* state);

/* Reset to defaults */
void app_state_reset(app_state_t* state);

/* Update state from SDR status */
void app_state_update_from_sdr(app_state_t* state, const sdr_status_t* sdr_status);

/* Format frequency for display (returns static buffer) */
const char* app_format_frequency(int64_t freq_hz);

/* Format frequency with digit grouping */
void app_format_frequency_grouped(int64_t freq_hz, char* buffer, size_t buffer_size);

/* Parse frequency from string (supports MHz, kHz, Hz suffixes) */
bool app_parse_frequency(const char* str, int64_t* freq_hz);

/* Get tuning step as string */
const char* app_get_step_string(tuning_step_t step);

/* Cycle to next tuning step */
tuning_step_t app_next_step(tuning_step_t current);

/* Cycle to previous tuning step */
tuning_step_t app_prev_step(tuning_step_t current);

/* Save current settings to a preset slot (0-4) */
void app_save_preset(app_state_t* state, int slot);

/* Apply a preset slot to current state (0-4) */
bool app_recall_preset(app_state_t* state, int slot);

/* Get preset info string for tooltip */
const char* app_get_preset_label(const app_state_t* state, int slot);

/* Save all presets to file */
bool app_save_presets_to_file(const app_state_t* state, const char* filename);

/* Load all presets from file */
bool app_load_presets_from_file(app_state_t* state, const char* filename);

#endif /* APP_STATE_H */
