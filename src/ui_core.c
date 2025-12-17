/**
 * Phoenix SDR Controller - UI Core Implementation
 * 
 * Core SDL2 rendering and window management.
 */

#include "ui_core.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Window config file */
#define WINDOW_CONFIG_FILE "phoenix_sdr_window.ini"

/* Font path - will search for a suitable monospace font */
#ifdef _WIN32
    #define FONT_PATH_PRIMARY "C:/Windows/Fonts/consola.ttf"
    #define FONT_PATH_FALLBACK "C:/Windows/Fonts/cour.ttf"
#else
    #define FONT_PATH_PRIMARY "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
    #define FONT_PATH_FALLBACK "/usr/share/fonts/TTF/DejaVuSansMono.ttf"
#endif

/* Helper: Load font with fallback */
static TTF_Font* load_font(const char* primary, const char* fallback, int size)
{
    TTF_Font* font = TTF_OpenFont(primary, size);
    if (!font && fallback) {
        font = TTF_OpenFont(fallback, size);
    }
    if (!font) {
        LOG_ERROR("Failed to load font: %s", TTF_GetError());
    }
    return font;
}

/* Load window position from config file */
static void load_window_position(int* x, int* y, int* w, int* h)
{
    /* Set defaults */
    *x = 0;
    *y = 30;  /* Small offset for title bar */
    *w = WINDOW_WIDTH;
    *h = WINDOW_HEIGHT;
    
    FILE* f = fopen(WINDOW_CONFIG_FILE, "r");
    if (!f) return;
    
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int val;
        if (sscanf(line, "x=%d", &val) == 1) *x = val;
        else if (sscanf(line, "y=%d", &val) == 1) *y = val;
        else if (sscanf(line, "w=%d", &val) == 1) *w = val;
        else if (sscanf(line, "h=%d", &val) == 1) *h = val;
    }
    fclose(f);
    
    /* Sanity check - ensure window is at least partially visible */
    if (*x < -100) *x = 0;
    if (*y < 0) *y = 30;
    if (*w < WINDOW_MIN_WIDTH) *w = WINDOW_WIDTH;
    if (*h < WINDOW_MIN_HEIGHT) *h = WINDOW_HEIGHT;
    
    LOG_INFO("Loaded window position: %d,%d size: %dx%d", *x, *y, *w, *h);
}

/* Save window position to config file */
static void save_window_position(int x, int y, int w, int h)
{
    FILE* f = fopen(WINDOW_CONFIG_FILE, "w");
    if (!f) {
        LOG_ERROR("Failed to save window position");
        return;
    }
    
    fprintf(f, "; Phoenix SDR Controller Window Position\n");
    fprintf(f, "x=%d\n", x);
    fprintf(f, "y=%d\n", y);
    fprintf(f, "w=%d\n", w);
    fprintf(f, "h=%d\n", h);
    fclose(f);
    
    LOG_INFO("Saved window position: %d,%d size: %dx%d", x, y, w, h);
}

/*
 * Initialize UI core
 */
