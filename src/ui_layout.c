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
    
    /* AFF interval control buttons */
    widget_button_init(&layout->btn_aff_interval_dec, 0, 0, 20, 22, "-");
    widget_button_init(&layout->btn_aff_interval_inc, 0, 0, 20, 22, "+");
    layout->aff_interval_value = AFF_INTERVAL_60S;
    
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
    
    /* Waterfall config buttons */
    widget_button_init(&layout->btn_wf_w_dec, 0, 0, 18, 18, "-");
    widget_button_init(&layout->btn_wf_w_inc, 0, 0, 18, 18, "+");
    widget_button_init(&layout->btn_wf_h_dec, 0, 0, 18, 18, "-");
    widget_button_init(&layout->btn_wf_h_inc, 0, 0, 18, 18, "+");
    
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
    
    /* Tick Correlation panel */
    widget_panel_init(&layout->panel_corr, 0, 0, 0, 0, "Tick Correlation");
    
    /* Sync status panel */
    widget_panel_init(&layout->panel_sync, 0, 0, 0, 0, "Sync Status");
    widget_led_init(&layout->led_sync_locked, 0, 0, LED_RADIUS, COLOR_GREEN, COLOR_TEXT_DIM, "Locked");
    
    /* Minute marker panel */
    widget_panel_init(&layout->panel_mark, 0, 0, 0, 0, "Minute Marker");
    
    /* Initialize debug mode */
    layout->debug_mode = false;
    layout->edit_mode = false;
    
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
 * Recalculate layout - USER ADJUSTED POSITIONS
 * Manually positioned with F2 edit mode, dumped with F3
 */
