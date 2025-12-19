/**
 * Phoenix SDR Controller - UI Widgets Implementation
 * 
 * Reusable UI widget components.
 */

#include "ui_widgets.h"
#include "app_state.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Widget constants */
#define TOGGLE_WIDTH 44
#define TOGGLE_HEIGHT 22
#define COMBO_ITEM_HEIGHT 24
#define LED_GLOW_RADIUS 2

/* ============================================================================
 * BUTTON WIDGET
 * ============================================================================ */

void widget_button_init(widget_button_t* btn, int x, int y, int w, int h, const char* label)
{
    if (!btn) return;
    
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->label = label;
    btn->enabled = true;
    btn->hovered = false;
    btn->pressed = false;
    btn->toggled = false;
    btn->color_normal = COLOR_BUTTON;
    btn->color_hover = COLOR_BUTTON_HOVER;
    btn->color_active = COLOR_BUTTON_ACTIVE;
    btn->color_disabled = COLOR_BG_WIDGET;
}

bool widget_button_update(widget_button_t* btn, const mouse_state_t* mouse)
{
    if (!btn || !mouse || !btn->enabled) {
        if (btn) btn->hovered = false;
        return false;
    }
    
    btn->hovered = ui_point_in_rect(mouse->x, mouse->y, btn->x, btn->y, btn->w, btn->h);
    
    if (btn->hovered && mouse->left_down) {
        btn->pressed = true;
    } else if (btn->pressed && mouse->left_released) {
        btn->pressed = false;
        if (btn->hovered) {
            return true;  /* Button was clicked */
        }
    } else if (!mouse->left_down) {
        btn->pressed = false;
    }
    
    return false;
}

void widget_button_draw(widget_button_t* btn, ui_core_t* ui)
{
    if (!btn || !ui) return;
    
    /* Determine background color */
    uint32_t bg_color;
    if (!btn->enabled) {
        bg_color = btn->color_disabled;
    } else if (btn->pressed || btn->toggled) {
        bg_color = btn->color_active;
    } else if (btn->hovered) {
        bg_color = btn->color_hover;
    } else {
        bg_color = btn->color_normal;
    }
    
    /* Draw button background */
    ui_draw_rect(ui, btn->x, btn->y, btn->w, btn->h, bg_color);
    
    /* Draw border */
    uint32_t border_color = btn->enabled ? COLOR_ACCENT_DIM : COLOR_TEXT_DIM;
    ui_draw_rect_outline(ui, btn->x, btn->y, btn->w, btn->h, border_color);
    
    /* Draw label */
    if (btn->label) {
        uint32_t text_color = btn->enabled ? COLOR_TEXT : COLOR_TEXT_DIM;
        ui_draw_text_centered(ui, ui->font_normal, btn->label, 
                              btn->x, btn->y + (btn->h - 14) / 2, btn->w, text_color);
    }
}

/* ============================================================================
 * SLIDER WIDGET
 * ============================================================================ */

void widget_slider_init(widget_slider_t* slider, int x, int y, int w, int h,
                        int min_val, int max_val, bool vertical)
{
    if (!slider) return;
    
    slider->x = x;
    slider->y = y;
    slider->w = w;
    slider->h = h;
    slider->min_val = min_val;
    slider->max_val = max_val;
    slider->value = min_val;
    slider->vertical = vertical;
    slider->enabled = true;
    slider->dragging = false;
    slider->label = NULL;
    slider->format = "%d";
}

bool widget_slider_update(widget_slider_t* slider, const mouse_state_t* mouse)
{
    if (!slider || !mouse || !slider->enabled) return false;
    
    bool in_bounds = ui_point_in_rect(mouse->x, mouse->y, 
                                       slider->x, slider->y, slider->w, slider->h);
    
    int old_value = slider->value;
    
    /* Start dragging - only if click started inside the slider bounds */
    if (in_bounds && mouse->left_clicked) {
        slider->dragging = true;
    }
    
    /* Stop dragging */
    if (!mouse->left_down) {
        slider->dragging = false;
    }
    
    /* Update value while dragging - but only if click originated from this slider */
    if (slider->dragging && mouse->left_down) {
        float ratio;
        if (slider->vertical) {
            /* Vertical: top = max, bottom = min */
            ratio = 1.0f - (float)(mouse->y - slider->y) / (float)slider->h;
        } else {
            /* Horizontal: left = min, right = max */
            ratio = (float)(mouse->x - slider->x) / (float)slider->w;
        }
        
        ratio = CLAMP(ratio, 0.0f, 1.0f);
        slider->value = slider->min_val + (int)(ratio * (slider->max_val - slider->min_val));
        slider->value = CLAMP(slider->value, slider->min_val, slider->max_val);
    }
    
    /* Handle mouse wheel */
    if (in_bounds && mouse->wheel_y != 0) {
        int step = (slider->max_val - slider->min_val) / 20;
        if (step < 1) step = 1;
        slider->value += mouse->wheel_y * step;
        slider->value = CLAMP(slider->value, slider->min_val, slider->max_val);
    }
    
    return slider->value != old_value;
}

