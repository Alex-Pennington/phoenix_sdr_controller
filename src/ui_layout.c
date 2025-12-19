/**
 * Phoenix SDR Controller - UI Layout Implementation
 * 
 * Main application layout and component management.
 */

#include "ui_layout.h"
#include "udp_telemetry.h"
#include "../src/bdc/bcd_decoder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Layout constants */
#define HEADER_HEIGHT 30
#define FOOTER_HEIGHT 22
#define PANEL_PADDING 6
#define WIDGET_SPACING 4
#define BUTTON_HEIGHT 24
#define SLIDER_WIDTH 32
#define SLIDER_HEIGHT 100
#define COMBO_HEIGHT 22
#define LED_RADIUS 5

/* Static combo box items */
static const char* s_agc_items[] = {"OFF", "5HZ", "50HZ", "100HZ"};
static const char* s_srate_items[] = {"2.0 MHz", "4.0 MHz", "6.0 MHz", "8.0 MHz", "10.0 MHz"};
static const char* s_bw_items[] = {"200 kHz", "300 kHz", "600 kHz", "1536 kHz", "5000 kHz", "6000 kHz", "7000 kHz", "8000 kHz"};
static const char* s_antenna_items[] = {"Antenna A", "Antenna B", "Hi-Z"};

/* Sample rate values matching combo items */
static const int s_srate_values[] = {2000000, 4000000, 6000000, 8000000, 10000000};
static const int s_bw_values[] = {200, 300, 600, 1536, 5000, 6000, 7000, 8000};

/* Helper: find combo index for sample rate */
static int srate_to_index(int srate)
{
    for (int i = 0; i < 5; i++) {
        if (s_srate_values[i] == srate) return i;
    }
    return 0;
}

/* Helper: find combo index for bandwidth */
static int bw_to_index(int bw)
{
    for (int i = 0; i < 8; i++) {
        if (s_bw_values[i] == bw) return i;
    }
    return 0;
}

/*
 * Create layout
 */
ui_layout_t* ui_layout_create(ui_core_t* ui)
{
    if (!ui) return NULL;
    
    ui_layout_t* layout = (ui_layout_t*)calloc(1, sizeof(ui_layout_t));
    if (!layout) {
        LOG_ERROR("Failed to allocate ui_layout_t");
        return NULL;
    }
    
    layout->ui = ui;
    
    /* Calculate initial layout regions */
    ui_layout_recalculate(layout);
    
    /* Initialize widgets with default positions (will be updated in recalculate) */
    
    /* Frequency display */
    widget_freq_display_init(&layout->freq_display, 0, 0, 400, 80);
    
    /* Control buttons */
    widget_button_init(&layout->btn_connect, 0, 0, 100, BUTTON_HEIGHT, "Connect");
    widget_button_init(&layout->btn_start, 0, 0, 80, BUTTON_HEIGHT, "Start");
    widget_button_init(&layout->btn_stop, 0, 0, 80, BUTTON_HEIGHT, "Stop");
    
    /* Step buttons */
    widget_button_init(&layout->btn_step_up, 0, 0, 40, BUTTON_HEIGHT, "+");
    widget_button_init(&layout->btn_step_down, 0, 0, 40, BUTTON_HEIGHT, "-");
    
    /* Frequency buttons */
    widget_button_init(&layout->btn_freq_up, 0, 0, 50, BUTTON_HEIGHT, "UP");
    widget_button_init(&layout->btn_freq_down, 0, 0, 50, BUTTON_HEIGHT, "DOWN");
    
    /* Gain sliders */
    widget_slider_init(&layout->slider_gain, 0, 0, SLIDER_WIDTH, SLIDER_HEIGHT,
                       GAIN_MIN, GAIN_MAX, true);
    layout->slider_gain.label = "IF Gain";
    layout->slider_gain.value = 40;
    
    widget_slider_init(&layout->slider_lna, 0, 0, SLIDER_WIDTH, SLIDER_HEIGHT,
                       LNA_MIN, LNA_MAX, true);
    layout->slider_lna.label = "LNA";
    layout->slider_lna.value = 4;
    
    /* Configuration combos */
    widget_combo_init(&layout->combo_agc, 0, 0, 120, COMBO_HEIGHT,
                      s_agc_items, ARRAY_SIZE(s_agc_items));
    layout->combo_agc.label = "AGC Mode";
    
    widget_combo_init(&layout->combo_srate, 0, 0, 120, COMBO_HEIGHT,
                      s_srate_items, ARRAY_SIZE(s_srate_items));
    layout->combo_srate.label = "Sample Rate";
    
    widget_combo_init(&layout->combo_bw, 0, 0, 120, COMBO_HEIGHT,
                      s_bw_items, ARRAY_SIZE(s_bw_items));
    layout->combo_bw.label = "Bandwidth";
    
    widget_combo_init(&layout->combo_antenna, 0, 0, 120, COMBO_HEIGHT,
                      s_antenna_items, ARRAY_SIZE(s_antenna_items));
    layout->combo_antenna.label = "Antenna";
    
    /* Toggles */
    widget_toggle_init(&layout->toggle_biast, 0, 0, "Bias-T");
    widget_toggle_init(&layout->toggle_notch, 0, 0, "FM Notch");
    widget_toggle_init(&layout->toggle_aff, 0, 0, "AFF");
    
    /* AFF interval slider (vertical) */
    widget_slider_init(&layout->slider_aff_interval, 0, 0, SLIDER_WIDTH, 80,
                       AFF_INTERVAL_30S, AFF_INTERVAL_120S, true);
    layout->slider_aff_interval.label = "AFF Int";
    layout->slider_aff_interval.value = AFF_INTERVAL_60S;
    
    /* WWV frequency shortcut buttons */
    widget_button_init(&layout->btn_wwv_2_5, 0, 0, 50, 24, "2.5");
    widget_button_init(&layout->btn_wwv_5, 0, 0, 40, 24, "5");
    widget_button_init(&layout->btn_wwv_10, 0, 0, 40, 24, "10");
    widget_button_init(&layout->btn_wwv_15, 0, 0, 40, 24, "15");
    widget_button_init(&layout->btn_wwv_20, 0, 0, 40, 24, "20");
    widget_button_init(&layout->btn_wwv_25, 0, 0, 40, 24, "25");
    widget_button_init(&layout->btn_wwv_30, 0, 0, 40, 24, "30");
    
    /* Memory preset buttons M1-M5 */
    static const char* preset_labels[NUM_PRESETS] = {"M1", "M2", "M3", "M4", "M5"};
    for (int i = 0; i < NUM_PRESETS; i++) {
        widget_button_init(&layout->btn_preset[i], 0, 0, 40, 24, preset_labels[i]);
    }
    
    /* External process buttons */
    widget_button_init(&layout->btn_server, 0, 0, 60, 24, "Server");
    widget_button_init(&layout->btn_waterfall, 0, 0, 60, 24, "Wfall");
    
    /* DC offset dot (positioned in recalculate) */
    layout->offset_dot.x = 0;
    layout->offset_dot.y = 0;
    layout->offset_dot.w = 12;
    layout->offset_dot.h = 12;
    
    /* LEDs */
    widget_led_init(&layout->led_connected, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_RED, "Connected");
    widget_led_init(&layout->led_streaming, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_TEXT_DIM, "Streaming");
    widget_led_init(&layout->led_overload, 0, 0, LED_RADIUS, COLOR_RED, COLOR_TEXT_DIM, "Overload");
    
    /* Panels */
    widget_panel_init(&layout->panel_freq, 0, 0, 0, 0, "Frequency");
    widget_panel_init(&layout->panel_gain, 0, 0, 0, 0, "Gain Control");
    widget_panel_init(&layout->panel_config, 0, 0, 0, 0, "Configuration");
    widget_panel_init(&layout->panel_wwv, 0, 0, 0, 0, "WWV Stats");
    
    /* WWV telemetry indicator LEDs */
    widget_led_init(&layout->led_tone500, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_TEXT_DIM, "500Hz");
    widget_led_init(&layout->led_tone600, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_TEXT_DIM, "600Hz");
    widget_led_init(&layout->led_match, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_RED, "Match");
    
    /* BCD time code panel */
    widget_panel_init(&layout->panel_bcd, 0, 0, 0, 0, "BCD Time");
    widget_led_init(&layout->led_bcd_sync, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_TEXT_DIM, "Sync");
    
    /* Initialize debug mode */
    layout->debug_mode = false;
    
    /* Recalculate to position everything */
    ui_layout_recalculate(layout);
    
    LOG_INFO("UI Layout created");
    return layout;
}

