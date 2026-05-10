#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

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

int main() {
    stdio_init_all();

    printf("Init buttons\n");
    buttons_init();
    disp_init();

    int pos = 0;
    uint64_t start = time_us_64();
    while (true) {
        sleep_ms(1);
        if (disp_ready()) {
            disp_clear(BLUE);
            disp_draw_rectangle(pos, 0, pos + 20, 20, GREEN);
            disp_render();
        }

        enum ButtonEvent btn_event = buttons_get_last_event();
        switch (btn_event) {
            case ButtonEventLDownPress: {
                pos -= 5;
            } break;
            case ButtonEventRDownPress: {
                pos += 5;
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
