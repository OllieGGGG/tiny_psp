#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"

#include "st7789.h"
#include "config.h"
#include "fonts.h"

#define DISPLAY_HEIGHT (240)
#define DISPLAY_WIDTH (240)
#define CHECK_COORDS_X(x) (x >= 0 && x <= DISPLAY_WIDTH)
#define CHECK_COORDS_Y(y) (y >= 0 && y <= DISPLAY_HEIGHT)

static uint16_t disp_buffer[240 * 240];

static uint dma_tx;
static bool dma_ready = true;

static void set_dc_pin(bool state) {
    gpio_put(LCD_DC, state);
}

static void set_rst_pin(bool state) {
    gpio_put(LCD_RST, state);
}

static void spi_write(void *buffer, size_t size) {
    gpio_put(LCD_CS, 0);
    spi_write_blocking(spi1, buffer, size);
    gpio_put(LCD_CS, 1);
}

static void dma_handler() {
    gpio_put(LCD_CS, 1);
    dma_hw->ints0 = 1u << dma_tx;
    dma_ready = true;
}

void disp_init() {
    spi_init(spi1, 50 * 1000 * 1000);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);

    gpio_init(LCD_CS);
    gpio_set_dir(LCD_CS, GPIO_OUT);
    gpio_put(LCD_CS, 1);

    gpio_init(LCD_DC);
    gpio_set_dir(LCD_DC, GPIO_OUT);

    gpio_init(LCD_RST);
    gpio_set_dir(LCD_RST, GPIO_OUT);

    st7789_init(set_dc_pin, set_rst_pin, spi_write, sleep_ms);

    dma_tx = dma_claim_unused_channel(true);
    dma_channel_config_t dma_ch_config = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&dma_ch_config, DMA_SIZE_8);
    channel_config_set_dreq(&dma_ch_config, spi_get_dreq(spi1, true));
    channel_config_set_read_increment(&dma_ch_config, true);
    channel_config_set_write_increment(&dma_ch_config, false);
    dma_channel_configure(dma_tx, &dma_ch_config, &spi_get_hw(spi1)->dr, disp_buffer, sizeof(disp_buffer), false);

    dma_channel_set_irq0_enabled(dma_tx, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void disp_render() {
    st7789_set_position(0, 0, 240, 240);
    gpio_put(LCD_DC, 1);
    gpio_put(LCD_CS, 0);
    dma_channel_set_read_addr(dma_tx, disp_buffer, true);
}

void disp_clear(uint16_t color) {
    for (int i = 0; i < sizeof(disp_buffer) / sizeof(disp_buffer[0]); i++) {
        disp_buffer[i] = ((color & 0xff00) >> 8) | ((color & 0XFF) << 8);
    }
}

bool disp_ready() {
    return !dma_channel_is_busy(dma_tx);
}

void disp_draw_rectangle(uint8_t x, uint8_t y, uint8_t x1, uint8_t y1, uint16_t color) {
    if (!CHECK_COORDS_X(x)) return;
    if (!CHECK_COORDS_Y(y)) return;
    if (!CHECK_COORDS_X(x1)) return;
    if (!CHECK_COORDS_Y(y1)) return;

    for (uint32_t col = x; col < x1; col++) {
        for (uint32_t row = y; row < y1; row++) {
            disp_buffer[col + row * DISPLAY_WIDTH] = ((color & 0xff00) >> 8) | ((color & 0XFF) << 8);
        }
    }
}

void disp_draw_circle(uint8_t x, uint8_t y, uint8_t r, uint16_t color) {
    for (uint8_t col = x - r; col < x + r; col++) {
        for (uint8_t row = y - r; row < y + r; row++) {
            uint8_t dx = abs(x - col);
            uint8_t dy = abs(y - row);
            if (dx * dx + dy * dy <= r * r) {
                disp_buffer[col + row * DISPLAY_WIDTH] = ((color & 0xff00) >> 8) | ((color & 0XFF) << 8);
            }
        }
    }
}

void disp_draw_text(uint8_t x, uint8_t y, char *t, size_t len, uint16_t color, uint8_t size) {
    /* Iterate over symbols to write as text */
    for (uint8_t symbol_idx = 0; symbol_idx < len; symbol_idx++) {

        /* Iterate over 8x8 symbol */
        for (uint8_t font_row = 0; font_row < 8; font_row++) {
            for (uint8_t font_col = 0; font_col < 8; font_col++) {
                if (font8x8_basic[(int)(t[symbol_idx])][font_row] & (1 << font_col)) {
                    for (uint8_t pixel_row = 0; pixel_row < size; pixel_row++) {
                        for (uint8_t pixel_col = 0; pixel_col < size; pixel_col++) {
                            uint32_t pixel_idx = x + (symbol_idx * 8 * size) + font_col * size + (y + pixel_row + font_row * size) * DISPLAY_WIDTH + pixel_col;
                            disp_buffer[pixel_idx] = ((color & 0xff00) >> 8) | ((color & 0XFF) << 8);
                        }
                    }
                }
            }
        }
    }
}
