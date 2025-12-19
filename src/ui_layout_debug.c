/**
 * Phoenix SDR Controller - UI Layout Debug Mode
 * 
 * F1 debug overlay showing widget positions and coordinates
 */

#include "ui_layout.h"
#include <stdio.h>

/* Debug colors */
#define DEBUG_COLOR_BUTTON     0xFF4400FF
#define DEBUG_COLOR_SLIDER     0x00FF44FF
#define DEBUG_COLOR_COMBO      0x4400FFFF
#define DEBUG_COLOR_TOGGLE     0xFF00FFFF
#define DEBUG_COLOR_LED        0xFFFF00FF
#define DEBUG_COLOR_PANEL      0x00FFFFFF
#define DEBUG_COLOR_REGION     0xFF8800FF

/*
 * Toggle debug mode
 */
void ui_layout_toggle_debug(ui_layout_t* layout)
{
    if (!layout) return;
    layout->debug_mode = !layout->debug_mode;
    LOG_INFO("Debug mode: %s", layout->debug_mode ? "ON" : "OFF");
}

/*
 * Helper: Check if point is inside rect
 */
static bool point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

/*
 * Helper: Draw debug rect with label
 */
static void draw_debug_rect(ui_core_t* ui, const char* label, int x, int y, int w, int h, uint32_t color)
{
    char buf[128];
    
    /* Draw colored border */
    ui_draw_rect_outline(ui, x, y, w, h, color);
    ui_draw_rect_outline(ui, x+1, y+1, w-2, h-2, color);
    
    /* Draw label with coordinates */
    snprintf(buf, sizeof(buf), "%s [%d,%d %dx%d]", label, x, y, w, h);
    
    /* Draw text with background */
    int text_y = y + 2;
    if (text_y + 12 > y + h) text_y = y + h - 14;  /* Keep inside rect */
    
    /* Background for readability */
    ui_draw_rect(ui, x + 2, text_y, w - 4, 12, 0x000000DD);
    ui_draw_text(ui, ui->font_small, buf, x + 4, text_y, color);
}

/*
 * Draw debug overlay
 */
