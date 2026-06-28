#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISP_WIDTH (240)
#define DISP_HEIGHT (240)

void disp_init();

void disp_render();
bool disp_ready();
void disp_clear(uint16_t color);

void disp_draw_circle(uint8_t x, uint8_t y, uint8_t diameter, uint16_t color);
void disp_draw_rectangle(uint8_t x, uint8_t y, uint8_t x1, uint8_t y1, uint16_t color);
void disp_draw_circle(uint8_t x, uint8_t y, uint8_t r, uint16_t color);
void disp_draw_text(uint8_t x, uint8_t y, char *t, size_t len, uint16_t color, uint8_t size);
