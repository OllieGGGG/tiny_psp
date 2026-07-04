#include <stdio.h>

#include "hardware/timer.h"
#include "hardware/watchdog.h"

#include "buttons.h"
#include "display.h"
#include "gpio.h"
#include "modem.h"

#include "pairing.c"
#include "snake.c"
#include "tiny_tcp.c"

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
//

enum SnakeGameState {
    SnakeGameStateStartGame,
    SnakeGameStateTick,
    SnakeGameStateWaitSync,
};

static uint64_t g_send_freq = 0;
static uint64_t g_recv_freq = 0;

int main() {
    stdio_init_all();

    sleep_ms(800);

    printf("*********START THE CONSOLE*********!!!!\n");

    gpio_mine_init();
    printf("Init GPIO\n");
    disp_init();
    printf("Init display\n");
    modem_init();
    printf("Initialize modem\n");
    buttons_init();
    printf("Init buttons\n");

    struct GameState game = {0};
    uint64_t tick_start = time_us_64();
    uint8_t curr_tick = 0;

    game_reset(&game);

    bool one_time = true;
    bool server;

    while (true) {
        enum ModemEvent modem_event = modem_get_last_event();
        enum ButtonEvent btn_event = buttons_get_last_event();
        enum TinyTcpEvent tiny_tcp_event = tiny_tcp_get_last_event();

        if (btn_event == ButtonEventTLPress) {
            watchdog_reboot(0, 0, 0);
        }

        if (pairing(modem_event, btn_event, &g_send_freq, &g_recv_freq, &server)) {
            continue;
        }

        if (one_time) {
            one_time = false;
            modem_clear_events();
            tiny_tcp_init(g_send_freq, g_recv_freq, server);
        }

        tiny_tcp_process(modem_event);

        static enum SnakeGameState snake_state = SnakeGameStateStartGame;
        // "Server" is the one who play the game and send event to the "client"
        if (server) {
            switch (snake_state) {
                case SnakeGameStateStartGame: {
                    uint32_t payload = (curr_tick << 24) | (0) | (0) | (0);
                    send(payload);
                    snake_state = SnakeGameStateWaitSync;
                } break;
                case SnakeGameStateTick: {
                    if (((time_us_64() - tick_start) / 1000) > SNAKE_STEP_MS) {
                        uint8_t old_food_x = game.food.x;
                        uint8_t old_food_y = game.food.y;

                        tick_start = time_us_64();
                        snake_step(&game, true);

                        if (!game.snake.alive) {
                            game_reset(&game);
                            curr_tick = 0;
                        }

                        uint32_t payload = (curr_tick << 24) | (game.snake.next_dir << 16) | (old_food_x << 8) | (old_food_y);
                        send(payload);
                        snake_state = SnakeGameStateWaitSync;
                    }
                } break;
                case SnakeGameStateWaitSync: {
                    if (tiny_tcp_event == TinyTcpEventTxDone) {
                        snake_state = SnakeGameStateTick;
                        curr_tick++;
                        if (!game.snake.alive) {
                            snake_state = SnakeGameStateStartGame;
                        }
                    }
                } break;
            }
            handle_input(&game.snake, btn_event);
        } else {
            uint32_t payload;
            if (tiny_tcp_event == TinyTcpEventRxDone && (recv(&payload) == 0)) {
                curr_tick = (payload >> 24) & 0xff;
                game.snake.next_dir = (payload >> 16) & 0xff;
                game.food.x = (payload >> 8) & 0xff;
                game.food.y = (payload) & 0xff;

                if (curr_tick == 0) {
                    game_reset(&game);
                } else {
                    snake_step(&game, false);
                }
            }
        }

        if (disp_ready()) {
            draw_game(&game);
        }
    }
}
