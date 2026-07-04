#include "stdio.h"
#include "hardware/gpio.h"

#include "pico/stdlib.h"

#include "buttons.h"
#include "config.h"
#include "gpio.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static enum ButtonEvent event_queue[BUTTONS_EVENT_QUEUE_SIZE];
static uint8_t event_queue_r_ptr = 0;
static uint8_t event_queue_w_ptr = 0;

struct BtnGpioEvent btn_gpio_event_table[] = {
    {.btn_gpio = BTN_R_DOWN, .btn_event_press = ButtonEventRDownPress, .btn_event_release = ButtonEventRDownRelease},

    {.btn_gpio = BTN_L_DOWN, .btn_event_press = ButtonEventLDownPress, .btn_event_release = ButtonEventLDownRelease},
    {.btn_gpio = BTN_L_UP,    .btn_event_press = ButtonEventLUpPress, .btn_event_release = ButtonEventLUpRelease},
    {.btn_gpio = BTN_L_RIGHT,    .btn_event_press = ButtonEventLRightPress, .btn_event_release = ButtonEventLRightRelease},
    {.btn_gpio = BTN_L_LEFT,    .btn_event_press = ButtonEventLLeftPress, .btn_event_release = ButtonEventLLeftRelease},

    {.btn_gpio = BTN_TR_DOWN, .btn_event_press = ButtonEventTRPress, .btn_event_release = ButtonEventNone},
    {.btn_gpio = BTN_TL_DOWN, .btn_event_press = ButtonEventTLPress, .btn_event_release = ButtonEventNone},
};

static struct {
    uint8_t btn_gpio;
    bool state;
    uint8_t table_idx;
} debounce_btn_state;

static int64_t debounce_callback(alarm_id_t id, void *user_data) {
    struct BtnGpioEvent *gpio_event = (struct BtnGpioEvent *)user_data;
    printf("Gpio event: %d\n", gpio_event->btn_event_press);
    if (gpio_get(debounce_btn_state.btn_gpio) == debounce_btn_state.state) {
        if (debounce_btn_state.state) event_queue[event_queue_w_ptr] = btn_gpio_event_table[debounce_btn_state.table_idx].btn_event_release;
        else event_queue[event_queue_w_ptr] = btn_gpio_event_table[debounce_btn_state.table_idx].btn_event_press;
        event_queue_w_ptr = (event_queue_w_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE;
    }
    return 0;
}

static void gpio_callback(uint gpio, uint32_t events) {
    if (((event_queue_w_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE) == event_queue_r_ptr) {
        return;
    }

    for (uint8_t btn_gpio_idx = 0; btn_gpio_idx < ARRAY_SIZE(btn_gpio_event_table); btn_gpio_idx++) {
        if (btn_gpio_event_table[btn_gpio_idx].btn_gpio == gpio) {
            debounce_btn_state.btn_gpio = gpio;
            debounce_btn_state.state = gpio_get(gpio);
            debounce_btn_state.table_idx = btn_gpio_idx;
            add_alarm_in_ms(70, debounce_callback, &(btn_gpio_event_table[btn_gpio_idx]), false);
            break;
        }
    }
}

void buttons_init() {
    for (uint8_t btn_gpio_idx = 0; btn_gpio_idx < ARRAY_SIZE(btn_gpio_event_table); btn_gpio_idx++) {
        uint8_t btn_gpio = btn_gpio_event_table[btn_gpio_idx].btn_gpio;
        gpio_init(btn_gpio);
        gpio_set_dir(btn_gpio, GPIO_IN);
        gpio_pull_up(btn_gpio);
    }

    gpio_mine_register_callback(BTN_L_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);
    gpio_mine_register_callback(BTN_L_UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);
    gpio_mine_register_callback(BTN_L_RIGHT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);
    gpio_mine_register_callback(BTN_L_LEFT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);

    gpio_mine_register_callback(BTN_R_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);

    gpio_mine_register_callback(BTN_TR_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);
    gpio_mine_register_callback(BTN_TL_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, &gpio_callback);
}

enum ButtonEvent buttons_get_last_event() {
    if (event_queue_r_ptr == event_queue_w_ptr) {
        return ButtonEventNone;
    }
    enum ButtonEvent btn_event = event_queue[event_queue_r_ptr];
    event_queue_r_ptr = (event_queue_r_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE;
    return btn_event;
}

