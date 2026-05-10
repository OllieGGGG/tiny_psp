#pragma once

#include <stdint.h>

#define BUTTONS_EVENT_QUEUE_SIZE (16)

enum ButtonEvent {
    ButtonEventNone,

    ButtonEventLDownPress,
    ButtonEventLDownRelease,
    ButtonEventRDownPress,
    ButtonEventRDownRelease,
};

struct BtnGpioEvent {
    uint8_t btn_gpio;
    enum ButtonEvent btn_event_press;
    enum ButtonEvent btn_event_release;
};

void buttons_init();

enum ButtonEvent buttons_get_last_event();
void buttons_clear_events();

