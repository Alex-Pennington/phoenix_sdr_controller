/**
 * Phoenix SDR Controller - UI Widgets
 * 
 * Reusable UI widget components.
 */

#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "ui_core.h"

/* Button widget */
typedef struct {
    int x, y, w, h;
    const char* label;
    bool enabled;
    bool hovered;
    bool pressed;
    bool toggled;
    uint32_t color_normal;
    uint32_t color_hover;
    uint32_t color_active;
    uint32_t color_disabled;
} widget_button_t;

/* Slider widget */
typedef struct {
    int x, y, w, h;
    int min_val, max_val;
    int value;
    bool vertical;
    bool enabled;
    bool dragging;
    const char* label;
    const char* format;  /* printf format for value display */
} widget_slider_t;

/* Combo box (dropdown) */
typedef struct {
    int x, y, w, h;
    const char** items;
    int item_count;
    int selected;
    bool enabled;
    bool open;
    const char* label;
} widget_combo_t;

/* Toggle switch */
typedef struct {
    int x, y;
    bool value;
    bool enabled;
    const char* label;
} widget_toggle_t;

/* LED indicator */
typedef struct {
    int x, y, radius;
    uint32_t color_on;
    uint32_t color_off;
    bool on;
    const char* label;
} widget_led_t;

/* Frequency display */
typedef struct {
    int x, y, w, h;
    int64_t frequency;
    int selected_digit;  /* -1 for none, 0-9 for digit selection */
    bool editing;
} widget_freq_display_t;

/* Panel/frame */
typedef struct {
    int x, y, w, h;
    const char* title;
    uint32_t bg_color;
    uint32_t border_color;
} widget_panel_t;

/* S-Meter */
typedef struct {
    int x, y, w, h;
    int value;  /* 0-100 */
    int peak;   /* Peak hold value */
} widget_smeter_t;

/* Button functions */
void widget_button_init(widget_button_t* btn, int x, int y, int w, int h, const char* label);
bool widget_button_update(widget_button_t* btn, const mouse_state_t* mouse);
void widget_button_draw(widget_button_t* btn, ui_core_t* ui);

/* Slider functions */
void widget_slider_init(widget_slider_t* slider, int x, int y, int w, int h, 
                        int min_val, int max_val, bool vertical);
bool widget_slider_update(widget_slider_t* slider, const mouse_state_t* mouse);
void widget_slider_draw(widget_slider_t* slider, ui_core_t* ui);

/* Combo box functions */
void widget_combo_init(widget_combo_t* combo, int x, int y, int w, int h,
                       const char** items, int count);
bool widget_combo_update(widget_combo_t* combo, const mouse_state_t* mouse);
void widget_combo_draw(widget_combo_t* combo, ui_core_t* ui);

/* Toggle functions */
void widget_toggle_init(widget_toggle_t* toggle, int x, int y, const char* label);
bool widget_toggle_update(widget_toggle_t* toggle, const mouse_state_t* mouse);
void widget_toggle_draw(widget_toggle_t* toggle, ui_core_t* ui);

/* LED functions */
void widget_led_init(widget_led_t* led, int x, int y, int radius, 
                     uint32_t color_on, uint32_t color_off, const char* label);
void widget_led_draw(widget_led_t* led, ui_core_t* ui);

/* Frequency display functions */
void widget_freq_display_init(widget_freq_display_t* disp, int x, int y, int w, int h);
bool widget_freq_display_update(widget_freq_display_t* disp, const mouse_state_t* mouse, SDL_Event* event);
void widget_freq_display_draw(widget_freq_display_t* disp, ui_core_t* ui);

/* Panel functions */
void widget_panel_init(widget_panel_t* panel, int x, int y, int w, int h, const char* title);
void widget_panel_draw(widget_panel_t* panel, ui_core_t* ui);

/* S-Meter functions */
void widget_smeter_init(widget_smeter_t* meter, int x, int y, int w, int h);
void widget_smeter_draw(widget_smeter_t* meter, ui_core_t* ui);

#endif /* UI_WIDGETS_H */
