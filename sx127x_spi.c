#include "stdio.h"
#include "string.h"
#include "sx127x_spi.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "config.h"

int sx127x_spi_read_registers(int reg, void *spi_device, size_t data_length, uint32_t *result) {
    if (data_length == 0 || data_length > 4) {
        return -1;
    }
    *result = 0;
    uint8_t tx[5] = {reg & 0x7F, 0};
    uint8_t rx[5] = { 0 };
    gpio_put(LORA_CS, 0);
    spi_write_read_blocking(spi0, tx, rx, data_length + 1);
    gpio_put(LORA_CS, 1);
    for (int i = 1; i < data_length + 1; i++) {
        *result = ((*result) << 8);
        *result = (*result) + rx[i];
    }
    return 0;
}

int sx127x_spi_write_register(int reg, const uint8_t *data, size_t data_length, void *spi_device) {
    if (data_length == 0 || data_length > 4) {
        return -1;
    }
    uint8_t tx[5] = {reg | 0x80};
    uint8_t rx[5];
    memcpy(&tx[1], data, data_length);

    gpio_put(LORA_CS, 0);
    spi_write_read_blocking(spi0, tx, rx, data_length + 1);
    gpio_put(LORA_CS, 1);

    return 0;
}
