#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "config.h"

void drv_spi_init() {
    spi_init(spi1, 50 * 1000 * 1000);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);

    gpio_init(LCD_CS);
    gpio_set_dir(LCD_CS, GPIO_OUT);
    gpio_put(LCD_CS, 1);
}

void spi_write(void *buf, size_t size) {
    gpio_put(LCD_CS, 0);
    spi_write_blocking(spi1, buf, size);
    gpio_put(LCD_CS, 1);
}
