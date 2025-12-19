/**
 * Phoenix SDR Controller - UI Layout Edit Mode
 * 
 * F2: Toggle drag-and-drop edit mode
 * F3: Dump all widget positions to layout_positions.txt
 */

#include "ui_layout.h"
#include <stdio.h>
#include <string.h>

/* Draggable widget info */
typedef struct {
    const char* name;
    int* x_ptr;
    int* y_ptr;
    int* w_ptr;
    int* h_ptr;
} draggable_widget_t;

/* Edit state */
static struct {
    draggable_widget_t* grabbed;
    int grab_offset_x;
    int grab_offset_y;
} edit_state = {0};

/*
 * Toggle edit mode
 */
void ui_layout_toggle_edit_mode(ui_layout_t* layout)
{
    if (!layout) return;
    layout->edit_mode = !layout->edit_mode;
    if (!layout->edit_mode) {
        edit_state.grabbed = NULL;
    }
    LOG_INFO("Edit mode: %s (F2=toggle, F3=dump)", layout->edit_mode ? "ON" : "OFF");
}

/*
 * Helper: Check if point is in widget bounds
 */
static bool point_in_rect(int px, int py, int x, int y, int w, int h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

/*
 * Find which widget is at the given coordinates
 */
static draggable_widget_t* find_widget_at(ui_layout_t* layout, int x, int y)
{
    static draggable_widget_t widgets[200];
    int count = 0;
    
    /* Add all buttons explicitly */
    widgets[count++] = (draggable_widget_t){"btn_connect", &layout->btn_connect.x, &layout->btn_connect.y, &layout->btn_connect.w, &layout->btn_connect.h};
    widgets[count++] = (draggable_widget_t){"btn_start", &layout->btn_start.x, &layout->btn_start.y, &layout->btn_start.w, &layout->btn_start.h};
    widgets[count++] = (draggable_widget_t){"btn_stop", &layout->btn_stop.x, &layout->btn_stop.y, &layout->btn_stop.w, &layout->btn_stop.h};
    widgets[count++] = (draggable_widget_t){"btn_freq_down", &layout->btn_freq_down.x, &layout->btn_freq_down.y, &layout->btn_freq_down.w, &layout->btn_freq_down.h};
    widgets[count++] = (draggable_widget_t){"btn_freq_up", &layout->btn_freq_up.x, &layout->btn_freq_up.y, &layout->btn_freq_up.w, &layout->btn_freq_up.h};
    widgets[count++] = (draggable_widget_t){"btn_step_down", &layout->btn_step_down.x, &layout->btn_step_down.y, &layout->btn_step_down.w, &layout->btn_step_down.h};
    widgets[count++] = (draggable_widget_t){"btn_step_up", &layout->btn_step_up.x, &layout->btn_step_up.y, &layout->btn_step_up.w, &layout->btn_step_up.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_2_5", &layout->btn_wwv_2_5.x, &layout->btn_wwv_2_5.y, &layout->btn_wwv_2_5.w, &layout->btn_wwv_2_5.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_5", &layout->btn_wwv_5.x, &layout->btn_wwv_5.y, &layout->btn_wwv_5.w, &layout->btn_wwv_5.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_10", &layout->btn_wwv_10.x, &layout->btn_wwv_10.y, &layout->btn_wwv_10.w, &layout->btn_wwv_10.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_15", &layout->btn_wwv_15.x, &layout->btn_wwv_15.y, &layout->btn_wwv_15.w, &layout->btn_wwv_15.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_20", &layout->btn_wwv_20.x, &layout->btn_wwv_20.y, &layout->btn_wwv_20.w, &layout->btn_wwv_20.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_25", &layout->btn_wwv_25.x, &layout->btn_wwv_25.y, &layout->btn_wwv_25.w, &layout->btn_wwv_25.h};
    widgets[count++] = (draggable_widget_t){"btn_wwv_30", &layout->btn_wwv_30.x, &layout->btn_wwv_30.y, &layout->btn_wwv_30.w, &layout->btn_wwv_30.h};
    widgets[count++] = (draggable_widget_t){"btn_server", &layout->btn_server.x, &layout->btn_server.y, &layout->btn_server.w, &layout->btn_server.h};
    widgets[count++] = (draggable_widget_t){"btn_waterfall", &layout->btn_waterfall.x, &layout->btn_waterfall.y, &layout->btn_waterfall.w, &layout->btn_waterfall.h};
    widgets[count++] = (draggable_widget_t){"btn_aff_dec", &layout->btn_aff_interval_dec.x, &layout->btn_aff_interval_dec.y, &layout->btn_aff_interval_dec.w, &layout->btn_aff_interval_dec.h};
    widgets[count++] = (draggable_widget_t){"btn_aff_inc", &layout->btn_aff_interval_inc.x, &layout->btn_aff_interval_inc.y, &layout->btn_aff_interval_inc.w, &layout->btn_aff_interval_inc.h};
    
    /* Add preset buttons */
    for (int i = 0; i < NUM_PRESETS; i++) {
        widgets[count].name = "btn_preset";
        widgets[count].x_ptr = &layout->btn_preset[i].x;
        widgets[count].y_ptr = &layout->btn_preset[i].y;
        widgets[count].w_ptr = &layout->btn_preset[i].w;
        widgets[count].h_ptr = &layout->btn_preset[i].h;
        count++;
    }
    
    /* Add sliders */
    widgets[count++] = (draggable_widget_t){"slider_gain", &layout->slider_gain.x, &layout->slider_gain.y, &layout->slider_gain.w, &layout->slider_gain.h};
    widgets[count++] = (draggable_widget_t){"slider_lna", &layout->slider_lna.x, &layout->slider_lna.y, &layout->slider_lna.w, &layout->slider_lna.h};
    
    /* Add combos */
    widgets[count++] = (draggable_widget_t){"combo_agc", &layout->combo_agc.x, &layout->combo_agc.y, &layout->combo_agc.w, &layout->combo_agc.h};
    widgets[count++] = (draggable_widget_t){"combo_srate", &layout->combo_srate.x, &layout->combo_srate.y, &layout->combo_srate.w, &layout->combo_srate.h};
    widgets[count++] = (draggable_widget_t){"combo_bw", &layout->combo_bw.x, &layout->combo_bw.y, &layout->combo_bw.w, &layout->combo_bw.h};
    widgets[count++] = (draggable_widget_t){"combo_antenna", &layout->combo_antenna.x, &layout->combo_antenna.y, &layout->combo_antenna.w, &layout->combo_antenna.h};
    
    /* Add toggles (no w/h) */
    widgets[count++] = (draggable_widget_t){"toggle_biast", &layout->toggle_biast.x, &layout->toggle_biast.y, NULL, NULL};
    widgets[count++] = (draggable_widget_t){"toggle_notch", &layout->toggle_notch.x, &layout->toggle_notch.y, NULL, NULL};
    widgets[count++] = (draggable_widget_t){"toggle_aff", &layout->toggle_aff.x, &layout->toggle_aff.y, NULL, NULL};
    
    /* Add panels */
    widgets[count++] = (draggable_widget_t){"panel_freq", &layout->panel_freq.x, &layout->panel_freq.y, &layout->panel_freq.w, &layout->panel_freq.h};
    widgets[count++] = (draggable_widget_t){"panel_gain", &layout->panel_gain.x, &layout->panel_gain.y, &layout->panel_gain.w, &layout->panel_gain.h};
    widgets[count++] = (draggable_widget_t){"panel_config", &layout->panel_config.x, &layout->panel_config.y, &layout->panel_config.w, &layout->panel_config.h};
    widgets[count++] = (draggable_widget_t){"panel_wwv", &layout->panel_wwv.x, &layout->panel_wwv.y, &layout->panel_wwv.w, &layout->panel_wwv.h};
    widgets[count++] = (draggable_widget_t){"panel_bcd", &layout->panel_bcd.x, &layout->panel_bcd.y, &layout->panel_bcd.w, &layout->panel_bcd.h};
    
    /* Add frequency display */
    widgets[count++] = (draggable_widget_t){"freq_display", &layout->freq_display.x, &layout->freq_display.y, &layout->freq_display.w, &layout->freq_display.h};
    
    /* Find widget at click point (reverse order = top first) */
    for (int i = count - 1; i >= 0; i--) {
        int wx = *widgets[i].x_ptr;
        int wy = *widgets[i].y_ptr;
        int ww = widgets[i].w_ptr ? *widgets[i].w_ptr : 50;
        int wh = widgets[i].h_ptr ? *widgets[i].h_ptr : 30;
        
        if (point_in_rect(x, y, wx, wy, ww, wh)) {
            return &widgets[i];
        }
    }
    
    return NULL;
}

/*
 * Handle mouse down in edit mode
 */
void ui_layout_edit_mouse_down(ui_layout_t* layout, int x, int y)
{
    if (!layout || !layout->edit_mode) return;
    
    draggable_widget_t* widget = find_widget_at(layout, x, y);
    if (widget) {
        edit_state.grabbed = widget;
        edit_state.grab_offset_x = x - *widget->x_ptr;
        edit_state.grab_offset_y = y - *widget->y_ptr;
        LOG_INFO("Grabbed: %s at (%d, %d)", widget->name, *widget->x_ptr, *widget->y_ptr);
    }
}

/*
 * Handle mouse move in edit mode
 */
void ui_layout_edit_mouse_move(ui_layout_t* layout, int x, int y)
{
    if (!layout || !layout->edit_mode || !edit_state.grabbed) return;
    
    *edit_state.grabbed->x_ptr = x - edit_state.grab_offset_x;
    *edit_state.grabbed->y_ptr = y - edit_state.grab_offset_y;
}

/*
 * Handle mouse up in edit mode
 */
void ui_layout_edit_mouse_up(ui_layout_t* layout)
{
    if (!layout || !edit_state.grabbed) return;
    
    LOG_INFO("Released: %s at (%d, %d)", 
             edit_state.grabbed->name,
             *edit_state.grabbed->x_ptr,
             *edit_state.grabbed->y_ptr);
    edit_state.grabbed = NULL;
}

/*
 * Dump all widget positions to file
 */
void ui_layout_dump_positions(const ui_layout_t* layout, const char* filename)
{
    if (!layout || !filename) return;
    
    FILE* f = fopen(filename, "w");
    if (!f) {
        LOG_ERROR("Failed to open %s for writing", filename);
        return;
    }
    
    fprintf(f, "# Phoenix SDR Controller - Widget Positions\n");
    fprintf(f, "# Generated layout dump - copy these values back to ui_layout.c\n\n");
    
    fprintf(f, "## Buttons\n");
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_connect:", layout->btn_connect.x, layout->btn_connect.y, layout->btn_connect.w, layout->btn_connect.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_start:", layout->btn_start.x, layout->btn_start.y, layout->btn_start.w, layout->btn_start.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_stop:", layout->btn_stop.x, layout->btn_stop.y, layout->btn_stop.w, layout->btn_stop.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_freq_down:", layout->btn_freq_down.x, layout->btn_freq_down.y, layout->btn_freq_down.w, layout->btn_freq_down.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_freq_up:", layout->btn_freq_up.x, layout->btn_freq_up.y, layout->btn_freq_up.w, layout->btn_freq_up.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_step_down:", layout->btn_step_down.x, layout->btn_step_down.y, layout->btn_step_down.w, layout->btn_step_down.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_step_up:", layout->btn_step_up.x, layout->btn_step_up.y, layout->btn_step_up.w, layout->btn_step_up.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_2_5:", layout->btn_wwv_2_5.x, layout->btn_wwv_2_5.y, layout->btn_wwv_2_5.w, layout->btn_wwv_2_5.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_5:", layout->btn_wwv_5.x, layout->btn_wwv_5.y, layout->btn_wwv_5.w, layout->btn_wwv_5.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_10:", layout->btn_wwv_10.x, layout->btn_wwv_10.y, layout->btn_wwv_10.w, layout->btn_wwv_10.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_15:", layout->btn_wwv_15.x, layout->btn_wwv_15.y, layout->btn_wwv_15.w, layout->btn_wwv_15.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_20:", layout->btn_wwv_20.x, layout->btn_wwv_20.y, layout->btn_wwv_20.w, layout->btn_wwv_20.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_25:", layout->btn_wwv_25.x, layout->btn_wwv_25.y, layout->btn_wwv_25.w, layout->btn_wwv_25.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_wwv_30:", layout->btn_wwv_30.x, layout->btn_wwv_30.y, layout->btn_wwv_30.w, layout->btn_wwv_30.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_server:", layout->btn_server.x, layout->btn_server.y, layout->btn_server.w, layout->btn_server.h);
    fprintf(f, "%-25s x=%-4d y=%-4d w=%-4d h=%-4d\n", "btn_waterfall:", layout->btn_waterfall.x, layout->btn_waterfall.y, layout->btn_waterfall.w, layout->btn_waterfall.h);
    
    fprintf(f, "\n");
    for (int i = 0; i < NUM_PRESETS; i++) {
        fprintf(f, "btn_preset[%d]:            x=%-4d y=%-4d w=%-4d h=%-4d\n",
                i, layout->btn_preset[i].x, layout->btn_preset[i].y,
                layout->btn_preset[i].w, layout->btn_preset[i].h);
    }
    
    fprintf(f, "\n## Sliders\n");
    fprintf(f, "slider_gain:              x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->slider_gain.x, layout->slider_gain.y,
            layout->slider_gain.w, layout->slider_gain.h);
    fprintf(f, "slider_lna:               x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->slider_lna.x, layout->slider_lna.y,
            layout->slider_lna.w, layout->slider_lna.h);
    
    fprintf(f, "\n## Combos\n");
    fprintf(f, "combo_agc:                x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->combo_agc.x, layout->combo_agc.y,
            layout->combo_agc.w, layout->combo_agc.h);
    fprintf(f, "combo_srate:              x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->combo_srate.x, layout->combo_srate.y,
            layout->combo_srate.w, layout->combo_srate.h);
    fprintf(f, "combo_bw:                 x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->combo_bw.x, layout->combo_bw.y,
            layout->combo_bw.w, layout->combo_bw.h);
    fprintf(f, "combo_antenna:            x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->combo_antenna.x, layout->combo_antenna.y,
            layout->combo_antenna.w, layout->combo_antenna.h);
    
    fprintf(f, "\n## Toggles\n");
    fprintf(f, "toggle_biast:             x=%-4d y=%-4d\n",
            layout->toggle_biast.x, layout->toggle_biast.y);
    fprintf(f, "toggle_notch:             x=%-4d y=%-4d\n",
            layout->toggle_notch.x, layout->toggle_notch.y);
    fprintf(f, "toggle_aff:               x=%-4d y=%-4d\n",
            layout->toggle_aff.x, layout->toggle_aff.y);
    
    fprintf(f, "\n## Panels\n");
    fprintf(f, "panel_freq:               x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->panel_freq.x, layout->panel_freq.y,
            layout->panel_freq.w, layout->panel_freq.h);
    fprintf(f, "panel_gain:               x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->panel_gain.x, layout->panel_gain.y,
            layout->panel_gain.w, layout->panel_gain.h);
    fprintf(f, "panel_config:             x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->panel_config.x, layout->panel_config.y,
            layout->panel_config.w, layout->panel_config.h);
    fprintf(f, "panel_wwv:                x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->panel_wwv.x, layout->panel_wwv.y,
            layout->panel_wwv.w, layout->panel_wwv.h);
    fprintf(f, "panel_bcd:                x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->panel_bcd.x, layout->panel_bcd.y,
            layout->panel_bcd.w, layout->panel_bcd.h);
    
    fprintf(f, "\n## Special Widgets\n");
    fprintf(f, "freq_display:             x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->freq_display.x, layout->freq_display.y,
            layout->freq_display.w, layout->freq_display.h);
    
    fprintf(f, "\n## Regions\n");
    fprintf(f, "region_header:            x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.header.x, layout->regions.header.y,
            layout->regions.header.w, layout->regions.header.h);
    fprintf(f, "region_freq_area:         x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.freq_area.x, layout->regions.freq_area.y,
            layout->regions.freq_area.w, layout->regions.freq_area.h);
    fprintf(f, "region_gain_panel:        x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.gain_panel.x, layout->regions.gain_panel.y,
            layout->regions.gain_panel.w, layout->regions.gain_panel.h);
    fprintf(f, "region_wwv_panel:         x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.wwv_panel.x, layout->regions.wwv_panel.y,
            layout->regions.wwv_panel.w, layout->regions.wwv_panel.h);
    fprintf(f, "region_bcd_panel:         x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.bcd_panel.x, layout->regions.bcd_panel.y,
            layout->regions.bcd_panel.w, layout->regions.bcd_panel.h);
    fprintf(f, "region_footer:            x=%-4d y=%-4d w=%-4d h=%-4d\n",
            layout->regions.footer.x, layout->regions.footer.y,
            layout->regions.footer.w, layout->regions.footer.h);
    
    fclose(f);
    LOG_INFO("Widget positions dumped to: %s", filename);
}