/*
 * Destroy layout
 */
void ui_layout_destroy(ui_layout_t* layout)
{
    if (layout) {
        free(layout);
        LOG_INFO("UI Layout destroyed");
    }
}

/*
 * Recalculate layout - COMPACT VERSION
 * Layout: Header with LEDs | Freq row | Tuning+WWV+Presets | Gain|Config|Control columns
 */
void ui_layout_recalculate(ui_layout_t* layout)
{
    if (!layout || !layout->ui) return;
    
    int w = layout->ui->window_width;
    int h = layout->ui->window_height;
    
    /* Header */
    layout->regions.header.x = 0;
    layout->regions.header.y = 0;
    layout->regions.header.w = w;
    layout->regions.header.h = HEADER_HEIGHT;
    
    /* Footer */
    layout->regions.footer.x = 0;
    layout->regions.footer.y = h - FOOTER_HEIGHT;
    layout->regions.footer.w = w;
    layout->regions.footer.h = FOOTER_HEIGHT;
    
    /* LEDs in header (right side) - spaced for labels */
    int led_y = HEADER_HEIGHT / 2;
    layout->led_connected.x = w - 200;
    layout->led_connected.y = led_y;
    layout->led_streaming.x = w - 130;
    layout->led_streaming.y = led_y;
    layout->led_overload.x = w - 55;
    layout->led_overload.y = led_y;
    
    /* Content area */
    int content_y = HEADER_HEIGHT + PANEL_PADDING;
    
    /* === ROW 1: Frequency display (full width, taller for 32pt font) === */
    int freq_h = 55;
    layout->regions.freq_area.x = PANEL_PADDING;
    layout->regions.freq_area.y = content_y;
    layout->regions.freq_area.w = w - PANEL_PADDING * 2;
    layout->regions.freq_area.h = freq_h;
    
    layout->panel_freq.x = layout->regions.freq_area.x;
    layout->panel_freq.y = layout->regions.freq_area.y;
    layout->panel_freq.w = layout->regions.freq_area.w;
    layout->panel_freq.h = layout->regions.freq_area.h;
    
    /* Frequency display inside panel (vertically centered) */
    layout->freq_display.x = layout->panel_freq.x + 5;
    layout->freq_display.y = layout->panel_freq.y + 20;
    layout->freq_display.w = layout->panel_freq.w - 40;
    layout->freq_display.h = freq_h - 20;
    
    /* DC offset dot (next to frequency) */
    layout->offset_dot.x = layout->freq_display.x + layout->freq_display.w + 8;
    layout->offset_dot.y = layout->freq_display.y + (layout->freq_display.h / 2) - 5;
    layout->offset_dot.w = 10;
    layout->offset_dot.h = 10;
    
    /* === ROW 2: Tuning buttons + WWV buttons (same row) === */
    int row2_y = content_y + freq_h + PANEL_PADDING;
    int btn_spacing = 4;
    
    /* Tuning buttons on left */
    layout->btn_freq_down.x = PANEL_PADDING;
    layout->btn_freq_down.y = row2_y;
    layout->btn_freq_down.w = 40;
    layout->btn_freq_down.h = 22;
    
    layout->btn_freq_up.x = layout->btn_freq_down.x + layout->btn_freq_down.w + btn_spacing;
    layout->btn_freq_up.y = row2_y;
    layout->btn_freq_up.w = 40;
    layout->btn_freq_up.h = 22;
    
    layout->btn_step_down.x = layout->btn_freq_up.x + layout->btn_freq_up.w + 12;
    layout->btn_step_down.y = row2_y;
    layout->btn_step_down.w = 30;
    layout->btn_step_down.h = 22;
    
    layout->btn_step_up.x = layout->btn_step_down.x + layout->btn_step_down.w + btn_spacing;
    layout->btn_step_up.y = row2_y;
    layout->btn_step_up.w = 30;
    layout->btn_step_up.h = 22;
    
    /* WWV buttons (after tuning) */
    int wwv_x = layout->btn_step_up.x + layout->btn_step_up.w + 20;
    layout->btn_wwv_2_5.x = wwv_x;
    layout->btn_wwv_2_5.y = row2_y;
    layout->btn_wwv_2_5.w = 32;
    layout->btn_wwv_2_5.h = 22;
    wwv_x += layout->btn_wwv_2_5.w + 3;
    
    layout->btn_wwv_5.x = wwv_x;
    layout->btn_wwv_5.y = row2_y;
    layout->btn_wwv_5.w = 28;
    layout->btn_wwv_5.h = 22;
    wwv_x += layout->btn_wwv_5.w + 3;
    
    layout->btn_wwv_10.x = wwv_x;
    layout->btn_wwv_10.y = row2_y;
    layout->btn_wwv_10.w = 28;
    layout->btn_wwv_10.h = 22;
    wwv_x += layout->btn_wwv_10.w + 3;
    
    layout->btn_wwv_15.x = wwv_x;
    layout->btn_wwv_15.y = row2_y;
    layout->btn_wwv_15.w = 28;
    layout->btn_wwv_15.h = 22;
    wwv_x += layout->btn_wwv_15.w + 3;
    
    layout->btn_wwv_20.x = wwv_x;
    layout->btn_wwv_20.y = row2_y;
    layout->btn_wwv_20.w = 28;
    layout->btn_wwv_20.h = 22;
    wwv_x += layout->btn_wwv_20.w + 3;
    
    layout->btn_wwv_25.x = wwv_x;
    layout->btn_wwv_25.y = row2_y;
    layout->btn_wwv_25.w = 28;
    layout->btn_wwv_25.h = 22;
    wwv_x += layout->btn_wwv_25.w + 3;
    
    layout->btn_wwv_30.x = wwv_x;
    layout->btn_wwv_30.y = row2_y;
    layout->btn_wwv_30.w = 28;
    layout->btn_wwv_30.h = 22;
    
    /* === ROW 3: Memory preset buttons === */
    int row3_y = row2_y + 26;
    int preset_x = PANEL_PADDING;
    
    for (int i = 0; i < NUM_PRESETS; i++) {
        layout->btn_preset[i].x = preset_x;
        layout->btn_preset[i].y = row3_y;
        layout->btn_preset[i].w = 36;
        layout->btn_preset[i].h = 22;
        preset_x += layout->btn_preset[i].w + 4;
    }
    
    /* External process buttons (after presets, with gap) */
    preset_x += 10;  /* Extra gap */
    layout->btn_server.x = preset_x;
    layout->btn_server.y = row3_y;
    layout->btn_server.w = 52;
    layout->btn_server.h = 22;
    preset_x += layout->btn_server.w + 4;
    
    layout->btn_waterfall.x = preset_x;
    layout->btn_waterfall.y = row3_y;
    layout->btn_waterfall.w = 52;
    layout->btn_waterfall.h = 22;
    
    /* === ROW 4: Four columns - Gain | Config | Control | WWV Stats === */
    int row4_y = row3_y + 28;
    int col_h = h - row4_y - FOOTER_HEIGHT - PANEL_PADDING;
    
    /* Column widths - adjusted for 4 columns */
    int gain_w = 100;
    int config_w = 150;
    int control_w = 140;
    int wwv_w = w - gain_w - config_w - control_w - PANEL_PADDING * 5;
    if (wwv_w < 180) wwv_w = 180;
    
    /* Gain panel (left column) */
    layout->regions.gain_panel.x = PANEL_PADDING;
    layout->regions.gain_panel.y = row4_y;
    layout->regions.gain_panel.w = gain_w;
    layout->regions.gain_panel.h = col_h;
    
    layout->panel_gain.x = layout->regions.gain_panel.x;
    layout->panel_gain.y = layout->regions.gain_panel.y;
    layout->panel_gain.w = layout->regions.gain_panel.w;
    layout->panel_gain.h = col_h;
    
    /* Gain sliders (side by side with more spacing) */
    layout->slider_gain.x = layout->regions.gain_panel.x + 18;
    layout->slider_gain.y = row4_y + 35;
    layout->slider_gain.h = col_h - 50;
    
    layout->slider_lna.x = layout->regions.gain_panel.x + 60;
    layout->slider_lna.y = row4_y + 35;
    layout->slider_lna.h = col_h - 50;
    
    /* Config panel (middle column) */
    int config_x = PANEL_PADDING + gain_w + PANEL_PADDING;
    layout->regions.config_panel.x = config_x;
    layout->regions.config_panel.y = row4_y;
    layout->regions.config_panel.w = config_w;
    layout->regions.config_panel.h = col_h;
    
    layout->panel_config.x = config_x;
    layout->panel_config.y = row4_y;
    layout->panel_config.w = config_w;
    layout->panel_config.h = col_h;
    
    /* Config combos (stacked with more vertical spacing for labels) */
    int combo_x = config_x + 8;
    int combo_w = config_w - 16;
    int combo_spacing = 46;  /* More space for label + combo */
    
    layout->combo_agc.x = combo_x;
    layout->combo_agc.y = row4_y + 32;
    layout->combo_agc.w = combo_w;
    
    layout->combo_srate.x = combo_x;
    layout->combo_srate.y = row4_y + 32 + combo_spacing;
    layout->combo_srate.w = combo_w;
    
    layout->combo_bw.x = combo_x;
    layout->combo_bw.y = row4_y + 32 + combo_spacing * 2;
    layout->combo_bw.w = combo_w;
    
    layout->combo_antenna.x = combo_x;
    layout->combo_antenna.y = row4_y + 32 + combo_spacing * 3;
    layout->combo_antenna.w = combo_w;
    
    /* Control panel (right column) */
    int status_x = config_x + config_w + PANEL_PADDING;
    layout->regions.status_panel.x = status_x;
    layout->regions.status_panel.y = row4_y;
    layout->regions.status_panel.w = control_w;
    layout->regions.status_panel.h = col_h;
    
    /* Connect button */
    layout->btn_connect.x = status_x + 8;
    layout->btn_connect.y = row4_y + 10;
    layout->btn_connect.w = control_w - 16;
    layout->btn_connect.h = 26;
    
    /* Start/Stop buttons (side by side) */
    int half_btn = (control_w - 24) / 2;
    layout->btn_start.x = status_x + 8;
    layout->btn_start.y = row4_y + 44;
    layout->btn_start.w = half_btn;
    layout->btn_start.h = 26;
    
    layout->btn_stop.x = status_x + 8 + half_btn + 8;
    layout->btn_stop.y = row4_y + 44;
    layout->btn_stop.w = half_btn;
    layout->btn_stop.h = 26;
    
    /* Toggles (below buttons with more spacing) */
    layout->toggle_biast.x = status_x + 8;
    layout->toggle_biast.y = row4_y + 82;
    
    layout->toggle_notch.x = status_x + 8;
    layout->toggle_notch.y = row4_y + 112;
    
    layout->toggle_aff.x = status_x + 8;
    layout->toggle_aff.y = row4_y + 142;
    
    /* AFF interval slider (right side of config panel) */
    layout->slider_aff_interval.x = config_x + config_w - 45;
    layout->slider_aff_interval.y = row4_y + 35;
    layout->slider_aff_interval.h = col_h - 50;
    
    /* WWV Stats panel (rightmost column - top half) */
    int wwv_panel_x = status_x + control_w + PANEL_PADDING;
    int wwv_h = (col_h - PANEL_PADDING) / 2;  /* Split column in half */
    layout->regions.wwv_panel.x = wwv_panel_x;
    layout->regions.wwv_panel.y = row4_y;
    layout->regions.wwv_panel.w = wwv_w;
    layout->regions.wwv_panel.h = wwv_h;
    
    layout->panel_wwv.x = wwv_panel_x;
    layout->panel_wwv.y = row4_y;
    layout->panel_wwv.w = wwv_w;
    layout->panel_wwv.h = wwv_h;
    
    /* WWV indicator LEDs (positioned inside panel) */
    int wwv_led_y = row4_y + wwv_h - 30;
    layout->led_tone500.x = wwv_panel_x + 20;
    layout->led_tone500.y = wwv_led_y;
    layout->led_tone600.x = wwv_panel_x + 70;
    layout->led_tone600.y = wwv_led_y;
    layout->led_match.x = wwv_panel_x + 120;
    layout->led_match.y = wwv_led_y;
    
    /* BCD Time panel (rightmost column - bottom half) */
    int bcd_panel_y = row4_y + wwv_h + PANEL_PADDING;
    int bcd_h = col_h - wwv_h - PANEL_PADDING;
    layout->regions.bcd_panel.x = wwv_panel_x;
    layout->regions.bcd_panel.y = bcd_panel_y;
    layout->regions.bcd_panel.w = wwv_w;
    layout->regions.bcd_panel.h = bcd_h;
    
    layout->panel_bcd.x = wwv_panel_x;
    layout->panel_bcd.y = bcd_panel_y;
    layout->panel_bcd.w = wwv_w;
    layout->panel_bcd.h = bcd_h;
    
    /* BCD sync LED (inside panel) */
    layout->led_bcd_sync.x = wwv_panel_x + 20;
    layout->led_bcd_sync.y = bcd_panel_y + bcd_h - 30;
    
    /* Tuning panel region (not visible, just for event tracking) */
    layout->regions.tuning_panel.x = PANEL_PADDING;
    layout->regions.tuning_panel.y = row2_y;
    layout->regions.tuning_panel.w = w - PANEL_PADDING * 2;
    layout->regions.tuning_panel.h = 50;
}