void widget_slider_draw(widget_slider_t* slider, ui_core_t* ui)
{
    if (!slider || !ui) return;
    
    /* Draw background track */
    ui_draw_rect(ui, slider->x, slider->y, slider->w, slider->h, COLOR_SLIDER_BG);
    ui_draw_rect_outline(ui, slider->x, slider->y, slider->w, slider->h, COLOR_ACCENT_DIM);
    
    /* Calculate fill ratio */
    float ratio = (float)(slider->value - slider->min_val) / 
                  (float)(slider->max_val - slider->min_val);
    
    /* Draw filled portion */
    uint32_t fill_color = slider->enabled ? COLOR_SLIDER_FG : COLOR_ACCENT_DIM;
    
    if (slider->vertical) {
        int fill_h = (int)(ratio * slider->h);
        ui_draw_rect(ui, slider->x + 2, slider->y + slider->h - fill_h, 
                     slider->w - 4, fill_h, fill_color);
    } else {
        int fill_w = (int)(ratio * slider->w);
        ui_draw_rect(ui, slider->x, slider->y + 2, fill_w, slider->h - 4, fill_color);
    }
    
    /* Draw value text */
    char value_str[32];
    snprintf(value_str, sizeof(value_str), slider->format ? slider->format : "%d", slider->value);
    
    uint32_t text_color = slider->enabled ? COLOR_TEXT : COLOR_TEXT_DIM;
    if (slider->vertical) {
        ui_draw_text_centered(ui, ui->font_small, value_str,
                              slider->x, slider->y + slider->h + 4, slider->w, text_color);
    } else {
        int text_y = slider->y + (slider->h - 12) / 2;
        ui_draw_text_centered(ui, ui->font_small, value_str,
                              slider->x, text_y, slider->w, text_color);
    }
    
    /* Draw label if present */
    if (slider->label) {
        if (slider->vertical) {
            ui_draw_text_centered(ui, ui->font_small, slider->label,
                                  slider->x, slider->y - 18, slider->w, text_color);
        } else {
            ui_draw_text(ui, ui->font_small, slider->label, 
                        slider->x, slider->y - 18, text_color);
        }
    }
}

/* ============================================================================
 * COMBO BOX WIDGET
 * ============================================================================ */

void widget_combo_init(widget_combo_t* combo, int x, int y, int w, int h,
                       const char** items, int count)
{
    if (!combo) return;
    
    combo->x = x;
    combo->y = y;
    combo->w = w;
    combo->h = h;
    combo->items = items;
    combo->item_count = count;
    combo->selected = 0;
    combo->enabled = true;
    combo->open = false;
    combo->label = NULL;
}