void ui_layout_recalculate(ui_layout_t* layout)
{
    if (!layout || !layout->ui) return;
    
    int w = layout->ui->window_width;
    int h = layout->ui->window_height;
    
    /* Header (still dynamic for LEDs) */
    layout->regions.header.x = 0;
    layout->regions.header.y = 0;
    layout->regions.header.w = w;
    layout->regions.header.h = HEADER_HEIGHT;
    
    /* Footer */
    layout->regions.footer.x = 0;
    layout->regions.footer.y = h - FOOTER_HEIGHT;
    layout->regions.footer.w = w;
    layout->regions.footer.h = FOOTER_HEIGHT;
    
    /* LEDs in header (right side) */
    int led_y = HEADER_HEIGHT / 2;
    layout->led_connected.x = w - 200;
    layout->led_connected.y = led_y;
    layout->led_streaming.x = w - 130;
    layout->led_streaming.y = led_y;
    layout->led_overload.x = w - 55;
    layout->led_overload.y = led_y;
    
    /* === USER ADJUSTED POSITIONS (from layout_positions.txt) === */
    
    /* Buttons */
    layout->btn_connect.x = 276; layout->btn_connect.y = 161;
    layout->btn_connect.w = 124; layout->btn_connect.h = 26;
    
    layout->btn_start.x = 276; layout->btn_start.y = 195;
    layout->btn_start.w = 58; layout->btn_start.h = 26;
    
    layout->btn_stop.x = 342; layout->btn_stop.y = 195;
    layout->btn_stop.w = 58; layout->btn_stop.h = 26;
    
    layout->btn_freq_down.x = 6; layout->btn_freq_down.y = 97;
    layout->btn_freq_down.w = 40; layout->btn_freq_down.h = 22;
    
    layout->btn_freq_up.x = 50; layout->btn_freq_up.y = 97;
    layout->btn_freq_up.w = 40; layout->btn_freq_up.h = 22;
    
    layout->btn_step_down.x = 102; layout->btn_step_down.y = 97;
    layout->btn_step_down.w = 30; layout->btn_step_down.h = 22;
    
    layout->btn_step_up.x = 136; layout->btn_step_up.y = 97;
    layout->btn_step_up.w = 30; layout->btn_step_up.h = 22;
    
    layout->btn_wwv_2_5.x = 336; layout->btn_wwv_2_5.y = 97;
    layout->btn_wwv_2_5.w = 32; layout->btn_wwv_2_5.h = 22;
    
    layout->btn_wwv_5.x = 371; layout->btn_wwv_5.y = 97;
    layout->btn_wwv_5.w = 28; layout->btn_wwv_5.h = 22;
    
    layout->btn_wwv_10.x = 402; layout->btn_wwv_10.y = 97;
    layout->btn_wwv_10.w = 28; layout->btn_wwv_10.h = 22;
    
    layout->btn_wwv_15.x = 433; layout->btn_wwv_15.y = 97;
    layout->btn_wwv_15.w = 28; layout->btn_wwv_15.h = 22;
    
    layout->btn_wwv_20.x = 464; layout->btn_wwv_20.y = 97;
    layout->btn_wwv_20.w = 28; layout->btn_wwv_20.h = 22;
    
    layout->btn_wwv_25.x = 495; layout->btn_wwv_25.y = 97;
    layout->btn_wwv_25.w = 28; layout->btn_wwv_25.h = 22;
    
    layout->btn_wwv_30.x = 526; layout->btn_wwv_30.y = 97;
    layout->btn_wwv_30.w = 28; layout->btn_wwv_30.h = 22;
    
    layout->btn_server.x = 216; layout->btn_server.y = 123;
    layout->btn_server.w = 52; layout->btn_server.h = 22;
    
    layout->btn_waterfall.x = 272; layout->btn_waterfall.y = 123;
    layout->btn_waterfall.w = 52; layout->btn_waterfall.h = 22;
    
    /* Waterfall config buttons (to right of Wfall button) */
    int wf_cfg_x = layout->btn_waterfall.x + layout->btn_waterfall.w + 8;
    int wf_cfg_y = layout->btn_waterfall.y + 2;
    
    layout->btn_wf_w_dec.x = wf_cfg_x;
    layout->btn_wf_w_dec.y = wf_cfg_y;
    layout->btn_wf_w_dec.w = 18;
    layout->btn_wf_w_dec.h = 18;
    
    layout->btn_wf_w_inc.x = wf_cfg_x + 58;
    layout->btn_wf_w_inc.y = wf_cfg_y;
    layout->btn_wf_w_inc.w = 18;
    layout->btn_wf_w_inc.h = 18;
    
    layout->btn_wf_h_dec.x = wf_cfg_x + 86;
    layout->btn_wf_h_dec.y = wf_cfg_y;
    layout->btn_wf_h_dec.w = 18;
    layout->btn_wf_h_dec.h = 18;
    
    layout->btn_wf_h_inc.x = wf_cfg_x + 144;
    layout->btn_wf_h_inc.y = wf_cfg_y;
    layout->btn_wf_h_inc.w = 18;
    layout->btn_wf_h_inc.h = 18;
    
    /* Preset buttons */
    layout->btn_preset[0].x = 6; layout->btn_preset[0].y = 123;
    layout->btn_preset[0].w = 36; layout->btn_preset[0].h = 22;
    
    layout->btn_preset[1].x = 46; layout->btn_preset[1].y = 123;
    layout->btn_preset[1].w = 36; layout->btn_preset[1].h = 22;
    
    layout->btn_preset[2].x = 86; layout->btn_preset[2].y = 123;
    layout->btn_preset[2].w = 36; layout->btn_preset[2].h = 22;
    
    layout->btn_preset[3].x = 126; layout->btn_preset[3].y = 123;
    layout->btn_preset[3].w = 36; layout->btn_preset[3].h = 22;
    
    layout->btn_preset[4].x = 166; layout->btn_preset[4].y = 123;
    layout->btn_preset[4].w = 36; layout->btn_preset[4].h = 22;
    
    /* Sliders */
    layout->slider_gain.x = 24; layout->slider_gain.y = 186;
    layout->slider_gain.w = 32; layout->slider_gain.h = 251;
    
    layout->slider_lna.x = 66; layout->slider_lna.y = 186;
    layout->slider_lna.w = 32; layout->slider_lna.h = 251;
    
    /* Combos */
    layout->combo_agc.x = 120; layout->combo_agc.y = 183;
    layout->combo_agc.w = 134; layout->combo_agc.h = 22;
    
    layout->combo_srate.x = 120; layout->combo_srate.y = 229;
    layout->combo_srate.w = 134; layout->combo_srate.h = 22;
    
    layout->combo_bw.x = 120; layout->combo_bw.y = 275;
    layout->combo_bw.w = 134; layout->combo_bw.h = 22;
    
    layout->combo_antenna.x = 120; layout->combo_antenna.y = 321;
    layout->combo_antenna.w = 134; layout->combo_antenna.h = 22;
    
    /* Toggles */
    layout->toggle_biast.x = 276; layout->toggle_biast.y = 233;
    layout->toggle_notch.x = 276; layout->toggle_notch.y = 263;
    layout->toggle_aff.x = 276; layout->toggle_aff.y = 293;
    
    /* Panels */
    layout->panel_freq.x = 6; layout->panel_freq.y = 36;
    layout->panel_freq.w = 708; layout->panel_freq.h = 55;
    
    layout->panel_gain.x = 6; layout->panel_gain.y = 151;
    layout->panel_gain.w = 100; layout->panel_gain.h = 301;
    
    layout->panel_config.x = 112; layout->panel_config.y = 151;
    layout->panel_config.w = 150; layout->panel_config.h = 301;
    
    layout->panel_wwv.x = 414; layout->panel_wwv.y = 151;
    layout->panel_wwv.w = 300; layout->panel_wwv.h = 250;
    
    layout->panel_bcd.x = 414; layout->panel_bcd.y = 407;
    layout->panel_bcd.w = 300; layout->panel_bcd.h = 150;
    
    layout->panel_corr.x = 414; layout->panel_corr.y = 563;
    layout->panel_corr.w = 300; layout->panel_corr.h = 120;
    
    layout->panel_sync.x = 414; layout->panel_sync.y = 689;
    layout->panel_sync.w = 300; layout->panel_sync.h = 140;
    
    layout->panel_mark.x = 414; layout->panel_mark.y = 835;
    layout->panel_mark.w = 300; layout->panel_mark.h = 80;
    
    /* Telemetry panel (bottom left, below gain panel) */
    layout->panel_telemetry.x = 6; layout->panel_telemetry.y = 458;
    layout->panel_telemetry.w = 400; layout->panel_telemetry.h = 250;
    
    /* Telemetry tab buttons (across top of panel) */
    int tab_w = 96;  /* 4 tabs * 96 = 384, leaves room for padding */
    int tab_h = 22;
    int tab_x = layout->panel_telemetry.x + 8;
    int tab_y = layout->panel_telemetry.y + 20;
    for (int i = 0; i < 4; i++) {
        layout->tab_telemetry[i].x = tab_x + (i * tab_w);
        layout->tab_telemetry[i].y = tab_y;
        layout->tab_telemetry[i].w = tab_w - 2;
        layout->tab_telemetry[i].h = tab_h;
    }
    
    /* Frequency display */
    layout->freq_display.x = 0; layout->freq_display.y = 67;
    layout->freq_display.w = 668; layout->freq_display.h = 35;
    
    /* DC offset dot (calculated from freq display) */
    layout->offset_dot.x = layout->freq_display.x + layout->freq_display.w + 8;
    layout->offset_dot.y = layout->freq_display.y + (layout->freq_display.h / 2) - 5;
    layout->offset_dot.w = 10;
    layout->offset_dot.h = 10;
    
    /* Regions (match panel positions) */
    layout->regions.freq_area = (SDL_Rect){6, 36, 708, 55};
    layout->regions.gain_panel = (SDL_Rect){6, 151, 100, 301};
    layout->regions.wwv_panel = (SDL_Rect){414, 151, 300, 250};
    layout->regions.bcd_panel = (SDL_Rect){414, 407, 300, 150};
    
    /* WWV indicator LEDs (inside panel) */
    layout->led_tone500.x = 434;
    layout->led_tone500.y = layout->panel_wwv.y + layout->panel_wwv.h - 30;
    layout->led_tone600.x = 484;
    layout->led_tone600.y = layout->panel_wwv.y + layout->panel_wwv.h - 30;
    layout->led_match.x = 534;
    layout->led_match.y = layout->panel_wwv.y + layout->panel_wwv.h - 30;
    
    /* BCD sync LED (inside panel) */
    layout->led_bcd_sync.x = 434;
    layout->led_bcd_sync.y = layout->panel_bcd.y + layout->panel_bcd.h - 30;
    
    /* AFF interval control buttons (in config panel, below antenna combo) */
    layout->btn_aff_interval_dec.x = layout->combo_antenna.x;
    layout->btn_aff_interval_dec.y = layout->combo_antenna.y + layout->combo_antenna.h + 30;
    layout->btn_aff_interval_dec.w = 20;
    layout->btn_aff_interval_dec.h = 22;
    
    layout->btn_aff_interval_inc.x = layout->btn_aff_interval_dec.x + 114;
    layout->btn_aff_interval_inc.y = layout->btn_aff_interval_dec.y;
    layout->btn_aff_interval_inc.w = 20;
    layout->btn_aff_interval_inc.h = 22;
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
    
    /* Sync waterfall config for display */
    layout->wf_width = pm->waterfall_cfg.width;
    layout->wf_height = pm->waterfall_cfg.height;
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
    
    /* Skip normal UI processing in edit mode */
    if (layout->edit_mode) {
        return;  /* Let edit mode handle mouse events */
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
    
    if (widget_button_update(&layout->btn_aff_interval_dec, mouse)) {
        actions->aff_interval_dec = true;
    }
    
    if (widget_button_update(&layout->btn_aff_interval_inc, mouse)) {
        actions->aff_interval_inc = true;
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
    
    /* Update telemetry tab buttons */
    for (int i = 0; i < 4; i++) {
        if (widget_button_update(&layout->tab_telemetry[i], mouse)) {
            layout->active_telemetry_tab = i;
        }
    }
    
    /* Update external process buttons */
    if (widget_button_update(&layout->btn_server, mouse)) {
        actions->server_toggled = true;
    }
    if (widget_button_update(&layout->btn_waterfall, mouse)) {
        actions->waterfall_toggled = true;
    }
    
    /* Update waterfall config buttons */
    if (widget_button_update(&layout->btn_wf_w_dec, mouse)) {
        actions->wf_w_dec = true;
    }
    if (widget_button_update(&layout->btn_wf_w_inc, mouse)) {
        actions->wf_w_inc = true;
    }
    if (widget_button_update(&layout->btn_wf_h_dec, mouse)) {
        actions->wf_h_dec = true;
    }
    if (widget_button_update(&layout->btn_wf_h_inc, mouse)) {
        actions->wf_h_inc = true;
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
    
    /* Draw waterfall config: W: - [value] + | H: - [value] + */
    int wf_lbl_y = layout->btn_wf_w_dec.y + 3;
    char wf_w_str[16], wf_h_str[16];
    snprintf(wf_w_str, sizeof(wf_w_str), "%d", layout->wf_width);
    snprintf(wf_h_str, sizeof(wf_h_str), "%d", layout->wf_height);
    
    widget_button_draw(&layout->btn_wf_w_dec, layout->ui);
    ui_draw_text(layout->ui, layout->ui->font_small, wf_w_str,
                 layout->btn_wf_w_dec.x + 20, wf_lbl_y, COLOR_TEXT);
    widget_button_draw(&layout->btn_wf_w_inc, layout->ui);
    
    ui_draw_text(layout->ui, layout->ui->font_small, "x",
                 layout->btn_wf_w_inc.x + 22, wf_lbl_y, COLOR_TEXT_DIM);
    
    widget_button_draw(&layout->btn_wf_h_dec, layout->ui);
    ui_draw_text(layout->ui, layout->ui->font_small, wf_h_str,
                 layout->btn_wf_h_dec.x + 20, wf_lbl_y, COLOR_TEXT);
    widget_button_draw(&layout->btn_wf_h_inc, layout->ui);
    
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
    
    /* Draw AFF interval control (label + buttons in config panel) */
    static const char* interval_labels[] = {"30s", "45s", "60s", "90s", "120s"};
    int interval_idx = layout->aff_interval_value;
    if (interval_idx < 0 || interval_idx >= AFF_INTERVAL_COUNT) interval_idx = AFF_INTERVAL_60S;
    
    char aff_label[32];
    snprintf(aff_label, sizeof(aff_label), "AFF Int: %s", interval_labels[interval_idx]);
    ui_draw_text(layout->ui, layout->ui->font_small, aff_label,
                 layout->btn_aff_interval_dec.x,
                 layout->btn_aff_interval_dec.y - 16,
                 COLOR_TEXT);
    
    widget_button_draw(&layout->btn_aff_interval_dec, layout->ui);
    widget_button_draw(&layout->btn_aff_interval_inc, layout->ui);
    
    /* Draw telemetry panel (bottom left) */
    ui_layout_draw_telemetry_panel(layout);
    
    /* Draw footer */
    ui_layout_draw_footer(layout, state);
    
    /* Draw edit mode banner (if active) */
    if (layout->edit_mode) {
        int banner_h = 40;
        int banner_y = (layout->ui->window_height - banner_h) / 2;
        
        /* Semi-transparent background */
        ui_draw_rect(layout->ui, 0, banner_y, layout->ui->window_width, banner_h, 0x000000DD);
        
        /* Border */
        ui_draw_rect_outline(layout->ui, 0, banner_y, layout->ui->window_width, banner_h, COLOR_ACCENT);
        ui_draw_rect_outline(layout->ui, 1, banner_y + 1, layout->ui->window_width - 2, banner_h - 2, COLOR_ACCENT);
        
        /* Text */
        const char* msg = "EDIT MODE: Click and drag widgets to reposition | F3 = Save positions | F2 = Exit";
        ui_draw_text_centered(layout->ui, layout->ui->font_large, msg, 
                             0, banner_y + 10, layout->ui->window_width, COLOR_ACCENT);
    }
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

/*
 * Draw Tick Correlation panel
 */
void ui_layout_draw_corr_panel(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_corr, layout->ui);
    
    int x = layout->panel_corr.x + 8;
    int y = layout->panel_corr.y + 22;
    int line_h = 14;
    char buf[128];
    
    if (!telem || !telem->corr.valid) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No correlation data", 
                     x, y, COLOR_TEXT_DIM);
        return;
    }
    
    const telem_corr_t* corr = &telem->corr;
    
    /* Tick number and expected event */
    snprintf(buf, sizeof(buf), "Tick #%d: %s", corr->tick_num, corr->expected);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
    y += line_h;
    
    /* Chain info */
    snprintf(buf, sizeof(buf), "Chain: #%d len=%d", corr->chain_id, corr->chain_len);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_GREEN);
    y += line_h;
    
    /* Interval and drift */
    snprintf(buf, sizeof(buf), "Interval: %.1fms (avg %.1fms)", 
             corr->interval_ms, corr->avg_interval_ms);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
    y += line_h;
    
    /* Drift with color coding */
    uint32_t drift_color = COLOR_GREEN;
    float abs_drift = corr->drift_ms < 0 ? -corr->drift_ms : corr->drift_ms;
    if (abs_drift > 50.0f) drift_color = COLOR_RED;
    else if (abs_drift > 20.0f) drift_color = COLOR_ORANGE;
    
    snprintf(buf, sizeof(buf), "Drift: %.1f ms", corr->drift_ms);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, drift_color);
    y += line_h;
    
    /* Correlation ratio */
    snprintf(buf, sizeof(buf), "Corr: %.1f (peak %.3f)", corr->corr_ratio, corr->corr_peak);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
}