/*
 * Update layout from app state
 */
void ui_layout_sync_state(ui_layout_t* layout, const app_state_t* state)
{
    if (!layout || !state) return;
    
    /* Frequency display */
    layout->freq_display.frequency = state->frequency;
    
    /* Gain sliders */
    layout->slider_gain.value = state->gain;
    layout->slider_lna.value = state->lna;
    
    /* Update LNA slider max based on antenna (Hi-Z has reduced LNA states) */
    if (state->antenna == ANTENNA_HIZ) {
        layout->slider_lna.max_val = LNA_MAX_HIZ;
    } else {
        layout->slider_lna.max_val = LNA_MAX;
    }
    
    /* Config combos */
    layout->combo_agc.selected = (int)state->agc;
    layout->combo_srate.selected = srate_to_index(state->sample_rate);
    layout->combo_bw.selected = bw_to_index(state->bandwidth);
    layout->combo_antenna.selected = (int)state->antenna;
    
    /* Toggles */
    layout->toggle_biast.value = state->bias_t;
    layout->toggle_notch.value = state->notch;
    
    /* LEDs */
    layout->led_connected.on = (state->conn_state == CONN_CONNECTED);
    layout->led_streaming.on = state->streaming;
    layout->led_overload.on = state->overload;
    
    /* Button states based on connection */
    bool connected = (state->conn_state == CONN_CONNECTED);
    layout->btn_start.enabled = connected && !state->streaming;
    layout->btn_stop.enabled = connected && state->streaming;
    
    /* Allow controls to be used when disconnected (they set local state,
       and will be pushed to server on connect) */
    layout->slider_gain.enabled = true;
    layout->slider_lna.enabled = true;
    layout->combo_agc.enabled = true;
    layout->combo_srate.enabled = !state->streaming;  /* Can't change while streaming */
    layout->combo_bw.enabled = !state->streaming;     /* Can't change while streaming */
    layout->combo_antenna.enabled = true;
    layout->toggle_biast.enabled = true;
    layout->toggle_notch.enabled = true;
    
    /* Update connect button label */
    layout->btn_connect.label = connected ? "Disconnect" : "Connect";
}

