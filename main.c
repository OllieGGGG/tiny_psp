#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

/* TODO(Oleg): remove after */
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "sx127x.h"

#include "st7789.h"
#include "buttons.h"
#include "display.h"

#include "config.h"

//void enable_pwm() {
//    /*
//     * Here is how to setup PWM in rp2040.
//     * So you have you wrap value that will act as limit counter value, so when it reach it will start counting from zero
//     * once again. By taking frequency of PWM block and you can understand what you wrap value should be to work on certain frequency.
//     * And by setting gpio level you can setup the amplitude of your signal. When PWM will counting it still will count until
//     * wrap value, but now when it hits gpio level counter it will setup PWM output to zero.
//     */
//    gpio_set_function(AUDIO, GPIO_FUNC_PWM);
//    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
//    uint slice_num = pwm_gpio_to_slice_num(AUDIO);
//
//    pwm_set_wrap(slice_num, 9000);
//    pwm_set_enabled(slice_num, true);
//    pwm_set_gpio_level(AUDIO, 300);
//}

#define ERROR_CHECK(x) {if (x != 0) printf("Something not good");}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Initialize lora modem\n");

    spi_init(spi0, 5 * 1000 * 1000);
    gpio_set_function(LORA_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LORA_MISO, GPIO_FUNC_SPI);
    gpio_set_function(LORA_SCK, GPIO_FUNC_SPI);

    gpio_init(LORA_CS);
    gpio_set_dir(LORA_CS, GPIO_OUT);
    gpio_put(LORA_CS, 1);

    //gpio_init(LORA_MISO);
    //gpio_set_dir(LORA_MISO, GPIO_OUT);
    //gpio_put(LORA_MISO, 1);

    gpio_init(LORA_RESET);
    gpio_set_dir(LORA_RESET, GPIO_OUT);
    gpio_put(LORA_RESET, 0);
    sleep_ms(100);
    gpio_put(LORA_RESET, 1);
    sleep_ms(100);

    sx127x device;
    sx127x_create(NULL, &device);
    ERROR_CHECK(sx127x_set_opmod(SX127X_MODE_STANDBY, SX127X_MODULATION_LORA, &device));
    ERROR_CHECK(sx127x_set_frequency(868000000, &device));
    ERROR_CHECK(sx127x_lora_reset_fifo(&device));
    ERROR_CHECK(sx127x_lora_set_bandwidth(SX127X_BW_125000, &device));
    ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, &device));
    ERROR_CHECK(sx127x_lora_set_spreading_factor(SX127X_SF_9, &device));
    ERROR_CHECK(sx127x_lora_set_syncword(18, &device));
    ERROR_CHECK(sx127x_set_preamble_length(8, &device));
    //sx127x_tx_set_callback(tx_callback, &device, &device);

    uint8_t reg_value;
    while (true) {
        //sx127x_read_register(0x42, &device.spi_device, &reg_value);
        uint64_t freq;
        sx127x_get_frequency(&device, &freq);
        printf("Frequency: %lld\n", freq);
        //gpio_put(LORA_CS, 0);
        //spi_write_read_blocking(spi0, tx, rx, 2);
        //gpio_put(LORA_CS, 1);
        //printf("nitialize lora modem: %d\n", reg_value);
        sleep_ms(100);
    }

    return 0;
    printf("Init buttons\n");
    buttons_init();
    disp_init();

    int pos = 0;
    uint64_t start = time_us_64();
    while (true) {
        sleep_ms(1);
        if (disp_ready()) {
            disp_clear(BLACK);
            disp_draw_rectangle(pos, 0, pos + 20, 20, RED);
            disp_render();
        }

        enum ButtonEvent btn_event = buttons_get_last_event();
        switch (btn_event) {
            case ButtonEventLDownPress: {
                pos -= 10;
            } break;
            case ButtonEventRDownPress: {
                pos += 10;
            } break;
        }
        //if (frame_cnt % 101 == 0) {
        //    frame_cnt = 0;
        //    uint64_t time = time_us_64() - start;
        //    printf("You got 1000 frames output in %lld ms\n", time / 1000);
        //    start = time_us_64();
        //}
    }
    printf("We are here!!!!\n");
}
