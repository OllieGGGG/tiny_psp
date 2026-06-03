#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_HEIGHT (240)
#define DISPLAY_WIDTH (240)
#define CHECK_COORDS_X(x) (x >= 0 && x <= DISPLAY_WIDTH)
#define CHECK_COORDS_Y(y) (y >= 0 && y <= DISPLAY_HEIGHT)

void disp_init();

void disp_render();
bool disp_ready();
void disp_clear(uint16_t color);

void disp_draw_circle(uint8_t x, uint8_t y, uint8_t diameter, uint16_t color);
void disp_draw_rectangle(uint8_t x, uint8_t y, uint8_t x1, uint8_t y1, uint16_t color);
void disp_draw_text(uint8_t x, uint8_t y, uint8_t font_size, const char *text);
