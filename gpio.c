#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "hardware/gpio.h"

typedef void (*gpio_callback_t)(unsigned int gpio, uint32_t events);
gpio_callback_t gpio_callbacks[64] = { NULL };

static void gpio_callback(uint gpio, uint32_t event_mask) {
    if (gpio_callbacks[gpio] != NULL) {
        gpio_callbacks[gpio](gpio, event_mask);
    }
}

void gpio_mine_register_callback(unsigned int gpio, uint32_t events, gpio_irq_callback_t callback) {
    assert(gpio < sizeof(gpio_callbacks));
    gpio_callbacks[gpio] = callback;
    //gpio_set_irq_enabled(gpio, events, true);
    gpio_set_irq_enabled_with_callback(gpio, events, true, &gpio_callback);
}

void gpio_mine_init() {
    gpio_set_irq_callback(&gpio_callback);
}
