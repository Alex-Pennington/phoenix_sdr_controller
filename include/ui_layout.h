/**
 * Phoenix SDR Controller - UI Layout
 * 
 * Main application layout and component management.
 */

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "ui_core.h"
#include "ui_widgets.h"
#include "app_state.h"
#include "sdr_protocol.h"

/* Layout regions */
typedef struct {
    /* Header bar */
    SDL_Rect header;
    
    /* Main frequency display area */
    SDL_Rect freq_area;
    
    /* Control panels */
    SDL_Rect gain_panel;
    SDL_Rect tuning_panel;
    SDL_Rect config_panel;
    SDL_Rect status_panel;
    
    /* Footer/status bar */
    SDL_Rect footer;
} layout_regions_t;

/* Main layout context */
typedef struct {
    ui_core_t* ui;
    layout_regions_t regions;
    
    /* Widgets */
    widget_freq_display_t freq_display;
    
    /* Control buttons */
    widget_button_t btn_connect;
    widget_button_t btn_start;
    widget_button_t btn_stop;
    
    /* Step buttons */
    widget_button_t btn_step_up;
    widget_button_t btn_step_down;
    
    /* Frequency buttons */
    widget_button_t btn_freq_up;
    widget_button_t btn_freq_down;
    
    /* Gain controls */
    widget_slider_t slider_gain;
    widget_slider_t slider_lna;
    
    /* Configuration combos */
    widget_combo_t combo_agc;
    widget_combo_t combo_srate;
    widget_combo_t combo_bw;
    widget_combo_t combo_antenna;
    
    /* Toggles */
    widget_toggle_t toggle_biast;
    widget_toggle_t toggle_notch;
    
    /* DC offset indicator (clickable dot next to freq display) */
    SDL_Rect offset_dot;
    
    /* WWV frequency shortcut buttons */
    widget_button_t btn_wwv_2_5;
    widget_button_t btn_wwv_5;
    widget_button_t btn_wwv_10;
    widget_button_t btn_wwv_15;
    widget_button_t btn_wwv_20;
    widget_button_t btn_wwv_25;
    widget_button_t btn_wwv_30;
    
    /* Status LEDs */
    widget_led_t led_connected;
    widget_led_t led_streaming;
    widget_led_t led_overload;
    
    /* Panels */
    widget_panel_t panel_freq;
    widget_panel_t panel_gain;
    widget_panel_t panel_config;
    
} ui_layout_t;

/* Action results from UI update */
typedef struct {
    bool connect_clicked;
    bool disconnect_clicked;
    bool start_clicked;
    bool stop_clicked;
    bool freq_changed;
    int64_t new_frequency;
    bool gain_changed;
    int new_gain;
    bool lna_changed;
    int new_lna;
    bool agc_changed;
    agc_mode_t new_agc;
    bool srate_changed;
    int new_srate;
    bool bw_changed;
    int new_bw;
    bool antenna_changed;
    antenna_port_t new_antenna;
    bool biast_changed;
    bool new_biast;
    bool notch_changed;
    bool new_notch;
    bool step_up;
    bool step_down;
    bool freq_up;
    bool freq_down;
    bool dc_offset_toggled;
    bool wwv_clicked;
    int64_t wwv_frequency;  /* Which WWV freq was clicked */
} ui_actions_t;

/* Create layout */
ui_layout_t* ui_layout_create(ui_core_t* ui);

/* Destroy layout */
void ui_layout_destroy(ui_layout_t* layout);

/* Recalculate layout (call on window resize) */
void ui_layout_recalculate(ui_layout_t* layout);

/* Update layout from app state */
void ui_layout_sync_state(ui_layout_t* layout, const app_state_t* state);

/* Update widgets and get actions */
void ui_layout_update(ui_layout_t* layout, const mouse_state_t* mouse, 
                      SDL_Event* event, ui_actions_t* actions);

/* Draw the complete layout */
void ui_layout_draw(ui_layout_t* layout, const app_state_t* state);

/* Draw header bar */
void ui_layout_draw_header(ui_layout_t* layout, const app_state_t* state);

/* Draw status bar */
void ui_layout_draw_footer(ui_layout_t* layout, const app_state_t* state);

#endif /* UI_LAYOUT_H */