bool widget_combo_update(widget_combo_t* combo, const mouse_state_t* mouse)
{
    if (!combo || !mouse || !combo->enabled) return false;
    
    bool in_header = ui_point_in_rect(mouse->x, mouse->y,
                                       combo->x, combo->y, combo->w, combo->h);
    
    int old_selected = combo->selected;
    
    if (combo->open) {
        /* Check clicks on dropdown items */
        int dropdown_h = combo->item_count * COMBO_ITEM_HEIGHT;
        bool in_dropdown = ui_point_in_rect(mouse->x, mouse->y,
                                             combo->x, combo->y + combo->h,
                                             combo->w, dropdown_h);
        
        if (mouse->left_clicked) {
            if (in_dropdown) {
                int item_idx = (mouse->y - (combo->y + combo->h)) / COMBO_ITEM_HEIGHT;
                if (item_idx >= 0 && item_idx < combo->item_count) {
                    combo->selected = item_idx;
                    LOG_INFO("Combo: selected item %d", item_idx);
                }
            }
            combo->open = false;
        }
    } else {
        /* Open dropdown on click */
        if (in_header && mouse->left_clicked) {
            combo->open = true;
            LOG_DEBUG("Combo: opened dropdown");
        }
    }
    
    if (combo->selected != old_selected) {
        LOG_INFO("Combo: selection changed from %d to %d", old_selected, combo->selected);
    }
    
    return combo->selected != old_selected;
}
void widget_combo_draw(widget_combo_t* combo, ui_core_t* ui)
{
    if (!combo || !ui) return;
    
    uint32_t bg_color = combo->enabled ? COLOR_BG_WIDGET : COLOR_BG_PANEL;
    uint32_t text_color = combo->enabled ? COLOR_TEXT : COLOR_TEXT_DIM;
    
    /* Draw header */
    ui_draw_rect(ui, combo->x, combo->y, combo->w, combo->h, bg_color);
    ui_draw_rect_outline(ui, combo->x, combo->y, combo->w, combo->h, COLOR_ACCENT_DIM);
    
    /* Draw selected item text */
    if (combo->items && combo->selected >= 0 && combo->selected < combo->item_count) {
        ui_draw_text(ui, ui->font_normal, combo->items[combo->selected],
                    combo->x + 8, combo->y + (combo->h - 14) / 2, text_color);
    }
    
    /* Draw dropdown arrow */
    int arrow_x = combo->x + combo->w - 16;
    int arrow_y = combo->y + combo->h / 2;
    ui_draw_text(ui, ui->font_normal, combo->open ? "^" : "v", 
                arrow_x, arrow_y - 7, COLOR_ACCENT);
    
    /* Draw label if present */
    if (combo->label) {
        ui_draw_text(ui, ui->font_small, combo->label,
                    combo->x, combo->y - 18, text_color);
    }
    
    /* Draw dropdown if open */
    if (combo->open && combo->items) {
        int dropdown_y = combo->y + combo->h;
        int dropdown_h = combo->item_count * COMBO_ITEM_HEIGHT;
        
        /* Dropdown background */
        ui_draw_rect(ui, combo->x, dropdown_y, combo->w, dropdown_h, COLOR_BG_PANEL);
        ui_draw_rect_outline(ui, combo->x, dropdown_y, combo->w, dropdown_h, COLOR_ACCENT);
        
        /* Draw items */
        for (int i = 0; i < combo->item_count; i++) {
            int item_y = dropdown_y + i * COMBO_ITEM_HEIGHT;
            
            /* Highlight selected item */
            if (i == combo->selected) {
                ui_draw_rect(ui, combo->x + 1, item_y, combo->w - 2, COMBO_ITEM_HEIGHT, COLOR_ACCENT_DIM);
            }
            
            ui_draw_text(ui, ui->font_normal, combo->items[i],
                        combo->x + 8, item_y + (COMBO_ITEM_HEIGHT - 14) / 2, COLOR_TEXT);
        }
    }
}

/* ============================================================================
 * TOGGLE SWITCH WIDGET
 * ============================================================================ */

void widget_toggle_init(widget_toggle_t* toggle, int x, int y, const char* label)
{
    if (!toggle) return;
    
    toggle->x = x;
    toggle->y = y;
    toggle->value = false;
    toggle->enabled = true;
    toggle->label = label;
}

bool widget_toggle_update(widget_toggle_t* toggle, const mouse_state_t* mouse)
{
    if (!toggle || !mouse || !toggle->enabled) return false;
    
    bool in_bounds = ui_point_in_rect(mouse->x, mouse->y,
                                       toggle->x, toggle->y, TOGGLE_WIDTH, TOGGLE_HEIGHT);
    
    if (in_bounds && mouse->left_clicked) {
        toggle->value = !toggle->value;
        return true;
    }
    
    return false;
}

void widget_toggle_draw(widget_toggle_t* toggle, ui_core_t* ui)
{
    if (!toggle || !ui) return;
    
    uint32_t bg_color = toggle->value ? COLOR_GREEN : COLOR_BG_WIDGET;
    uint32_t text_color = toggle->enabled ? COLOR_TEXT : COLOR_TEXT_DIM;
    
    if (!toggle->enabled) {
        bg_color = COLOR_BG_PANEL;
    }
    
    /* Draw track */
    ui_draw_rounded_rect(ui, toggle->x, toggle->y, TOGGLE_WIDTH, TOGGLE_HEIGHT, 
                         TOGGLE_HEIGHT / 2, bg_color);
    ui_draw_rect_outline(ui, toggle->x, toggle->y, TOGGLE_WIDTH, TOGGLE_HEIGHT, COLOR_ACCENT_DIM);
    
    /* Draw knob */
    int knob_x = toggle->value ? toggle->x + TOGGLE_WIDTH - TOGGLE_HEIGHT + 2 : toggle->x + 2;
    int knob_size = TOGGLE_HEIGHT - 4;
    ui_draw_rect(ui, knob_x, toggle->y + 2, knob_size, knob_size, COLOR_TEXT);
    
    /* Draw label */
    if (toggle->label) {
        ui_draw_text(ui, ui->font_normal, toggle->label,
                    toggle->x + TOGGLE_WIDTH + 8, toggle->y + (TOGGLE_HEIGHT - 14) / 2, text_color);
    }
}

