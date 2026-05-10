#include <stdio.h>

#include "hardware/gpio.h"

#include "buttons.h"
#include "config.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static enum ButtonEvent event_queue[BUTTONS_EVENT_QUEUE_SIZE];
static uint8_t event_queue_r_ptr = 0;
static uint8_t event_queue_w_ptr = 0;

struct BtnGpioEvent btn_gpio_event_table[] = {
    {.btn_gpio = BTN_L_DOWN, .btn_event_press = ButtonEventRDownPress, .btn_event_release = ButtonEventRDownRelease},
    {.btn_gpio = BTN_R_DOWN, .btn_event_press = ButtonEventLDownPress, .btn_event_release = ButtonEventLDownRelease}
};

static void gpio_callback(uint gpio, uint32_t events) {
    if (((event_queue_w_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE) == event_queue_r_ptr) {
        return;
    }

    printf("Gpio callback\n");
    for (uint8_t btn_gpio_idx = 0; btn_gpio_idx < ARRAY_SIZE(btn_gpio_event_table); btn_gpio_idx++) {
        if (btn_gpio_event_table[btn_gpio_idx].btn_gpio == gpio) {
            if (events == GPIO_IRQ_EDGE_FALL) event_queue[event_queue_w_ptr] = btn_gpio_event_table[btn_gpio_idx].btn_event_press;
            else if (events == GPIO_IRQ_EDGE_RISE) event_queue[event_queue_w_ptr] = btn_gpio_event_table[btn_gpio_idx].btn_event_release;
            event_queue_w_ptr = (event_queue_w_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE;
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

    gpio_set_irq_enabled_with_callback(BTN_R_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BTN_L_DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

enum ButtonEvent buttons_get_last_event() {
    if (event_queue_r_ptr == event_queue_w_ptr) {
        return ButtonEventNone;
    }
    enum ButtonEvent btn_event = event_queue[event_queue_r_ptr];
    event_queue_r_ptr = (event_queue_r_ptr + 1) % BUTTONS_EVENT_QUEUE_SIZE;
    return btn_event;
}