/*
 * Update process button states from process manager
 */
void ui_layout_sync_process_state(ui_layout_t* layout, process_manager_t* pm)
{
    if (!layout || !pm) return;
    
    /* Update server button label based on running state */
    bool server_running = process_manager_is_running(pm, PROC_SDR_SERVER);
    layout->btn_server.label = server_running ? "Stop S" : "Server";
    
    /* Update waterfall button label based on running state */
    bool wfall_running = process_manager_is_running(pm, PROC_WATERFALL);
    layout->btn_waterfall.label = wfall_running ? "Stop W" : "Wfall";
}

/*
 * Update widgets and get actions
 */
void ui_layout_update(ui_layout_t* layout, const mouse_state_t* mouse,
                      SDL_Event* event, ui_actions_t* actions)
{
    if (!layout || !mouse || !actions) return;
    
    /* Clear actions */
    memset(actions, 0, sizeof(ui_actions_t));
    
    /* Handle debug mode clicks */
    if (layout->debug_mode && mouse->left_clicked) {
        ui_layout_debug_click(layout, mouse->x, mouse->y);
        return;  /* Don't process normal UI interactions in debug mode */
    }
    
    /* Update LEDs for hover state */
    widget_led_update(&layout->led_connected, mouse);
    widget_led_update(&layout->led_streaming, mouse);
    widget_led_update(&layout->led_overload, mouse);
    widget_led_update(&layout->led_tone500, mouse);
    widget_led_update(&layout->led_tone600, mouse);
    widget_led_update(&layout->led_match, mouse);
    widget_led_update(&layout->led_bcd_sync, mouse);
    
    /* Handle window resize */
    if (layout->ui->window_width != layout->regions.header.w ||
        layout->ui->window_height != layout->regions.footer.y + FOOTER_HEIGHT) {
        ui_layout_recalculate(layout);
    }
    
    /* Update buttons */
    if (widget_button_update(&layout->btn_connect, mouse)) {
        if (layout->led_connected.on) {
            actions->disconnect_clicked = true;
        } else {
            actions->connect_clicked = true;
        }
    }
    
    if (widget_button_update(&layout->btn_start, mouse)) {
        actions->start_clicked = true;
    }
    
    if (widget_button_update(&layout->btn_stop, mouse)) {
        actions->stop_clicked = true;
    }
    
    if (widget_button_update(&layout->btn_step_up, mouse)) {
        actions->step_up = true;
    }
    
    if (widget_button_update(&layout->btn_step_down, mouse)) {
        actions->step_down = true;
    }
    
    if (widget_button_update(&layout->btn_freq_up, mouse)) {
        actions->freq_up = true;
    }
    
    if (widget_button_update(&layout->btn_freq_down, mouse)) {
        actions->freq_down = true;
    }
    
    /* Update frequency display */
    if (widget_freq_display_update(&layout->freq_display, mouse, event)) {
        actions->freq_changed = true;
        actions->new_frequency = layout->freq_display.frequency;
    }
    
    /* Update sliders */
    if (widget_slider_update(&layout->slider_gain, mouse)) {
        actions->gain_changed = true;
        actions->new_gain = layout->slider_gain.value;
    }
    
    if (widget_slider_update(&layout->slider_lna, mouse)) {
        actions->lna_changed = true;
        actions->new_lna = layout->slider_lna.value;
    }
    
    /* Update combos */
    if (widget_combo_update(&layout->combo_agc, mouse)) {
        actions->agc_changed = true;
        actions->new_agc = (agc_mode_t)layout->combo_agc.selected;
    }
    
    if (widget_combo_update(&layout->combo_srate, mouse)) {
        actions->srate_changed = true;
        actions->new_srate = s_srate_values[layout->combo_srate.selected];
    }
    
    if (widget_combo_update(&layout->combo_bw, mouse)) {
        actions->bw_changed = true;
        actions->new_bw = s_bw_values[layout->combo_bw.selected];
    }
    
    if (widget_combo_update(&layout->combo_antenna, mouse)) {
        actions->antenna_changed = true;
        actions->new_antenna = (antenna_port_t)layout->combo_antenna.selected;
    }
    
    /* Update toggles */
    if (widget_toggle_update(&layout->toggle_biast, mouse)) {
        actions->biast_changed = true;
        actions->new_biast = layout->toggle_biast.value;
    }
    
    if (widget_toggle_update(&layout->toggle_notch, mouse)) {
        actions->notch_changed = true;
        actions->new_notch = layout->toggle_notch.value;
    }
    
    if (widget_toggle_update(&layout->toggle_aff, mouse)) {
        actions->aff_toggled = true;
        actions->new_aff = layout->toggle_aff.value;
    }
    
    if (widget_slider_update(&layout->slider_aff_interval, mouse)) {
        actions->aff_interval_changed = true;
        actions->new_aff_interval = layout->slider_aff_interval.value;
    }
    
    /* Check for DC offset dot click */
    if (mouse->left_clicked) {
        if (mouse->x >= layout->offset_dot.x && 
            mouse->x < layout->offset_dot.x + layout->offset_dot.w &&
            mouse->y >= layout->offset_dot.y && 
            mouse->y < layout->offset_dot.y + layout->offset_dot.h) {
            actions->dc_offset_toggled = true;
        }
    }
    
    /* Update WWV buttons */
    if (widget_button_update(&layout->btn_wwv_2_5, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_2_5_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_5, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_5_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_10, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_10_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_15, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_15_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_20, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_20_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_25, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_25_MHZ;
    }
    if (widget_button_update(&layout->btn_wwv_30, mouse)) {
        actions->wwv_clicked = true;
        actions->wwv_frequency = WWV_30_MHZ;
    }
    
    /* Update Memory Preset buttons - detect Ctrl key for save vs recall */
    SDL_Keymod mod = SDL_GetModState();
    bool ctrl_held = (mod & KMOD_CTRL) != 0;
    
    for (int i = 0; i < NUM_PRESETS; i++) {
        if (widget_button_update(&layout->btn_preset[i], mouse)) {
            actions->preset_clicked = true;
            actions->preset_index = i;
            actions->preset_save = ctrl_held;
        }
    }
    
    /* Update external process buttons */
    if (widget_button_update(&layout->btn_server, mouse)) {
        actions->server_toggled = true;
    }
    if (widget_button_update(&layout->btn_waterfall, mouse)) {
        actions->waterfall_toggled = true;
    }
}

/*
 * Draw the complete layout
 */
void ui_layout_draw(ui_layout_t* layout, const app_state_t* state)
{
    if (!layout || !layout->ui) return;
    
    /* Draw header */
    ui_layout_draw_header(layout, state);
    
    /* Draw panels */
    widget_panel_draw(&layout->panel_freq, layout->ui);
    widget_panel_draw(&layout->panel_gain, layout->ui);
    widget_panel_draw(&layout->panel_config, layout->ui);
    
    /* Draw frequency display */
    widget_freq_display_draw(&layout->freq_display, layout->ui);
    
    /* Draw DC offset indicator dot (next to frequency) */
    if (state) {
        uint32_t dot_color = state->dc_offset_enabled ? COLOR_ACCENT : COLOR_TEXT_DIM;
        ui_draw_rect(layout->ui, layout->offset_dot.x, layout->offset_dot.y,
                     layout->offset_dot.w, layout->offset_dot.h, dot_color);
        /* Draw a small border */
        ui_draw_rect_outline(layout->ui, layout->offset_dot.x, layout->offset_dot.y,
                             layout->offset_dot.w, layout->offset_dot.h, COLOR_TEXT);
    }
    
    /* Draw tuning panel background */
    ui_draw_rect(layout->ui, layout->regions.tuning_panel.x, layout->regions.tuning_panel.y,
                 layout->regions.tuning_panel.w, layout->regions.tuning_panel.h, COLOR_BG_PANEL);
    
    /* Draw step label - moved further right to avoid WWV button overlap */
    if (state) {
        char step_str[32];
        snprintf(step_str, sizeof(step_str), "Step: %s", app_get_step_string(state->tuning_step));
        /* Position it between step buttons and WWV buttons with padding */
        int label_x = layout->btn_step_up.x + layout->btn_step_up.w + 6;
        ui_draw_text(layout->ui, layout->ui->font_normal, step_str,
                    label_x, layout->btn_step_up.y + 3, COLOR_ACCENT);
    }
    
    /* Draw tuning buttons */
    widget_button_draw(&layout->btn_freq_down, layout->ui);
    widget_button_draw(&layout->btn_freq_up, layout->ui);
    widget_button_draw(&layout->btn_step_down, layout->ui);
    widget_button_draw(&layout->btn_step_up, layout->ui);
    
    /* Draw WWV shortcut buttons */
    widget_button_draw(&layout->btn_wwv_2_5, layout->ui);
    widget_button_draw(&layout->btn_wwv_5, layout->ui);
    widget_button_draw(&layout->btn_wwv_10, layout->ui);
    widget_button_draw(&layout->btn_wwv_15, layout->ui);
    widget_button_draw(&layout->btn_wwv_20, layout->ui);
    widget_button_draw(&layout->btn_wwv_25, layout->ui);
    widget_button_draw(&layout->btn_wwv_30, layout->ui);
    
    /* Draw Memory Preset buttons */
    for (int i = 0; i < NUM_PRESETS; i++) {
        widget_button_draw(&layout->btn_preset[i], layout->ui);
    }
    
    /* Draw external process buttons */
    widget_button_draw(&layout->btn_server, layout->ui);
    widget_button_draw(&layout->btn_waterfall, layout->ui);
    
    /* Draw gain sliders */
    widget_slider_draw(&layout->slider_gain, layout->ui);
    widget_slider_draw(&layout->slider_lna, layout->ui);
    
    /* Draw config combos */
    widget_combo_draw(&layout->combo_agc, layout->ui);
    widget_combo_draw(&layout->combo_srate, layout->ui);
    widget_combo_draw(&layout->combo_bw, layout->ui);
    widget_combo_draw(&layout->combo_antenna, layout->ui);
    
    /* Draw status panel background */
    ui_draw_rect(layout->ui, layout->regions.status_panel.x, layout->regions.status_panel.y,
                 layout->regions.status_panel.w, layout->regions.status_panel.h, COLOR_BG_PANEL);
    
    /* Draw control buttons */
    widget_button_draw(&layout->btn_connect, layout->ui);
    widget_button_draw(&layout->btn_start, layout->ui);
    widget_button_draw(&layout->btn_stop, layout->ui);
    
    /* Draw toggles */
    widget_toggle_draw(&layout->toggle_biast, layout->ui);
    widget_toggle_draw(&layout->toggle_notch, layout->ui);
    widget_toggle_draw(&layout->toggle_aff, layout->ui);
    
    /* Draw AFF interval slider */
    widget_slider_draw(&layout->slider_aff_interval, layout->ui);
    
    /* Draw footer */
    ui_layout_draw_footer(layout, state);
}

/*
 * Draw header bar
 */
void ui_layout_draw_header(ui_layout_t* layout, const app_state_t* state)
{
    if (!layout || !layout->ui) return;
    
    /* Draw header background */
    ui_draw_rect(layout->ui, 0, 0, layout->regions.header.w, layout->regions.header.h, COLOR_BG_PANEL);
    ui_draw_line(layout->ui, 0, layout->regions.header.h - 1, 
                layout->regions.header.w, layout->regions.header.h - 1, COLOR_ACCENT_DIM);
    
    /* Draw title */
    ui_draw_text(layout->ui, layout->ui->font_title, APP_NAME, 8, 5, COLOR_ACCENT);
    
    /* Draw LEDs in header (right side) */
    widget_led_draw(&layout->led_connected, layout->ui);
    widget_led_draw(&layout->led_streaming, layout->ui);
    widget_led_draw(&layout->led_overload, layout->ui);
}

/*
 * Draw footer/status bar
 */
void ui_layout_draw_footer(ui_layout_t* layout, const app_state_t* state)
{
    if (!layout || !layout->ui) return;
    
    /* Draw footer background */
    ui_draw_rect(layout->ui, 0, layout->regions.footer.y,
                 layout->regions.footer.w, layout->regions.footer.h, COLOR_BG_PANEL);
    ui_draw_line(layout->ui, 0, layout->regions.footer.y,
                 layout->regions.footer.w, layout->regions.footer.y, COLOR_ACCENT_DIM);
    
    /* Draw status message */
    if (state) {
        ui_draw_text(layout->ui, layout->ui->font_small, state->status_message,
                    10, layout->regions.footer.y + 8, COLOR_TEXT);
        
        /* Draw connection info on right side */
        if (state->conn_state == CONN_CONNECTED) {
            char conn_str[64];
            snprintf(conn_str, sizeof(conn_str), "%s:%d", state->server_host, state->server_port);
            ui_draw_text_right(layout->ui, layout->ui->font_small, conn_str,
                               0, layout->regions.footer.y + 8, 
                               layout->regions.footer.w - 10, COLOR_GREEN);
        }
    }
}

/*
 * Sync WWV telemetry data to LED states
 */
void ui_layout_sync_telemetry(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout) return;
    
    if (!telem || !telem->subcarrier.valid) {
        /* No data - turn off LEDs */
        layout->led_tone500.on = false;
        layout->led_tone600.on = false;
        layout->led_match.on = false;
        return;
    }
    
    /* Update tone LEDs based on which is detected */
    layout->led_tone500.on = (telem->subcarrier.detected == SUBCAR_500HZ);
    layout->led_tone600.on = (telem->subcarrier.detected == SUBCAR_600HZ);
    layout->led_match.on = telem->subcarrier.match;
}