/* ============================================================================
 * LED INDICATOR WIDGET
 * ============================================================================ */

void widget_led_init(widget_led_t* led, int x, int y, int radius,
                     uint32_t color_on, uint32_t color_off, const char* label)
{
    if (!led) return;
    
    led->x = x;
    led->y = y;
    led->radius = radius;
    led->color_on = color_on;
    led->color_off = color_off;
    led->on = false;
    led->label = label;
    led->hovered = false;
}

void widget_led_update(widget_led_t* led, const mouse_state_t* mouse)
{
    if (!led || !mouse) {
        if (led) led->hovered = false;
        return;
    }
    
    /* Check if mouse is over LED (use slightly larger hit area) */
    int hit_radius = led->radius + 15;  /* Generous hit area */
    int dx = mouse->x - led->x;
    int dy = mouse->y - led->y;
    led->hovered = (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

void widget_led_draw(widget_led_t* led, ui_core_t* ui)
{
    if (!led || !ui) return;
    
    uint32_t color = led->on ? led->color_on : led->color_off;
    
    /* Draw LED as a filled circle (approximated as a small square for simplicity) */
    int size = led->radius * 2;
    ui_draw_rect(ui, led->x - led->radius, led->y - led->radius, size, size, color);
    
    /* Draw glow effect when on */
    if (led->on) {
        uint32_t glow_color = (color & 0xFFFFFF00) | 0x40;  /* Same color, lower alpha */
        ui_draw_rect_outline(ui, led->x - led->radius - 1, led->y - led->radius - 1,
                             size + 2, size + 2, glow_color);
    }
    
    /* Draw label ONLY when hovered */
    if (led->label && led->hovered) {
        ui_draw_text(ui, ui->font_small, led->label,
                    led->x + led->radius + 6, led->y - 6, COLOR_TEXT);
    }
}

/* ============================================================================
 * FREQUENCY DISPLAY WIDGET
 * ============================================================================ */

void widget_freq_display_init(widget_freq_display_t* disp, int x, int y, int w, int h)
{
    if (!disp) return;
    
    disp->x = x;
    disp->y = y;
    disp->w = w;
    disp->h = h;
    disp->frequency = 15000000;
    disp->selected_digit = -1;
    disp->editing = false;
}

bool widget_freq_display_update(widget_freq_display_t* disp, const mouse_state_t* mouse, SDL_Event* event)
{
    if (!disp || !mouse) return false;
    
    bool in_bounds = ui_point_in_rect(mouse->x, mouse->y, disp->x, disp->y, disp->w, disp->h);
    
    /* Handle click to select digit */
    if (in_bounds && mouse->left_clicked) {
        /* Calculate which digit was clicked based on x position */
        /* Frequency format: X XXX XXX XXX (10 digits + 3 spaces) */
        int rel_x = mouse->x - disp->x - 10;  /* Offset for padding */
        int char_width = 28;  /* Approximate width of frequency font character */
        int digit_pos = rel_x / char_width;
        
        /* Map position to digit (accounting for spaces) */
        int digit_map[] = {0, -1, 1, 2, 3, -1, 4, 5, 6, -1, 7, 8, 9};
        if (digit_pos >= 0 && digit_pos < 13) {
            disp->selected_digit = digit_map[digit_pos];
            disp->editing = (disp->selected_digit >= 0);
        }
        return false;
    }
    
    /* Handle mouse wheel to change frequency */
    if (in_bounds && mouse->wheel_y != 0 && disp->selected_digit >= 0) {
        /* Calculate step based on selected digit position */
        int64_t step = 1;
        for (int i = 0; i < (9 - disp->selected_digit); i++) {
            step *= 10;
        }
        
        disp->frequency += mouse->wheel_y * step;
        disp->frequency = CLAMP(disp->frequency, FREQ_MIN, FREQ_MAX);
        return true;
    }
    
    /* Click outside to deselect */
    if (!in_bounds && mouse->left_clicked) {
        disp->selected_digit = -1;
        disp->editing = false;
    }
    
    return false;
}

void widget_freq_display_draw(widget_freq_display_t* disp, ui_core_t* ui)
{
    if (!disp || !ui) return;
    
    /* Don't draw background - parent panel already draws it */
    
    /* Format frequency with grouping */
    char freq_str[20];
    app_format_frequency_grouped(disp->frequency, freq_str, sizeof(freq_str));
    
    /* Draw frequency text */
    int text_x = disp->x + 10;
    int text_y = disp->y + (disp->h - 48) / 2;
    
    /* Draw each character, highlighting selected digit */
    int char_width = 28;
    int digit_index = 0;
    
    for (int i = 0; freq_str[i] != '\0'; i++) {
        char ch[2] = {freq_str[i], '\0'};
        
        uint32_t color = COLOR_FREQ_DISPLAY;
        
        /* Highlight selected digit */
        if (freq_str[i] != ' ' && digit_index == disp->selected_digit) {
            /* Draw highlight background */
            ui_draw_rect(ui, text_x + i * char_width - 2, text_y - 2, 
                        char_width, 52, COLOR_ACCENT_DIM);
            color = COLOR_TEXT;
        }
        
        ui_draw_text(ui, ui->font_freq, ch, text_x + i * char_width, text_y, color);
        
        if (freq_str[i] != ' ') {
            digit_index++;
        }
    }
    
    /* Draw "Hz" suffix */
    ui_draw_text(ui, ui->font_large, "Hz", 
                disp->x + disp->w - 40, disp->y + disp->h - 30, COLOR_TEXT_DIM);
}

/* ============================================================================
 * PANEL WIDGET
 * ============================================================================ */

void widget_panel_init(widget_panel_t* panel, int x, int y, int w, int h, const char* title)
{
    if (!panel) return;
    
    panel->x = x;
    panel->y = y;
    panel->w = w;
    panel->h = h;
    panel->title = title;
    panel->bg_color = COLOR_BG_PANEL;
    panel->border_color = COLOR_ACCENT_DIM;
}

void widget_panel_draw(widget_panel_t* panel, ui_core_t* ui)
{
    if (!panel || !ui) return;
    
    /* Draw background */
    ui_draw_rect(ui, panel->x, panel->y, panel->w, panel->h, panel->bg_color);
    ui_draw_rect_outline(ui, panel->x, panel->y, panel->w, panel->h, panel->border_color);
    
    /* Draw title bar */
    if (panel->title) {
        ui_draw_rect(ui, panel->x, panel->y, panel->w, 24, COLOR_BG_WIDGET);
        ui_draw_text(ui, ui->font_normal, panel->title, 
                    panel->x + 8, panel->y + 4, COLOR_ACCENT);
        ui_draw_line(ui, panel->x, panel->y + 24, 
                    panel->x + panel->w, panel->y + 24, panel->border_color);
    }
}

/* ============================================================================
 * S-METER WIDGET
 * ============================================================================ */

void widget_smeter_init(widget_smeter_t* meter, int x, int y, int w, int h)
{
    if (!meter) return;
    
    meter->x = x;
    meter->y = y;
    meter->w = w;
    meter->h = h;
    meter->value = 0;
    meter->peak = 0;
}

void widget_smeter_draw(widget_smeter_t* meter, ui_core_t* ui)
{
    if (!meter || !ui) return;
    
    /* Draw background */
    ui_draw_rect(ui, meter->x, meter->y, meter->w, meter->h, COLOR_BG_WIDGET);
    ui_draw_rect_outline(ui, meter->x, meter->y, meter->w, meter->h, COLOR_ACCENT_DIM);
    
    /* Draw meter bar */
    int bar_w = (meter->value * (meter->w - 4)) / 100;
    
    /* Color gradient from green to yellow to red */
    uint32_t bar_color;
    if (meter->value < 50) {
        bar_color = COLOR_GREEN;
    } else if (meter->value < 80) {
        bar_color = COLOR_YELLOW;
    } else {
        bar_color = COLOR_RED;
    }
    
    ui_draw_rect(ui, meter->x + 2, meter->y + 2, bar_w, meter->h - 4, bar_color);
    
    /* Draw peak marker */
    if (meter->peak > 0) {
        int peak_x = meter->x + 2 + (meter->peak * (meter->w - 4)) / 100;
        ui_draw_line(ui, peak_x, meter->y + 2, peak_x, meter->y + meter->h - 2, COLOR_RED);
    }
    
    /* Draw scale markers */
    for (int i = 0; i <= 10; i++) {
        int marker_x = meter->x + 2 + (i * (meter->w - 4)) / 10;
        ui_draw_line(ui, marker_x, meter->y + meter->h - 6, 
                    marker_x, meter->y + meter->h - 2, COLOR_TEXT_DIM);
    }
}