/*
 * Helper: Get sync state name
 */
static const char* get_sync_state_name(sync_state_t state)
{
    switch (state) {
        case SYNC_ACQUIRING: return "ACQUIRING";
        case SYNC_TENTATIVE: return "TENTATIVE";
        case SYNC_LOCKED:    return "LOCKED";
        case SYNC_RECOVERING: return "RECOVERING";
        default: return "UNKNOWN";
    }
}

/*
 * Draw Sync Status panel with two sub-frames
 */
void ui_layout_draw_sync_panel(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout || !layout->ui) return;
    
    /* Draw main panel background */
    widget_panel_draw(&layout->panel_sync, layout->ui);
    
    int panel_x = layout->panel_sync.x;
    int panel_y = layout->panel_sync.y;
    int panel_w = layout->panel_sync.w;
    int line_h = 14;
    char buf[128];
    
    /* Sub-frame 1: Marker Confirmation (top half) */
    int frame1_y = panel_y + 20;
    int frame1_h = 55;
    
    /* Draw sub-frame border */
    SDL_Rect frame1 = { panel_x + 4, frame1_y, panel_w - 8, frame1_h };
    SDL_SetRenderDrawColor(layout->ui->renderer, 60, 60, 70, 255);
    SDL_RenderDrawRect(layout->ui->renderer, &frame1);
    
    /* Sub-frame 1 title */
    ui_draw_text(layout->ui, layout->ui->font_small, "Marker Confirmation", 
                 panel_x + 8, frame1_y + 2, COLOR_ACCENT);
    
    int x = panel_x + 8;
    int y = frame1_y + 16;
    
    if (!telem || !telem->sync.valid) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No sync data", 
                     x, y, COLOR_TEXT_DIM);
    } else {
        const telem_sync_t* sync = &telem->sync;
        
        /* State with color coding */
        uint32_t state_color = COLOR_TEXT_DIM;
        if (sync->state == SYNC_LOCKED) state_color = COLOR_GREEN;
        else if (sync->state == SYNC_TENTATIVE) state_color = COLOR_ORANGE;
        else if (sync->state == SYNC_RECOVERING) state_color = COLOR_RED;
        
        snprintf(buf, sizeof(buf), "State: %s  Markers: %d", 
                 get_sync_state_name(sync->state), sync->marker_num);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, state_color);
        y += line_h;
        
        /* Interval and delta */
        snprintf(buf, sizeof(buf), "Interval: %.1fs  Delta: %.1fms", 
                 sync->interval_sec, sync->delta_ms);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT_DIM);
        y += line_h;
        
        /* Good intervals */
        snprintf(buf, sizeof(buf), "Good intervals: %d", sync->good_intervals);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, 
                     sync->good_intervals >= 2 ? COLOR_GREEN : COLOR_TEXT_DIM);
    }
    
    /* Sub-frame 2: State Transitions (bottom half) */
    int frame2_y = frame1_y + frame1_h + 4;
    int frame2_h = 50;
    
    /* Draw sub-frame border */
    SDL_Rect frame2 = { panel_x + 4, frame2_y, panel_w - 8, frame2_h };
    SDL_SetRenderDrawColor(layout->ui->renderer, 60, 60, 70, 255);
    SDL_RenderDrawRect(layout->ui->renderer, &frame2);
    
    /* Sub-frame 2 title */
    ui_draw_text(layout->ui, layout->ui->font_small, "State Transition", 
                 panel_x + 8, frame2_y + 2, COLOR_ACCENT);
    
    x = panel_x + 8;
    y = frame2_y + 16;
    
    if (!telem || !telem->sync.state_changed) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No transition yet", 
                     x, y, COLOR_TEXT_DIM);
    } else {
        const telem_sync_t* sync = &telem->sync;
        
        /* Transition arrow */
        snprintf(buf, sizeof(buf), "%s -> %s", 
                 get_sync_state_name(sync->old_state),
                 get_sync_state_name(sync->state));
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_TEXT);
        y += line_h;
        
        /* Confidence */
        uint32_t conf_color = COLOR_GREEN;
        if (sync->confidence < 0.5f) conf_color = COLOR_RED;
        else if (sync->confidence < 0.8f) conf_color = COLOR_ORANGE;
        
        snprintf(buf, sizeof(buf), "Confidence: %.0f%%", sync->confidence * 100.0f);
        ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, conf_color);
    }
    
    /* Sync locked LED at bottom of panel */
    layout->led_sync_locked.x = panel_x + panel_w - 60;
    layout->led_sync_locked.y = panel_y + layout->panel_sync.h - 18;
    layout->led_sync_locked.on = (telem && telem->sync.valid && telem->sync.state == SYNC_LOCKED);
    widget_led_draw(&layout->led_sync_locked, layout->ui);
}