/*
 * Draw WWV telemetry panel
 */
void ui_layout_draw_wwv_panel(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_wwv, layout->ui);
    
    int x = layout->regions.wwv_panel.x + 8;
    int y = layout->regions.wwv_panel.y + 22;
    int line_h = 18;
    char buf[64];
    
    /* Check if we have data */
    bool has_data = telem && (telem->channel.valid || telem->carrier.valid);
    
    if (!has_data) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No telemetry data", 
                     x, y, COLOR_TEXT_DIM);
        ui_draw_text(layout->ui, layout->ui->font_small, "Waiting on UDP 3005...", 
                     x, y + line_h, COLOR_TEXT_DIM);
        return;
    }
    
    /* === Channel Quality === */
    if (telem->channel.valid) {
        /* Quality indicator with color */
        uint32_t quality_color;
        switch (telem->channel.quality) {
            case QUALITY_GOOD: quality_color = COLOR_GREEN; break;
            case QUALITY_FAIR: quality_color = COLOR_YELLOW; break;
            case QUALITY_POOR: quality_color = COLOR_ORANGE; break;
            default: quality_color = COLOR_RED; break;
        }
        
        snprintf(buf, sizeof(buf), "Quality: %s", 
                 udp_telemetry_quality_str(telem->channel.quality));
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, quality_color);
        y += line_h;
        
        /* SNR */
        snprintf(buf, sizeof(buf), "SNR: %.1f dB", telem->channel.snr_db);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        y += line_h;
        
        /* Noise floor */
        snprintf(buf, sizeof(buf), "Noise: %.1f dB", telem->channel.noise_db);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
        y += line_h;
    }
    
    y += 4; /* Gap between sections */
    
    /* === Carrier Offset === */
    if (telem->carrier.valid) {
        uint32_t offset_color = telem->carrier.measurement_valid ? COLOR_ACCENT : COLOR_TEXT_DIM;
        
        snprintf(buf, sizeof(buf), "Offset: %+.2f Hz", telem->carrier.offset_hz);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, offset_color);
        y += line_h;
        
        snprintf(buf, sizeof(buf), "       %+.2f ppm", telem->carrier.offset_ppm);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, offset_color);
        y += line_h;
    }
    
    y += 4;
    
    /* === Subcarrier Status with WWV/WWVH Schedule === */
    if (telem->subcarrier.valid) {
        int minute = telem->subcarrier.minute;
        
        /* Get expected tones for each station */
        wwv_tone_t wwv_tone = wwv_get_tone(minute);
        wwv_tone_t wwvh_tone = wwvh_get_tone(minute);
        wwv_special_t wwv_special = wwv_get_special(minute);
        wwv_special_t wwvh_special = wwvh_get_special(minute);
        
        /* Minute header with special broadcast indicator */
        if (wwv_special != SPECIAL_NONE) {
            snprintf(buf, sizeof(buf), "Min %02d [WWV %s]", minute, wwv_special_str(wwv_special));
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_ORANGE);
        } else if (wwvh_special != SPECIAL_NONE) {
            snprintf(buf, sizeof(buf), "Min %02d [WWVH %s]", minute, wwv_special_str(wwvh_special));
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_ORANGE);
        } else {
            snprintf(buf, sizeof(buf), "Min %02d", minute);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        }
        y += line_h;
        
        /* Detect header */
        ui_draw_text(layout->ui, layout->ui->font_small, "Detect:", x, y, COLOR_TEXT_DIM);
        y += line_h;
        
        /* 500 Hz line: show detected state and which station broadcasts it */
        bool tone500_detected = (telem->subcarrier.detected == SUBCAR_500HZ);
        const char* tone500_status = tone500_detected ? "(on) " : "(off)";
        uint32_t tone500_color = tone500_detected ? COLOR_GREEN : COLOR_TEXT_DIM;
        
        /* Which station(s) broadcast 500 Hz this minute? */
        const char* tone500_station = "";
        if (wwv_tone == TONE_500HZ && wwvh_tone == TONE_500HZ) {
            tone500_station = "WWV+WWVH";
        } else if (wwv_tone == TONE_500HZ) {
            tone500_station = "WWV";
        } else if (wwvh_tone == TONE_500HZ) {
            tone500_station = "WWVH";
        }
        
        snprintf(buf, sizeof(buf), " 500Hz %s %s", tone500_status, tone500_station);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, tone500_color);
        y += line_h;
        
        /* 600 Hz line: show detected state and which station broadcasts it */
        bool tone600_detected = (telem->subcarrier.detected == SUBCAR_600HZ);
        const char* tone600_status = tone600_detected ? "(on) " : "(off)";
        uint32_t tone600_color = tone600_detected ? COLOR_GREEN : COLOR_TEXT_DIM;
        
        /* Which station(s) broadcast 600 Hz this minute? */
        const char* tone600_station = "";
        if (wwv_tone == TONE_600HZ && wwvh_tone == TONE_600HZ) {
            tone600_station = "WWV+WWVH";
        } else if (wwv_tone == TONE_600HZ) {
            tone600_station = "WWV";
        } else if (wwvh_tone == TONE_600HZ) {
            tone600_station = "WWVH";
        }
        
        snprintf(buf, sizeof(buf), " 600Hz %s %s", tone600_status, tone600_station);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, tone600_color);
        y += line_h;
    }
    
    y += 4;
    
    /* === Sync Status === */
    if (telem->sync.valid) {
        uint32_t sync_color;
        switch (telem->sync.state) {
            case SYNC_LOCKED: sync_color = COLOR_GREEN; break;
            case SYNC_TENTATIVE: sync_color = COLOR_ORANGE; break;
            case SYNC_RECOVERING: sync_color = COLOR_YELLOW; break;
            case SYNC_ACQUIRING:
            default: sync_color = COLOR_TEXT_DIM; break;
        }
        
        /* Show state and marker count */
        snprintf(buf, sizeof(buf), "Sync: %s (%d)", 
                 udp_telemetry_sync_state_str(telem->sync.state),
                 telem->sync.marker_num);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, sync_color);
        y += line_h;
        
        /* Show timing details when we have markers */
        if (telem->sync.state >= SYNC_TENTATIVE) {
            snprintf(buf, sizeof(buf), "Delta: %+.1f ms  Int: %.2fs", 
                     telem->sync.delta_ms, telem->sync.interval_sec);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
            y += line_h;
        }
        
        /* Show good intervals count when locked */
        if (telem->sync.state == SYNC_LOCKED && telem->sync.good_intervals > 0) {
            snprintf(buf, sizeof(buf), "Good: %d intervals", telem->sync.good_intervals);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
            y += line_h;
        }
    }
    
    /* === Tone LEDs at bottom === */
    widget_led_draw(&layout->led_tone500, layout->ui);
    widget_led_draw(&layout->led_tone600, layout->ui);
    widget_led_draw(&layout->led_match, layout->ui);
}

