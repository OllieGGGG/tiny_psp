#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MODEM_EVENT_QUEUE_SIZE (10)
#define RX_MSGS_QUEUE_SIZE (10)
#define MSG_SIZE (8)
#define MAIN_FREQUENCY (862000000)

enum ModemEvent {
    ModemEventNone = 0x1,
    ModemEventTxDone,
    ModemEventRxDone,
};

int modem_init();
/* Send message immediately without waiting for signal about Tx done */
int modem_send(uint8_t *buf);
enum ModemEvent modem_get_last_event();
void modem_clear_events();

int modem_get_last_msg(uint8_t *buf);

int modem_set_recv_mode();
int modem_set_standby_mode();
int modem_set_frequency(uint64_t freq);

