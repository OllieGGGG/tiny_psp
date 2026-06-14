#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

#include "st7789.h"
#include "buttons.h"
#include "display.h"
#include "gpio.h"
#include "modem.h"

#include "pairing.c"

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

static uint64_t g_send_freq = 0;
static uint64_t g_recv_freq = 0;

int main() {
    stdio_init_all();
    sleep_ms(500);

    gpio_mine_init();
    printf("Init GPIO\n");
    disp_init();
    printf("Init display\n");
    modem_init();
    printf("Initialize modem\n");
    buttons_init();
    printf("Init buttons\n");

    uint8_t modem_tmp_msg[MSG_SIZE];
    uint8_t state = 0;
    modem_tmp_msg[0] = state;
    bool wait_tx = false;
    int pos = 0;
    uint64_t start = time_us_64();
    uint8_t cnt = 0;


    uint8_t mine_pos_x = 0;
    uint8_t mine_pos_y = 0;
    uint8_t opp_pos_x = 0;
    uint8_t opp_pos_y = 0;

    while (true) {
        enum ModemEvent modem_event = modem_get_last_event();
        enum ButtonEvent btn_event = buttons_get_last_event();

        /* Make pairing until all good */
        if (pairing(modem_event, btn_event, &g_send_freq, &g_recv_freq)) {
            continue;
        }

        /* Start send coordinates */
        if (wait_tx) {
            if (modem_event == ModemEventTxDone) {
                modem_set_frequency(g_recv_freq);
                modem_set_recv_mode();
                start = time_us_64();
                wait_tx = false;
            }
        } else {
            bool send = false;
            if (modem_event == ModemEventRxDone && (modem_get_last_msg(modem_tmp_msg) == 0)) {
                opp_pos_x = *((uint8_t *)modem_tmp_msg);
                opp_pos_y = *((uint8_t *)modem_tmp_msg + 1);
                printf("Receive coordinates: %d, %d\n", opp_pos_x, opp_pos_y);
                send = true;
            } else if (((time_us_64() - start) / 1000) > 80) {
                send = true;
            }
            if (send) {
                send = false;
                modem_tmp_msg[0] = mine_pos_x;
                modem_tmp_msg[1] = mine_pos_y;
                modem_set_frequency(g_send_freq);
                modem_send(modem_tmp_msg);
                wait_tx = true;
            }
        }

        if (disp_ready()) {
            disp_clear(BLUE);
            disp_draw_rectangle(mine_pos_x, mine_pos_y, mine_pos_x + 20, mine_pos_y + 20, RED);
            disp_draw_rectangle(opp_pos_x, opp_pos_y, opp_pos_x + 20, opp_pos_y + 20, GREEN);
            disp_render();
        }

        switch (btn_event) {
            case ButtonEventLDownPress: {
                mine_pos_x -= 10;
            } break;
            case ButtonEventRDownPress: {
                mine_pos_x += 10;
            } break;
        }
    }
    printf("We are here!!!!\n");
}