/*
 * Sync BCD decoder state to LED
 */
void ui_layout_sync_bcd(ui_layout_t* layout, const bcd_decoder_t* bcd)
{
    if (!layout) return;
    
    if (!bcd) {
        layout->led_bcd_sync.on = false;
        return;
    }
    
    /* LED on when locked */
    bcd_sync_state_t state = bcd_decoder_get_sync_state((bcd_decoder_t*)bcd);
    layout->led_bcd_sync.on = (state == BCD_SYNC_LOCKED);
}

/*
 * Draw BCD time code panel (local fallback decoder)
 * Note: This displays data from the local frame assembler.
 * SNR info not available here - use telemetry panel for signal quality.
 */
void ui_layout_draw_bcd_panel(ui_layout_t* layout, const bcd_decoder_t* bcd)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_bcd, layout->ui);
    
    int x = layout->regions.bcd_panel.x + 8;
    int y = layout->regions.bcd_panel.y + 22;
    int line_h = 16;
    char buf[64];
    
    /* Check if we have a decoder */
    if (!bcd) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No BCD decoder", 
                     x, y, COLOR_TEXT_DIM);
        return;
    }
    
    /* Get comprehensive UI status */
    bcd_ui_status_t status;
    bcd_decoder_get_ui_status((bcd_decoder_t*)bcd, &status);
    
    /* === Sync status with P-marker indicators === */
    {
        /* Sync state */
        uint32_t sync_color;
        const char* sync_str;
        switch (status.sync_state) {
            case BCD_SYNC_LOCKED:
                sync_color = COLOR_GREEN;
                sync_str = "LOCKED";
                break;
            case BCD_SYNC_ACTIVE:
                sync_color = COLOR_ORANGE;
                sync_str = "ACTIVE";
                break;
            default:
                sync_color = COLOR_TEXT_DIM;
                sync_str = "WAITING";
                break;
        }
        
        snprintf(buf, sizeof(buf), "Sync: %s", sync_str);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, sync_color);
        
        /* P-marker dots (7 total) */
        int dot_x = x + 100;
        int dot_y = y + 3;
        for (int i = 0; i < 7; i++) {
            uint32_t dot_color = (i < status.p_markers_found) ? COLOR_GREEN : COLOR_BG_DARK;
            ui_draw_rect(layout->ui, dot_x + i * 10, dot_y, 6, 6, dot_color);
        }
        y += line_h;
        
        /* Frame position */
        if (status.frame_position >= 0) {
            snprintf(buf, sizeof(buf), "Frame: [%02d/59] (%d sym)", 
                     status.frame_position, status.symbols_in_frame);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        } else {
            ui_draw_text(layout->ui, layout->ui->font_small, "Frame: [--/59]", x, y, COLOR_TEXT_DIM);
        }
        y += line_h;
    }
    
    /* === Last symbol === */
    {
        const char* sym_str;
        uint32_t sym_color = COLOR_TEXT;
        switch (status.last_symbol) {
            case BCD_SYMBOL_ZERO:   sym_str = "ZERO"; break;
            case BCD_SYMBOL_ONE:    sym_str = "ONE"; break;
            case BCD_SYMBOL_MARKER: sym_str = "P-MARK"; sym_color = COLOR_ACCENT; break;
            default:                sym_str = "--"; sym_color = COLOR_TEXT_DIM; break;
        }
        snprintf(buf, sizeof(buf), "Last: %s (%.0fms)", sym_str, status.last_symbol_width_ms);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, sym_color);
        y += line_h + 4;
    }
    
    /* === Decoded time === */
    {
        if (status.time_valid) {
            snprintf(buf, sizeof(buf), "%02d:%02d UTC", 
                     status.current_time.hours, status.current_time.minutes);
            ui_draw_text(layout->ui, layout->ui->font_title, buf, x, y, COLOR_ACCENT);
            y += 22;
            
            snprintf(buf, sizeof(buf), "DOY %03d  Year %02d", 
                     status.current_time.day_of_year, status.current_time.year);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
            y += line_h;
            
            if (status.current_time.dut1_sign != 0) {
                snprintf(buf, sizeof(buf), "DUT1: %+.1f s", 
                         status.current_time.dut1_sign * status.current_time.dut1_value);
                ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
                y += line_h;
            }
        } else {
            ui_draw_text(layout->ui, layout->ui->font_small, "--:-- UTC", x, y, COLOR_TEXT_DIM);
            y += line_h;
        }
    }
    
    y += 4;
    
    /* === Statistics === */
    {
        snprintf(buf, sizeof(buf), "Decoded: %u | Failed: %u", 
                 status.frames_decoded, status.frames_failed);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
        y += line_h;
        
        snprintf(buf, sizeof(buf), "Symbols: %u", status.total_symbols);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_GREEN);
    }
    
    /* Sync LED at bottom */
    layout->led_bcd_sync.on = (status.sync_state == BCD_SYNC_LOCKED);
    widget_led_draw(&layout->led_bcd_sync, layout->ui);
}

