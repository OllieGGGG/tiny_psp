#pragma once

void gpio_mine_register_callback(unsigned int gpio, uint32_t events, gpio_irq_callback_t callback);
void gpio_mine_init();