/*
 * Draw Minute Marker panel
 */
void ui_layout_draw_mark_panel(ui_layout_t* layout, const udp_telemetry_t* telem)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_mark, layout->ui);
    
    int x = layout->panel_mark.x + 8;
    int y = layout->panel_mark.y + 22;
    int line_h = 14;
    char buf[128];
    
    if (!telem || !telem->marker.valid) {
        ui_draw_text(layout->ui, layout->ui->font_small, "No marker detected", 
                     x, y, COLOR_TEXT_DIM);
        return;
    }
    
    const telem_marker_t* mark = &telem->marker;
    
    /* Marker number and duration */
    snprintf(buf, sizeof(buf), "%s  Duration: %.0f ms", mark->marker_num, mark->duration_ms);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, COLOR_GREEN);
    y += line_h;
    
    /* SNR with color coding */
    uint32_t snr_color = COLOR_GREEN;
    if (mark->snr_db < 10.0f) snr_color = COLOR_RED;
    else if (mark->snr_db < 15.0f) snr_color = COLOR_ORANGE;
    
    snprintf(buf, sizeof(buf), "SNR: %.1f dB  Energy: %.4f", mark->snr_db, mark->accum_energy);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, snr_color);
    y += line_h;
    
    /* Confidence with color coding */
    uint32_t conf_color = COLOR_TEXT_DIM;
    if (strcmp(mark->confidence, "HIGH") == 0) conf_color = COLOR_GREEN;
    else if (strcmp(mark->confidence, "MED") == 0) conf_color = COLOR_ORANGE;
    else if (strcmp(mark->confidence, "LOW") == 0) conf_color = COLOR_RED;
    
    snprintf(buf, sizeof(buf), "Confidence: %s", mark->confidence);
    ui_draw_text(layout->ui, layout->ui->font_small, buf, x, y, conf_color);
}
/*
 * Draw Telemetry panel with tabs (bottom left)
 */
