/**
 * Phoenix SDR Controller - UI Layout Implementation
 * 
 * Main application layout and component management.
 */

#include "ui_layout.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Layout constants */
#define HEADER_HEIGHT 40
#define FOOTER_HEIGHT 28
#define PANEL_PADDING 10
#define WIDGET_SPACING 8
#define BUTTON_HEIGHT 32
#define SLIDER_WIDTH 40
#define SLIDER_HEIGHT 150
#define COMBO_HEIGHT 28
#define LED_RADIUS 6

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
    
    /* WWV frequency shortcut buttons */
    widget_button_init(&layout->btn_wwv_2_5, 0, 0, 50, 24, "2.5");
    widget_button_init(&layout->btn_wwv_5, 0, 0, 40, 24, "5");
    widget_button_init(&layout->btn_wwv_10, 0, 0, 40, 24, "10");
    widget_button_init(&layout->btn_wwv_15, 0, 0, 40, 24, "15");
    widget_button_init(&layout->btn_wwv_20, 0, 0, 40, 24, "20");
    widget_button_init(&layout->btn_wwv_25, 0, 0, 40, 24, "25");
    widget_button_init(&layout->btn_wwv_30, 0, 0, 40, 24, "30");
    
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
 * Recalculate layout (call on window resize)
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
    
    /* Content area */
    int content_y = HEADER_HEIGHT + PANEL_PADDING;
    int content_h = h - HEADER_HEIGHT - FOOTER_HEIGHT - PANEL_PADDING * 2;
    
    /* Left column: Frequency and Tuning (width ~450) */
    int left_w = 450;
    
    /* Frequency panel */
    layout->regions.freq_area.x = PANEL_PADDING;
    layout->regions.freq_area.y = content_y;
    layout->regions.freq_area.w = left_w;
    layout->regions.freq_area.h = 140;
    
    layout->panel_freq.x = layout->regions.freq_area.x;
    layout->panel_freq.y = layout->regions.freq_area.y;
    layout->panel_freq.w = layout->regions.freq_area.w;
    layout->panel_freq.h = layout->regions.freq_area.h;
    
    /* Frequency display inside panel */
    layout->freq_display.x = layout->panel_freq.x + 20;
    layout->freq_display.y = layout->panel_freq.y + 34;
    layout->freq_display.w = layout->panel_freq.w - 50;  /* Leave room for offset dot */
    layout->freq_display.h = 80;
    
    /* DC offset dot (next to frequency display) */
    layout->offset_dot.x = layout->freq_display.x + layout->freq_display.w + 5;
    layout->offset_dot.y = layout->freq_display.y + (layout->freq_display.h / 2) - 6;
    layout->offset_dot.w = 12;
    layout->offset_dot.h = 12;
    
    /* Tuning panel (below frequency) */
    layout->regions.tuning_panel.x = PANEL_PADDING;
    layout->regions.tuning_panel.y = layout->regions.freq_area.y + layout->regions.freq_area.h + PANEL_PADDING;
    layout->regions.tuning_panel.w = left_w;
    layout->regions.tuning_panel.h = 60;
    
    /* Tuning buttons */
    int tuning_y = layout->regions.tuning_panel.y + 15;
    layout->btn_freq_down.x = layout->regions.tuning_panel.x + 10;
    layout->btn_freq_down.y = tuning_y;
    
    layout->btn_freq_up.x = layout->btn_freq_down.x + 60;
    layout->btn_freq_up.y = tuning_y;
    
    layout->btn_step_down.x = layout->btn_freq_up.x + 80;
    layout->btn_step_down.y = tuning_y;
    
    layout->btn_step_up.x = layout->btn_step_down.x + 50;
    layout->btn_step_up.y = tuning_y;
    
    /* WWV shortcut buttons (below tuning panel) */
    int wwv_y = layout->regions.tuning_panel.y + layout->regions.tuning_panel.h + 10;
    int wwv_x = PANEL_PADDING + 10;
    int wwv_spacing = 5;
    
    layout->btn_wwv_2_5.x = wwv_x;
    layout->btn_wwv_2_5.y = wwv_y;
    wwv_x += layout->btn_wwv_2_5.w + wwv_spacing;
    
    layout->btn_wwv_5.x = wwv_x;
    layout->btn_wwv_5.y = wwv_y;
    wwv_x += layout->btn_wwv_5.w + wwv_spacing;
    
    layout->btn_wwv_10.x = wwv_x;
    layout->btn_wwv_10.y = wwv_y;
    wwv_x += layout->btn_wwv_10.w + wwv_spacing;
    
    layout->btn_wwv_15.x = wwv_x;
    layout->btn_wwv_15.y = wwv_y;
    wwv_x += layout->btn_wwv_15.w + wwv_spacing;
    
    layout->btn_wwv_20.x = wwv_x;
    layout->btn_wwv_20.y = wwv_y;
    wwv_x += layout->btn_wwv_20.w + wwv_spacing;
    
    layout->btn_wwv_25.x = wwv_x;
    layout->btn_wwv_25.y = wwv_y;
    wwv_x += layout->btn_wwv_25.w + wwv_spacing;
    
    layout->btn_wwv_30.x = wwv_x;
    layout->btn_wwv_30.y = wwv_y;
    
    /* Middle column: Gain (width ~120) */
    int gain_x = left_w + PANEL_PADDING * 2;
    int gain_w = 120;
    
    layout->regions.gain_panel.x = gain_x;
    layout->regions.gain_panel.y = content_y;
    layout->regions.gain_panel.w = gain_w;
    layout->regions.gain_panel.h = 220;
    
    layout->panel_gain.x = layout->regions.gain_panel.x;
    layout->panel_gain.y = layout->regions.gain_panel.y;
    layout->panel_gain.w = layout->regions.gain_panel.w;
    layout->panel_gain.h = layout->regions.gain_panel.h;
    
    /* Gain sliders */
    layout->slider_gain.x = gain_x + 15;
    layout->slider_gain.y = content_y + 50;
    
    layout->slider_lna.x = gain_x + 70;
    layout->slider_lna.y = content_y + 50;
    
    /* Right column: Config (width ~180) */
    int config_x = gain_x + gain_w + PANEL_PADDING;
    int config_w = 180;
    
    layout->regions.config_panel.x = config_x;
    layout->regions.config_panel.y = content_y;
    layout->regions.config_panel.w = config_w;
    layout->regions.config_panel.h = 280;
    
    layout->panel_config.x = layout->regions.config_panel.x;
    layout->panel_config.y = layout->regions.config_panel.y;
    layout->panel_config.w = layout->regions.config_panel.w;
    layout->panel_config.h = layout->regions.config_panel.h;
    
    /* Config combos */
    int combo_x = config_x + 10;
    int combo_y = content_y + 40;
    
    layout->combo_agc.x = combo_x;
    layout->combo_agc.y = combo_y;
    layout->combo_agc.w = config_w - 20;
    
    layout->combo_srate.x = combo_x;
    layout->combo_srate.y = combo_y + 55;
    layout->combo_srate.w = config_w - 20;
    
    layout->combo_bw.x = combo_x;
    layout->combo_bw.y = combo_y + 110;
    layout->combo_bw.w = config_w - 20;
    
    layout->combo_antenna.x = combo_x;
    layout->combo_antenna.y = combo_y + 165;
    layout->combo_antenna.w = config_w - 20;
    
    /* Status panel (rightmost) */
    int status_x = config_x + config_w + PANEL_PADDING;
    int status_w = w - status_x - PANEL_PADDING;
    if (status_w < 150) status_w = 150;
    
    layout->regions.status_panel.x = status_x;
    layout->regions.status_panel.y = content_y;
    layout->regions.status_panel.w = status_w;
    layout->regions.status_panel.h = 200;
    
    /* Connect and streaming buttons */
    layout->btn_connect.x = status_x + 10;
    layout->btn_connect.y = content_y + 10;
    layout->btn_connect.w = status_w - 20;
    
    layout->btn_start.x = status_x + 10;
    layout->btn_start.y = content_y + 50;
    layout->btn_start.w = (status_w - 30) / 2;
    
    layout->btn_stop.x = layout->btn_start.x + layout->btn_start.w + 10;
    layout->btn_stop.y = content_y + 50;
    layout->btn_stop.w = layout->btn_start.w;
    
    /* LEDs */
    layout->led_connected.x = status_x + 20;
    layout->led_connected.y = content_y + 100;
    
    layout->led_streaming.x = status_x + 20;
    layout->led_streaming.y = content_y + 125;
    
    layout->led_overload.x = status_x + 20;
    layout->led_overload.y = content_y + 150;
    
    /* Toggles */
    layout->toggle_biast.x = status_x + 10;
    layout->toggle_biast.y = content_y + 175;
    
    layout->toggle_notch.x = status_x + 10;
    layout->toggle_notch.y = content_y + 205;
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
 * Update widgets and get actions
 */
