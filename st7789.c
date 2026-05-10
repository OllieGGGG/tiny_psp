#include "stdio.h"
#include "stdbool.h"
#include "st7789.h"

static void (*set_dc_pin)(bool state) = NULL;
static void (*set_rst_pin)(bool state) = NULL;
static void (*spi_write)(void *buffer, size_t buffer_size) = NULL;
static void (*sleep_ms)(uint32_t ms) = NULL;

static void st7789_write_command(uint8_t cmd) {
    set_dc_pin(false);
	spi_write(&cmd, 1);
}

static void st7789_write_data(uint8_t *buff, uint32_t buff_size) {
    set_dc_pin(true);
	spi_write(buff, buff_size);
}

static void st7789_write_small_data(uint8_t data) {
    set_dc_pin(true);
	spi_write(&data, 1);
}

static void st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

	/* Column Address set */
	st7789_write_command(ST7789_CASET);
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		st7789_write_data(data, sizeof(data));
	}

	/* Row Address set */
	st7789_write_command(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		st7789_write_data(data, sizeof(data));
	}

	/* Write to RAM, after that display will be ready to receive data and will start from column and row we setup */
	st7789_write_command(ST7789_RAMWR);
}

void st7789_init(void (*set_dc_pin_)(bool), void (*set_rst_pin_)(bool), void (*spi_write_)(void *, size_t), void (*sleep_ms_)(uint32_t)) {
    set_dc_pin = set_dc_pin_;
    set_rst_pin = set_rst_pin_;
    spi_write = spi_write_;
    sleep_ms = sleep_ms_;

    set_dc_pin(false);

    set_rst_pin(false);
    sleep_ms(100);
    set_rst_pin(true);
    sleep_ms(100);
    st7789_write_command(ST7789_SWRESET);
    sleep_ms(100);
    st7789_write_command(ST7789_COLMOD);		//	Set color mode
    st7789_write_small_data(ST7789_COLOR_MODE_16bit);
  	st7789_write_command(0xB2);				//	Porch control
	uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
	st7789_write_data(data, sizeof(data));

    st7789_write_command (ST7789_INVON);		//	Inversion ON
	st7789_write_command (ST7789_SLPOUT);	//	Out of sleep mode
  	st7789_write_command (ST7789_NORON);		//	Normal Display on
  	st7789_write_command (ST7789_DISPON);	//	Main screen turned on

  	sleep_ms(50);
  	st7789_write_command (ST7789_MADCTL);	//	Main screen turned on
    st7789_write_small_data(ST7789_MADCTL_RGB);
}

void st7789_set_position(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;
	if ((x + w - 1) >= ST7789_WIDTH)
		return;
	if ((y + h - 1) >= ST7789_HEIGHT)
		return;

	st7789_set_address_window(x, y, x + w - 1, y + h - 1);
}