void ui_layout_draw_debug(ui_layout_t* layout)
{
    if (!layout || !layout->ui || !layout->debug_mode) return;
    
    ui_core_t* ui = layout->ui;
    
    /* Draw message */
    ui_draw_text(ui, ui->font_large, "DEBUG MODE (F1 to toggle)", 10, 40, COLOR_YELLOW);
    
    /* Draw all regions */
    draw_debug_rect(ui, "HEADER", 
        layout->regions.header.x, layout->regions.header.y,
        layout->regions.header.w, layout->regions.header.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "FREQ_AREA", 
        layout->regions.freq_area.x, layout->regions.freq_area.y,
        layout->regions.freq_area.w, layout->regions.freq_area.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "GAIN_PANEL", 
        layout->regions.gain_panel.x, layout->regions.gain_panel.y,
        layout->regions.gain_panel.w, layout->regions.gain_panel.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "CONFIG_PANEL", 
        layout->regions.config_panel.x, layout->regions.config_panel.y,
        layout->regions.config_panel.w, layout->regions.config_panel.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "STATUS_PANEL", 
        layout->regions.status_panel.x, layout->regions.status_panel.y,
        layout->regions.status_panel.w, layout->regions.status_panel.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "WWV_PANEL", 
        layout->regions.wwv_panel.x, layout->regions.wwv_panel.y,
        layout->regions.wwv_panel.w, layout->regions.wwv_panel.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "BCD_PANEL", 
        layout->regions.bcd_panel.x, layout->regions.bcd_panel.y,
        layout->regions.bcd_panel.w, layout->regions.bcd_panel.h, DEBUG_COLOR_REGION);
    
    draw_debug_rect(ui, "FOOTER", 
        layout->regions.footer.x, layout->regions.footer.y,
        layout->regions.footer.w, layout->regions.footer.h, DEBUG_COLOR_REGION);
    
    /* Draw buttons */
    draw_debug_rect(ui, "btn_connect", 
        layout->btn_connect.x, layout->btn_connect.y,
        layout->btn_connect.w, layout->btn_connect.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_start", 
        layout->btn_start.x, layout->btn_start.y,
        layout->btn_start.w, layout->btn_start.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_stop", 
        layout->btn_stop.x, layout->btn_stop.y,
        layout->btn_stop.w, layout->btn_stop.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_freq_up", 
        layout->btn_freq_up.x, layout->btn_freq_up.y,
        layout->btn_freq_up.w, layout->btn_freq_up.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_freq_down", 
        layout->btn_freq_down.x, layout->btn_freq_down.y,
        layout->btn_freq_down.w, layout->btn_freq_down.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_step_up", 
        layout->btn_step_up.x, layout->btn_step_up.y,
        layout->btn_step_up.w, layout->btn_step_up.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_step_down", 
        layout->btn_step_down.x, layout->btn_step_down.y,
        layout->btn_step_down.w, layout->btn_step_down.h, DEBUG_COLOR_BUTTON);
    
    /* WWV buttons */
    draw_debug_rect(ui, "wwv_2.5", 
        layout->btn_wwv_2_5.x, layout->btn_wwv_2_5.y,
        layout->btn_wwv_2_5.w, layout->btn_wwv_2_5.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_5", 
        layout->btn_wwv_5.x, layout->btn_wwv_5.y,
        layout->btn_wwv_5.w, layout->btn_wwv_5.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_10", 
        layout->btn_wwv_10.x, layout->btn_wwv_10.y,
        layout->btn_wwv_10.w, layout->btn_wwv_10.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_15", 
        layout->btn_wwv_15.x, layout->btn_wwv_15.y,
        layout->btn_wwv_15.w, layout->btn_wwv_15.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_20", 
        layout->btn_wwv_20.x, layout->btn_wwv_20.y,
        layout->btn_wwv_20.w, layout->btn_wwv_20.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_25", 
        layout->btn_wwv_25.x, layout->btn_wwv_25.y,
        layout->btn_wwv_25.w, layout->btn_wwv_25.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "wwv_30", 
        layout->btn_wwv_30.x, layout->btn_wwv_30.y,
        layout->btn_wwv_30.w, layout->btn_wwv_30.h, DEBUG_COLOR_BUTTON);
    
    /* Preset buttons */
    for (int i = 0; i < NUM_PRESETS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "M%d", i+1);
        draw_debug_rect(ui, name, 
            layout->btn_preset[i].x, layout->btn_preset[i].y,
            layout->btn_preset[i].w, layout->btn_preset[i].h, DEBUG_COLOR_BUTTON);
    }
    
    /* Process buttons */
    draw_debug_rect(ui, "btn_server", 
        layout->btn_server.x, layout->btn_server.y,
        layout->btn_server.w, layout->btn_server.h, DEBUG_COLOR_BUTTON);
    
    draw_debug_rect(ui, "btn_waterfall", 
        layout->btn_waterfall.x, layout->btn_waterfall.y,
        layout->btn_waterfall.w, layout->btn_waterfall.h, DEBUG_COLOR_BUTTON);
    
    /* Sliders */
    draw_debug_rect(ui, "slider_gain", 
        layout->slider_gain.x, layout->slider_gain.y,
        layout->slider_gain.w, layout->slider_gain.h, DEBUG_COLOR_SLIDER);
    
    draw_debug_rect(ui, "slider_lna", 
        layout->slider_lna.x, layout->slider_lna.y,
        layout->slider_lna.w, layout->slider_lna.h, DEBUG_COLOR_SLIDER);
    
    draw_debug_rect(ui, "slider_aff", 
        layout->slider_aff_interval.x, layout->slider_aff_interval.y,
        layout->slider_aff_interval.w, layout->slider_aff_interval.h, DEBUG_COLOR_SLIDER);
    
    /* Combos */
    draw_debug_rect(ui, "combo_agc", 
        layout->combo_agc.x, layout->combo_agc.y,
        layout->combo_agc.w, layout->combo_agc.h, DEBUG_COLOR_COMBO);
    
    draw_debug_rect(ui, "combo_srate", 
        layout->combo_srate.x, layout->combo_srate.y,
        layout->combo_srate.w, layout->combo_srate.h, DEBUG_COLOR_COMBO);
    
    draw_debug_rect(ui, "combo_bw", 
        layout->combo_bw.x, layout->combo_bw.y,
        layout->combo_bw.w, layout->combo_bw.h, DEBUG_COLOR_COMBO);
    
    draw_debug_rect(ui, "combo_antenna", 
        layout->combo_antenna.x, layout->combo_antenna.y,
        layout->combo_antenna.w, layout->combo_antenna.h, DEBUG_COLOR_COMBO);
    
    /* Toggles */
    draw_debug_rect(ui, "toggle_biast", 
        layout->toggle_biast.x - 5, layout->toggle_biast.y - 5,
        50, 30, DEBUG_COLOR_TOGGLE);
    
    draw_debug_rect(ui, "toggle_notch", 
        layout->toggle_notch.x - 5, layout->toggle_notch.y - 5,
        50, 30, DEBUG_COLOR_TOGGLE);
    
    draw_debug_rect(ui, "toggle_aff", 
        layout->toggle_aff.x - 5, layout->toggle_aff.y - 5,
        50, 30, DEBUG_COLOR_TOGGLE);
    
    /* Frequency display */
    draw_debug_rect(ui, "freq_display", 
        layout->freq_display.x, layout->freq_display.y,
        layout->freq_display.w, layout->freq_display.h, DEBUG_COLOR_PANEL);
    
    /* DC offset dot */
    draw_debug_rect(ui, "dc_offset_dot", 
        layout->offset_dot.x, layout->offset_dot.y,
        layout->offset_dot.w, layout->offset_dot.h, DEBUG_COLOR_LED);
    
    /* LEDs */
    char led_buf[64];
    
    snprintf(led_buf, sizeof(led_buf), "led_connected [%d,%d]", 
        layout->led_connected.x, layout->led_connected.y);
    ui_draw_text(ui, ui->font_small, led_buf, 
        layout->led_connected.x + 10, layout->led_connected.y - 15, DEBUG_COLOR_LED);
    
    snprintf(led_buf, sizeof(led_buf), "led_streaming [%d,%d]", 
        layout->led_streaming.x, layout->led_streaming.y);
    ui_draw_text(ui, ui->font_small, led_buf, 
        layout->led_streaming.x + 10, layout->led_streaming.y - 15, DEBUG_COLOR_LED);
    
    snprintf(led_buf, sizeof(led_buf), "led_overload [%d,%d]", 
        layout->led_overload.x, layout->led_overload.y);
    ui_draw_text(ui, ui->font_small, led_buf, 
        layout->led_overload.x + 10, layout->led_overload.y - 15, DEBUG_COLOR_LED);
}