void ui_layout_update(ui_layout_t* layout, const mouse_state_t* mouse,
                      SDL_Event* event, ui_actions_t* actions)
{
    if (!layout || !mouse || !actions) return;
    
    /* Clear actions */
    memset(actions, 0, sizeof(ui_actions_t));
    
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
    
    /* Draw step label */
    if (state) {
        char step_str[32];
        snprintf(step_str, sizeof(step_str), "Step: %s", app_get_step_string(state->tuning_step));
        ui_draw_text(layout->ui, layout->ui->font_normal, step_str,
                    layout->btn_step_up.x + 60, layout->btn_step_up.y + 8, COLOR_TEXT);
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
    
    /* Draw LEDs */
    widget_led_draw(&layout->led_connected, layout->ui);
    widget_led_draw(&layout->led_streaming, layout->ui);
    widget_led_draw(&layout->led_overload, layout->ui);
    
    /* Draw toggles */
    widget_toggle_draw(&layout->toggle_biast, layout->ui);
    widget_toggle_draw(&layout->toggle_notch, layout->ui);
    
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
    ui_draw_text(layout->ui, layout->ui->font_title, APP_NAME, 10, 8, COLOR_ACCENT);
    
    /* Draw version */
    char version_str[32];
    snprintf(version_str, sizeof(version_str), "v%s", APP_VERSION);
    ui_draw_text_right(layout->ui, layout->ui->font_small, version_str,
                       0, 14, layout->regions.header.w - 10, COLOR_TEXT_DIM);
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