ui_core_t* ui_core_init(const char* title)
{
    /* Initialize SDL2 */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return NULL;
    }
    
    /* Initialize SDL2_ttf */
    if (TTF_Init() < 0) {
        LOG_ERROR("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return NULL;
    }
    
    /* Allocate context */
    ui_core_t* ui = (ui_core_t*)calloc(1, sizeof(ui_core_t));
    if (!ui) {
        LOG_ERROR("Failed to allocate ui_core_t");
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    /* Load saved window position */
    int win_x, win_y, win_w, win_h;
    load_window_position(&win_x, &win_y, &win_w, &win_h);
    
    /* Create window at saved position */
    ui->window = SDL_CreateWindow(
        title ? title : APP_NAME,
        win_x, win_y,
        win_w, win_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    /* Store initial position */
    ui->window_x = win_x;
    ui->window_y = win_y;
    
    if (!ui->window) {
        LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        free(ui);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    /* Set minimum window size */
    SDL_SetWindowMinimumSize(ui->window, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    
    /* Create renderer */
    ui->renderer = SDL_CreateRenderer(ui->window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!ui->renderer) {
        LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(ui->window);
        free(ui);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    /* Enable alpha blending */
    SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_BLEND);
    
    /* Load fonts */
    ui->font_small = load_font(FONT_PATH_PRIMARY, FONT_PATH_FALLBACK, FONT_SIZE_SMALL);
    ui->font_normal = load_font(FONT_PATH_PRIMARY, FONT_PATH_FALLBACK, FONT_SIZE_NORMAL);
    ui->font_large = load_font(FONT_PATH_PRIMARY, FONT_PATH_FALLBACK, FONT_SIZE_LARGE);
    ui->font_freq = load_font(FONT_PATH_PRIMARY, FONT_PATH_FALLBACK, FONT_SIZE_FREQ);
    ui->font_title = load_font(FONT_PATH_PRIMARY, FONT_PATH_FALLBACK, FONT_SIZE_TITLE);
    
    if (!ui->font_normal) {
        LOG_ERROR("Failed to load essential fonts");
        ui_core_shutdown(ui);
        return NULL;
    }
    
    /* Initialize state */
    ui->window_width = WINDOW_WIDTH;
    ui->window_height = WINDOW_HEIGHT;
    ui->running = true;
    ui->last_frame = SDL_GetTicks();
    ui->frame_time = 16;  /* ~60 FPS */
    ui->delta_time = 0.016f;
    
    LOG_INFO("UI Core initialized (%dx%d)", ui->window_width, ui->window_height);
    
    return ui;
}

/*
 * Shutdown UI core
 */
void ui_core_shutdown(ui_core_t* ui)
{
    if (!ui) return;
    
    /* Save window position before closing */
    if (ui->window) {
        int x, y, w, h;
        SDL_GetWindowPosition(ui->window, &x, &y);
        SDL_GetWindowSize(ui->window, &w, &h);
        save_window_position(x, y, w, h);
    }
    
    /* Free fonts */
    if (ui->font_small) TTF_CloseFont(ui->font_small);
    if (ui->font_normal) TTF_CloseFont(ui->font_normal);
    if (ui->font_large) TTF_CloseFont(ui->font_large);
    if (ui->font_freq) TTF_CloseFont(ui->font_freq);
    if (ui->font_title) TTF_CloseFont(ui->font_title);
    
    /* Destroy renderer and window */
    if (ui->renderer) SDL_DestroyRenderer(ui->renderer);
    if (ui->window) SDL_DestroyWindow(ui->window);
    
    free(ui);
    
    TTF_Quit();
    SDL_Quit();
    
    LOG_INFO("UI Core shutdown");
}

/*
 * Begin frame - clear screen and poll events
 */
bool ui_core_begin_frame(ui_core_t* ui, mouse_state_t* mouse)
{
    if (!ui || !ui->running) return false;
    
    /* Calculate delta time */
    uint32_t current_time = SDL_GetTicks();
    ui->frame_time = current_time - ui->last_frame;
    ui->last_frame = current_time;
    ui->delta_time = ui->frame_time / 1000.0f;
    
    /* Reset per-frame state */
    ui->last_key = 0;
    
    /* Reset per-frame mouse state */
    if (mouse) {
        mouse->left_clicked = false;
        mouse->right_clicked = false;
        mouse->left_released = false;
        mouse->wheel_y = 0;
    }
    
    /* Poll events */
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ui->running = false;
                return false;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    ui->window_width = event.window.data1;
                    ui->window_height = event.window.data2;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (mouse) {
                    mouse->x = event.motion.x;
                    mouse->y = event.motion.y;
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (mouse) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse->left_down = true;
                        mouse->left_clicked = true;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        mouse->right_down = true;
                        mouse->right_clicked = true;
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (mouse) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse->left_down = false;
                        mouse->left_released = true;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        mouse->right_down = false;
                    }
                }
                break;
                
            case SDL_MOUSEWHEEL:
                if (mouse) {
                    mouse->wheel_y = event.wheel.y;
                }
                break;
                
            case SDL_KEYDOWN:
                /* Store last key for external handling */
                ui->last_key = event.key.keysym.sym;
                break;
        }
    }
    
    /* Clear screen with background color */
    ui_set_color(ui, COLOR_BG_DARK);
    SDL_RenderClear(ui->renderer);
    
    return true;
}

/*
 * End frame - present and cap FPS
 */
void ui_core_end_frame(ui_core_t* ui)
{
    if (!ui) return;
    
    SDL_RenderPresent(ui->renderer);
    
    /* Cap at ~60 FPS if vsync not working */
    uint32_t frame_duration = SDL_GetTicks() - ui->last_frame;
    if (frame_duration < 16) {
        SDL_Delay(16 - frame_duration);
    }
}

/*
 * Set draw color from RGBA hex (0xRRGGBBAA)
 */
void ui_set_color(ui_core_t* ui, uint32_t rgba)
{
    if (!ui || !ui->renderer) return;
    
    Uint8 r = (rgba >> 24) & 0xFF;
    Uint8 g = (rgba >> 16) & 0xFF;
    Uint8 b = (rgba >> 8) & 0xFF;
    Uint8 a = rgba & 0xFF;
    
    SDL_SetRenderDrawColor(ui->renderer, r, g, b, a);
}

/*
 * Draw filled rectangle
 */
void ui_draw_rect(ui_core_t* ui, int x, int y, int w, int h, uint32_t color)
{
    if (!ui || !ui->renderer) return;
    
    ui_set_color(ui, color);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(ui->renderer, &rect);
}

/*
 * Draw rectangle outline
 */
void ui_draw_rect_outline(ui_core_t* ui, int x, int y, int w, int h, uint32_t color)
{
    if (!ui || !ui->renderer) return;
    
    ui_set_color(ui, color);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(ui->renderer, &rect);
}

/*
 * Draw rounded rectangle (simplified - draws regular rect for now)
 */
void ui_draw_rounded_rect(ui_core_t* ui, int x, int y, int w, int h, int radius, uint32_t color)
{
    if (!ui || !ui->renderer) return;
    
    /* For simplicity, just draw a regular rectangle */
    /* A proper implementation would draw arcs at corners */
    (void)radius;
    ui_draw_rect(ui, x, y, w, h, color);
}

/*
 * Draw line
 */
void ui_draw_line(ui_core_t* ui, int x1, int y1, int x2, int y2, uint32_t color)
{
    if (!ui || !ui->renderer) return;
    
    ui_set_color(ui, color);
    SDL_RenderDrawLine(ui->renderer, x1, y1, x2, y2);
}

/*
 * Draw horizontal gradient
 */
void ui_draw_gradient_h(ui_core_t* ui, int x, int y, int w, int h, uint32_t color1, uint32_t color2)
{
    if (!ui || !ui->renderer || w <= 0) return;
    
    /* Extract color components */
    Uint8 r1 = (color1 >> 24) & 0xFF;
    Uint8 g1 = (color1 >> 16) & 0xFF;
    Uint8 b1 = (color1 >> 8) & 0xFF;
    Uint8 a1 = color1 & 0xFF;
    
    Uint8 r2 = (color2 >> 24) & 0xFF;
    Uint8 g2 = (color2 >> 16) & 0xFF;
    Uint8 b2 = (color2 >> 8) & 0xFF;
    Uint8 a2 = color2 & 0xFF;
    
    /* Draw vertical lines with interpolated colors */
    for (int i = 0; i < w; i++) {
        float t = (float)i / (float)(w - 1);
        Uint8 r = (Uint8)(r1 + t * (r2 - r1));
        Uint8 g = (Uint8)(g1 + t * (g2 - g1));
        Uint8 b = (Uint8)(b1 + t * (b2 - b1));
        Uint8 a = (Uint8)(a1 + t * (a2 - a1));
        
        SDL_SetRenderDrawColor(ui->renderer, r, g, b, a);
        SDL_RenderDrawLine(ui->renderer, x + i, y, x + i, y + h - 1);
    }
}

/*
 * Draw vertical gradient
 */
void ui_draw_gradient_v(ui_core_t* ui, int x, int y, int w, int h, uint32_t color1, uint32_t color2)
{
    if (!ui || !ui->renderer || h <= 0) return;
    
    /* Extract color components */
    Uint8 r1 = (color1 >> 24) & 0xFF;
    Uint8 g1 = (color1 >> 16) & 0xFF;
    Uint8 b1 = (color1 >> 8) & 0xFF;
    Uint8 a1 = color1 & 0xFF;
    
    Uint8 r2 = (color2 >> 24) & 0xFF;
    Uint8 g2 = (color2 >> 16) & 0xFF;
    Uint8 b2 = (color2 >> 8) & 0xFF;
    Uint8 a2 = color2 & 0xFF;
    
    /* Draw horizontal lines with interpolated colors */
    for (int i = 0; i < h; i++) {
        float t = (float)i / (float)(h - 1);
        Uint8 r = (Uint8)(r1 + t * (r2 - r1));
        Uint8 g = (Uint8)(g1 + t * (g2 - g1));
        Uint8 b = (Uint8)(b1 + t * (b2 - b1));
        Uint8 a = (Uint8)(a1 + t * (a2 - a1));
        
        SDL_SetRenderDrawColor(ui->renderer, r, g, b, a);
        SDL_RenderDrawLine(ui->renderer, x, y + i, x + w - 1, y + i);
    }
}

/*
 * Render text (returns width)
 */
int ui_draw_text(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, uint32_t color)
{
    if (!ui || !ui->renderer || !font || !text || !*text) return 0;
    
    /* Create color struct */
    SDL_Color sdl_color = {
        (color >> 24) & 0xFF,
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF
    };
    
    /* Render text to surface */
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, sdl_color);
    if (!surface) {
        return 0;
    }
    
    /* Create texture from surface */
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
    int width = surface->w;
    int height = surface->h;
    SDL_FreeSurface(surface);
    
    if (!texture) {
        return 0;
    }
    
    /* Render texture */
    SDL_Rect dest = {x, y, width, height};
    SDL_RenderCopy(ui->renderer, texture, NULL, &dest);
    SDL_DestroyTexture(texture);
    
    return width;
}