/*
 * Handle debug mode click - copy widget metadata to clipboard
 */
void ui_layout_debug_click(ui_layout_t* layout, int x, int y)
{
    if (!layout) return;
    
    char buf[512];
    const char* widget_name = NULL;
    int wx = 0, wy = 0, ww = 0, wh = 0;
    
    /* Check all widgets and find which one was clicked */
    #define CHECK_WIDGET(name, widget) \
        if (point_in_rect(x, y, widget.x, widget.y, widget.w, widget.h)) { \
            widget_name = name; wx = widget.x; wy = widget.y; ww = widget.w; wh = widget.h; \
            goto found; \
        }
    
    #define CHECK_LED(name, led) \
        if (point_in_rect(x, y, led.x - led.radius, led.y - led.radius, led.radius * 2, led.radius * 2)) { \
            widget_name = name; wx = led.x; wy = led.y; ww = led.radius * 2; wh = led.radius * 2; \
            goto found; \
        }
    
    #define CHECK_TOGGLE(name, toggle) \
        if (point_in_rect(x, y, toggle.x - 5, toggle.y - 5, 50, 30)) { \
            widget_name = name; wx = toggle.x; wy = toggle.y; ww = 50; wh = 30; \
            goto found; \
        }
    
    /* Buttons */
    CHECK_WIDGET("btn_connect", layout->btn_connect)
    CHECK_WIDGET("btn_start", layout->btn_start)
    CHECK_WIDGET("btn_stop", layout->btn_stop)
    CHECK_WIDGET("btn_step_up", layout->btn_step_up)
    CHECK_WIDGET("btn_step_down", layout->btn_step_down)
    CHECK_WIDGET("btn_freq_up", layout->btn_freq_up)
    CHECK_WIDGET("btn_freq_down", layout->btn_freq_down)
    CHECK_WIDGET("btn_server", layout->btn_server)
    CHECK_WIDGET("btn_waterfall", layout->btn_waterfall)
    CHECK_WIDGET("btn_wwv_2_5", layout->btn_wwv_2_5)
    CHECK_WIDGET("btn_wwv_5", layout->btn_wwv_5)
    CHECK_WIDGET("btn_wwv_10", layout->btn_wwv_10)
    CHECK_WIDGET("btn_wwv_15", layout->btn_wwv_15)
    CHECK_WIDGET("btn_wwv_20", layout->btn_wwv_20)
    CHECK_WIDGET("btn_wwv_25", layout->btn_wwv_25)
    CHECK_WIDGET("btn_wwv_30", layout->btn_wwv_30)
    
    /* Check preset buttons */
    for (int i = 0; i < NUM_PRESETS; i++) {
        if (point_in_rect(x, y, layout->btn_preset[i].x, layout->btn_preset[i].y,
                        layout->btn_preset[i].w, layout->btn_preset[i].h)) {
            snprintf(buf, sizeof(buf), "btn_preset[%d]", i);
            widget_name = buf;
            wx = layout->btn_preset[i].x; wy = layout->btn_preset[i].y;
            ww = layout->btn_preset[i].w; wh = layout->btn_preset[i].h;
            goto found;
        }
    }
    
    /* Sliders */
    CHECK_WIDGET("slider_gain", layout->slider_gain)
    CHECK_WIDGET("slider_lna", layout->slider_lna)
    
    /* Combos */
    CHECK_WIDGET("combo_agc", layout->combo_agc)
    CHECK_WIDGET("combo_srate", layout->combo_srate)
    CHECK_WIDGET("combo_bw", layout->combo_bw)
    CHECK_WIDGET("combo_antenna", layout->combo_antenna)
    
    /* Toggles (use fixed size) */
    CHECK_TOGGLE("toggle_biast", layout->toggle_biast)
    CHECK_TOGGLE("toggle_notch", layout->toggle_notch)
    CHECK_TOGGLE("toggle_aff", layout->toggle_aff)
    
    /* LEDs (use radius for hit testing) */
    CHECK_LED("led_connected", layout->led_connected)
    CHECK_LED("led_streaming", layout->led_streaming)
    CHECK_LED("led_overload", layout->led_overload)
    CHECK_LED("led_tone500", layout->led_tone500)
    CHECK_LED("led_tone600", layout->led_tone600)
    CHECK_LED("led_match", layout->led_match)
    CHECK_LED("led_bcd_sync", layout->led_bcd_sync)
    
    /* Frequency display */
    CHECK_WIDGET("freq_display", layout->freq_display)
    
    /* Panels */
    CHECK_WIDGET("panel_freq", layout->panel_freq)
    CHECK_WIDGET("panel_gain", layout->panel_gain)
    CHECK_WIDGET("panel_config", layout->panel_config)
    CHECK_WIDGET("panel_wwv", layout->panel_wwv)
    
    /* Regions */
    if (point_in_rect(x, y, layout->regions.header.x, layout->regions.header.y,
                     layout->regions.header.w, layout->regions.header.h)) {
        widget_name = "region_header";
        wx = layout->regions.header.x; wy = layout->regions.header.y;
        ww = layout->regions.header.w; wh = layout->regions.header.h;
        goto found;
    }
    if (point_in_rect(x, y, layout->regions.freq_area.x, layout->regions.freq_area.y,
                     layout->regions.freq_area.w, layout->regions.freq_area.h)) {
        widget_name = "region_freq_area";
        wx = layout->regions.freq_area.x; wy = layout->regions.freq_area.y;
        ww = layout->regions.freq_area.w; wh = layout->regions.freq_area.h;
        goto found;
    }
    if (point_in_rect(x, y, layout->regions.gain_panel.x, layout->regions.gain_panel.y,
                     layout->regions.gain_panel.w, layout->regions.gain_panel.h)) {
        widget_name = "region_gain_panel";
        wx = layout->regions.gain_panel.x; wy = layout->regions.gain_panel.y;
        ww = layout->regions.gain_panel.w; wh = layout->regions.gain_panel.h;
        goto found;
    }
    if (point_in_rect(x, y, layout->regions.wwv_panel.x, layout->regions.wwv_panel.y,
                     layout->regions.wwv_panel.w, layout->regions.wwv_panel.h)) {
        widget_name = "region_wwv_panel";
        wx = layout->regions.wwv_panel.x; wy = layout->regions.wwv_panel.y;
        ww = layout->regions.wwv_panel.w; wh = layout->regions.wwv_panel.h;
        goto found;
    }
    if (point_in_rect(x, y, layout->regions.bcd_panel.x, layout->regions.bcd_panel.y,
                     layout->regions.bcd_panel.w, layout->regions.bcd_panel.h)) {
        widget_name = "region_bcd_panel";
        wx = layout->regions.bcd_panel.x; wy = layout->regions.bcd_panel.y;
        ww = layout->regions.bcd_panel.w; wh = layout->regions.bcd_panel.h;
        goto found;
    }
    if (point_in_rect(x, y, layout->regions.footer.x, layout->regions.footer.y,
                     layout->regions.footer.w, layout->regions.footer.h)) {
        widget_name = "region_footer";
        wx = layout->regions.footer.x; wy = layout->regions.footer.y;
        ww = layout->regions.footer.w; wh = layout->regions.footer.h;
        goto found;
    }
    
    #undef CHECK_WIDGET
    #undef CHECK_LED
    #undef CHECK_TOGGLE
    
found:
    if (widget_name) {
        /* Format metadata and copy to clipboard */
        snprintf(buf, sizeof(buf), "%s: x=%d y=%d w=%d h=%d", widget_name, wx, wy, ww, wh);
        SDL_SetClipboardText(buf);
        LOG_INFO("Copied to clipboard: %s", buf);
    } else {
        LOG_DEBUG("No widget at click position (%d, %d)", x, y);
    }
}
