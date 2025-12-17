/**
 * Phoenix SDR Controller - UI Core
 * 
 * Core SDL2 rendering and window management.
 */

#ifndef UI_CORE_H
#define UI_CORE_H

#include "common.h"
#include <SDL.h>
#include <SDL_ttf.h>

/* Window dimensions */
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 480
#define WINDOW_MIN_WIDTH 640
#define WINDOW_MIN_HEIGHT 400

/* Font sizes */
#define FONT_SIZE_SMALL 11
#define FONT_SIZE_NORMAL 13
#define FONT_SIZE_LARGE 16
#define FONT_SIZE_FREQ 32
#define FONT_SIZE_TITLE 18

/* UI Core context */
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_small;
    TTF_Font* font_normal;
    TTF_Font* font_large;
    TTF_Font* font_freq;
    TTF_Font* font_title;
    int window_width;
    int window_height;
    int window_x;        /* Window X position */
    int window_y;        /* Window Y position */
    bool running;
    uint32_t frame_time;
    uint32_t last_frame;
    float delta_time;
    int last_key;        /* Last key pressed (SDL_Keycode, 0 if none) */
} ui_core_t;

/* Mouse state */
typedef struct {
    int x, y;
    bool left_down;
    bool right_down;
    bool left_clicked;
    bool right_clicked;
    bool left_released;
    int wheel_y;
} mouse_state_t;

/* Initialize UI core */
ui_core_t* ui_core_init(const char* title);

/* Shutdown UI core */
void ui_core_shutdown(ui_core_t* ui);

/* Begin frame (clear screen, poll events) */
bool ui_core_begin_frame(ui_core_t* ui, mouse_state_t* mouse);

/* End frame (present) */
void ui_core_end_frame(ui_core_t* ui);

/* Set draw color from RGBA hex */
void ui_set_color(ui_core_t* ui, uint32_t rgba);

/* Draw filled rectangle */
void ui_draw_rect(ui_core_t* ui, int x, int y, int w, int h, uint32_t color);

/* Draw rectangle outline */
void ui_draw_rect_outline(ui_core_t* ui, int x, int y, int w, int h, uint32_t color);

/* Draw rounded rectangle */
void ui_draw_rounded_rect(ui_core_t* ui, int x, int y, int w, int h, int radius, uint32_t color);

/* Draw line */
void ui_draw_line(ui_core_t* ui, int x1, int y1, int x2, int y2, uint32_t color);

/* Draw horizontal gradient */
void ui_draw_gradient_h(ui_core_t* ui, int x, int y, int w, int h, uint32_t color1, uint32_t color2);

/* Draw vertical gradient */
void ui_draw_gradient_v(ui_core_t* ui, int x, int y, int w, int h, uint32_t color1, uint32_t color2);

/* Render text (returns width) */
int ui_draw_text(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, uint32_t color);

/* Render text centered */
void ui_draw_text_centered(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, int w, uint32_t color);

/* Render text right-aligned */
void ui_draw_text_right(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, int w, uint32_t color);

/* Get text size */
void ui_get_text_size(TTF_Font* font, const char* text, int* w, int* h);

/* Check if point is in rectangle */
bool ui_point_in_rect(int px, int py, int x, int y, int w, int h);

/* Get current ticks */
uint32_t ui_get_ticks(void);

/* Delay */
void ui_delay(uint32_t ms);

#endif /* UI_CORE_H */