/*
 * Draw BCD panel from modem telemetry (BCDS packets)
 * This displays data received from the modem's BCD decoder rather than local decoding.
 */
void ui_layout_draw_bcd_panel_from_telem(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_bcd, layout->ui);
    
    int x = layout->regions.bcd_panel.x + 8;
    int y = layout->regions.bcd_panel.y + 22;
    int panel_w = layout->regions.bcd_panel.w - 16;
    int line_h = 16;
    char buf[64];
    
    /* Check if we have telemetry */
    if (!telem || !telem->bcds.valid) {
        ui_draw_text(layout->ui, layout->ui->font_small, "Awaiting BCDS data...", 
                     x, y, COLOR_TEXT_DIM);
        widget_led_draw(&layout->led_bcd_sync, layout->ui);
        return;
    }
    
    const telem_bcds_t* bcds = &telem->bcds;
    
    /* === Signal quality bar (from BCDE envelope data) === */
    if (telem->bcd100.valid) {
        int bar_w = panel_w - 50;
        int bar_h = 10;
        float snr_norm = telem->bcd100.snr_db / 24.0f;
        if (snr_norm < 0) snr_norm = 0;
        if (snr_norm > 1) snr_norm = 1;
        int fill_w = (int)(snr_norm * bar_w);
        
        ui_draw_text(layout->ui, layout->ui->font_small, "Signal:", x, y, COLOR_TEXT_DIM);
        ui_draw_rect(layout->ui, x + 50, y, bar_w, bar_h, COLOR_BG_DARK);
        
        uint32_t bar_color;
        const char* strength_str;
        if (telem->bcd100.snr_db >= 12.0f) {
            bar_color = COLOR_GREEN;
            strength_str = "STRONG";
        } else if (telem->bcd100.snr_db >= 6.0f) {
            bar_color = COLOR_ORANGE;
            strength_str = "GOOD";
        } else if (telem->bcd100.snr_db >= 1.0f) {
            bar_color = COLOR_RED;
            strength_str = "WEAK";
        } else {
            bar_color = COLOR_TEXT_DIM;
            strength_str = "NONE";
        }
        
        if (fill_w > 0) {
            ui_draw_rect(layout->ui, x + 50, y, fill_w, bar_h, bar_color);
        }
        y += line_h;
        
        snprintf(buf, sizeof(buf), "SNR: %.1f dB [%s]", telem->bcd100.snr_db, strength_str);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, bar_color);
        y += line_h + 2;
    }
    
    /* === Sync status === */
    {
        uint32_t sync_color;
        const char* sync_str;
        switch (bcds->sync_state) {
            case BCD_MODEM_SYNC_LOCKED:
                sync_color = COLOR_GREEN;
                sync_str = "LOCKED";
                break;
            case BCD_MODEM_SYNC_CONFIRMING:
                sync_color = COLOR_ORANGE;
                sync_str = "CONFIRMING";
                break;
            default:
                sync_color = COLOR_TEXT_DIM;
                sync_str = "SEARCHING";
                break;
        }
        
        snprintf(buf, sizeof(buf), "Sync: %s", sync_str);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, sync_color);
        
        /* P-marker dots based on frame progress (7 markers at 0,9,19,29,39,49,59) */
        int dot_x = x + 100;
        int dot_y = y + 3;
        int markers_passed = 0;
        if (bcds->frame_pos >= 0) markers_passed = 1;
        if (bcds->frame_pos >= 9) markers_passed = 2;
        if (bcds->frame_pos >= 19) markers_passed = 3;
        if (bcds->frame_pos >= 29) markers_passed = 4;
        if (bcds->frame_pos >= 39) markers_passed = 5;
        if (bcds->frame_pos >= 49) markers_passed = 6;
        if (bcds->frame_pos >= 59) markers_passed = 7;
        
        for (int i = 0; i < 7; i++) {
            uint32_t dot_color = (i < markers_passed) ? COLOR_GREEN : COLOR_BG_DARK;
            ui_draw_rect(layout->ui, dot_x + i * 10, dot_y, 6, 6, dot_color);
        }
        y += line_h;
        
        /* Frame position */
        if (bcds->frame_pos >= 0) {
            snprintf(buf, sizeof(buf), "Frame: [%02d/59]", bcds->frame_pos);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        } else {
            ui_draw_text(layout->ui, layout->ui->font_small, "Frame: [--/59]", x, y, COLOR_TEXT_DIM);
        }
        y += line_h;
    }
    
    /* === Last symbol === */
    {
        const char* sym_str;
        uint32_t sym_color = COLOR_TEXT;
        switch (bcds->last_symbol) {
            case '0': sym_str = "ZERO"; break;
            case '1': sym_str = "ONE"; break;
            case 'P': sym_str = "P-MARK"; sym_color = COLOR_ACCENT; break;
            default:  sym_str = "--"; sym_color = COLOR_TEXT_DIM; break;
        }
        snprintf(buf, sizeof(buf), "Last: %s (%.0fms)", sym_str, bcds->last_symbol_width_ms);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, sym_color);
        y += line_h + 4;
    }
    
    /* === Decoded time === */
    if (bcds->time_valid) {
        snprintf(buf, sizeof(buf), "%02d:%02d UTC", bcds->hours, bcds->minutes);
        ui_draw_text(layout->ui, layout->ui->font_title, buf, x, y, COLOR_ACCENT);
        y += 22;
        
        snprintf(buf, sizeof(buf), "DOY %03d  Year %02d", bcds->day_of_year, bcds->year);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        y += line_h;
        
        if (bcds->dut1_sign != 0) {
            snprintf(buf, sizeof(buf), "DUT1: %+.1f s", bcds->dut1_sign * bcds->dut1_value);
            ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
            y += line_h;
        }
    } else {
        ui_draw_text(layout->ui, layout->ui->font_small, "--:-- UTC", x, y, COLOR_TEXT_DIM);
        y += line_h;
    }
    
    y += 4;
    
    /* === Statistics === */
    snprintf(buf, sizeof(buf), "Decoded: %u | Failed: %u", 
             bcds->decoded_count, bcds->failed_count);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
    y += line_h;
    
    snprintf(buf, sizeof(buf), "Symbols: %u", bcds->symbol_count);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_GREEN);
    
    /* Sync LED at bottom */
    layout->led_bcd_sync.on = (bcds->sync_state == BCD_MODEM_SYNC_LOCKED);
    widget_led_draw(&layout->led_bcd_sync, layout->ui);
}