/*
 * Render text centered horizontally
 */
void ui_draw_text_centered(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, int w, uint32_t color)
{
    if (!font || !text || !*text) return;
    
    int text_w, text_h;
    TTF_SizeText(font, text, &text_w, &text_h);
    
    int centered_x = x + (w - text_w) / 2;
    ui_draw_text(ui, font, text, centered_x, y, color);
}

/*
 * Render text right-aligned
 */
void ui_draw_text_right(ui_core_t* ui, TTF_Font* font, const char* text, int x, int y, int w, uint32_t color)
{
    if (!font || !text || !*text) return;
    
    int text_w, text_h;
    TTF_SizeText(font, text, &text_w, &text_h);
    
    int right_x = x + w - text_w;
    ui_draw_text(ui, font, text, right_x, y, color);
}

/*
 * Get text size
 */
void ui_get_text_size(TTF_Font* font, const char* text, int* w, int* h)
{
    if (!font || !text) {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
    }
    
    TTF_SizeText(font, text, w, h);
}

/*
 * Check if point is in rectangle
 */
bool ui_point_in_rect(int px, int py, int x, int y, int w, int h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

/*
 * Get current ticks (milliseconds)
 */
uint32_t ui_get_ticks(void)
{
    return SDL_GetTicks();
}

/*
 * Delay for specified milliseconds
 */
void ui_delay(uint32_t ms)
{
    SDL_Delay(ms);
}