void ui_layout_draw_telemetry_panel(ui_layout_t* layout)
{
    if (!layout || !layout->ui) return;
    
    /* Draw panel background */
    widget_panel_draw(&layout->panel_telemetry, layout->ui);
    
    /* Draw tab buttons - highlight active tab */
    for (int i = 0; i < 4; i++) {
        widget_button_t* tab = &layout->tab_telemetry[i];
        
        /* Set toggled state for active tab */
        tab->toggled = (i == layout->active_telemetry_tab);
        
        /* Draw the tab */
        widget_button_draw(tab, layout->ui);
    }
    
    /* Draw content area placeholder */
    int content_x = layout->panel_telemetry.x + 8;
    int content_y = layout->tab_telemetry[0].y + layout->tab_telemetry[0].h + 4;
    int content_w = layout->panel_telemetry.w - 16;
    int content_h = layout->panel_telemetry.h - (content_y - layout->panel_telemetry.y) - 8;
    
    /* Draw content border */
    SDL_Rect content_rect = { content_x, content_y, content_w, content_h };
    SDL_SetRenderDrawColor(layout->ui->renderer, 40, 40, 50, 255);
    SDL_RenderFillRect(layout->ui->renderer, &content_rect);
    SDL_SetRenderDrawColor(layout->ui->renderer, 80, 80, 90, 255);
    SDL_RenderDrawRect(layout->ui->renderer, &content_rect);
    
    /* Draw placeholder text based on active tab */
    const char* tab_labels[] = {
        "Channel Quality Data",
        "Carrier Tracking Data",
        "Tone Tracking Data (500/600 Hz)",
        "Debug Console Output"
    };
    
    int text_y = content_y + 10;
    ui_draw_text(layout->ui, layout->ui->font_small, tab_labels[layout->active_telemetry_tab],
                 content_x + 8, text_y, COLOR_TEXT_DIM);
    text_y += 18;
    ui_draw_text(layout->ui, layout->ui->font_small, "(Implementation pending)",
                 content_x + 8, text_y, COLOR_TEXT_DIM);
}