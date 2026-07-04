#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"

#include "st7789.h"
#include "modem.h"
#include "buttons.h"
#include "display.h"

enum PairingState {
    PairingStateIdle = 0x1,
    PairingStateListen,
    PairingStateListenSendAck,
    PairingStateListenSendAckWaitTx,
    PairingStateListenReady,

    PairingStateAdvertise,
    PairingStateAdvertiseWaitTx,
    PairingStateAdvertiseRx,
    PairingStateAdvertiseReady,
};

int pairing(enum ModemEvent modem_event, enum ButtonEvent btn_event, uint64_t *send_freq, uint64_t *recv_freq, bool *server) {
    /* Bind logic:
     * Choose either ADVERTISE either LISTEN ->
     * Advertise packets on certain frequency ->
     * Listener will receive packets ->
     * Listener will acknowledge with new frequencies devices should use
     */
    static enum PairingState pairing_state = PairingStateIdle;

    static uint64_t timer_start;
    uint8_t modem_tmp_msg[MSG_SIZE];
    static uint32_t listen_send_ack_times;

    int rc = -1;

    switch (pairing_state) {
        case PairingStateIdle: {
            disp_clear(BLACK);
            disp_draw_text(0, 180, "Advertiser", 10, BLUE, 1);
            disp_draw_text(DISP_WIDTH - 65, 180, "Listener", 8, BLUE, 1);

            if (btn_event == ButtonEventLDownPress) {
                pairing_state = PairingStateAdvertise;
                disp_clear(BLACK);
                disp_draw_text(0, 60, "Advertiser", 10, BLUE, 1);
                *server = true;
            } else if (btn_event == ButtonEventRDownPress) {
                pairing_state = PairingStateListen;
                disp_clear(BLACK);
                disp_draw_text(120, 180, "Listener", 8, BLUE, 1);
                modem_set_recv_mode();
                *server = false;
            }
            disp_render();
            sleep_ms(1000);
        } break;

        /********** Advertiser part **********/
        case PairingStateAdvertise: {
            *send_freq = MAIN_FREQUENCY;
            memcpy(modem_tmp_msg, &*send_freq, sizeof(*send_freq));
            modem_send(modem_tmp_msg);
            pairing_state = PairingStateAdvertiseWaitTx;
        } break;
        case PairingStateAdvertiseWaitTx: {
            if (modem_event == ModemEventTxDone) {
                pairing_state = PairingStateAdvertiseRx;
                modem_set_recv_mode();
                timer_start = time_us_64();
            }
        } break;
        case PairingStateAdvertiseRx: {
            if ((modem_event == ModemEventRxDone) && (modem_get_last_msg(modem_tmp_msg) == 0)) {
                pairing_state = PairingStateAdvertiseReady;
                *recv_freq = *((uint64_t *)modem_tmp_msg);
            } else if (((time_us_64() - timer_start) / 1000) > 200) {
                timer_start = time_us_64();
                pairing_state = PairingStateAdvertise;
                modem_set_standby_mode();
                modem_clear_events();
            }
        } break;
        case PairingStateAdvertiseReady: {
            rc = 0;
        } break;

        /********** Listener part **********/
        case PairingStateListen: {
            if ((modem_event == ModemEventRxDone) && (modem_get_last_msg(modem_tmp_msg) == 0)) {
                *recv_freq = *((uint64_t *)modem_tmp_msg);
                pairing_state = PairingStateListenSendAck;
                listen_send_ack_times = 10; 
            }
        } break;
        case PairingStateListenSendAck: {
            if (listen_send_ack_times-- > 0) {
                *send_freq = MAIN_FREQUENCY - 1000000;
                memcpy(modem_tmp_msg, &*send_freq, sizeof(*send_freq));
                modem_send(modem_tmp_msg);
                pairing_state = PairingStateListenSendAckWaitTx;
            } else {
                pairing_state = PairingStateListenReady;
            }
        } break;
        case PairingStateListenSendAckWaitTx: {
            if (modem_event == ModemEventTxDone) {
                pairing_state = PairingStateListenSendAck;
            }
        } break;
        case PairingStateListenReady: {
            rc = 0;
        } break;
    }

    return rc;
}
