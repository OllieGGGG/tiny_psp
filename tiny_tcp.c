#include "inttypes.h"
#include "stdlib.h"

#include "pico/stdlib.h"

#include "modem.h"

enum TinyTcpState {
    TcpStateRECVSetup,
    TcpStateRECVWait,
    TcpStateRECVWaitAckTxDone,

    TcpStateSENDIdle,
    TcpStateSENDSend,
    TcpStateSENDWaitTxDone,
    TcpStateSENDWaitAck,
};

enum TinyTcpEvent {
    TinyTcpEventNone,
    TinyTcpEventTxDone,
    TinyTcpEventRxDone,
};

struct __attribute__((packed)) ModemMsg {
    uint32_t seq;
    uint32_t ack;
    uint32_t payload;
};

static uint32_t send_msgs_queue[10];
static size_t send_msgs_w_ptr = 0;
static size_t send_msgs_r_ptr = 0;

static uint32_t recv_msgs_queue[10];
static size_t recv_msgs_w_ptr = 0;
static size_t recv_msgs_r_ptr = 0;

static enum TinyTcpEvent event_queue[10];
static uint8_t event_queue_r_ptr = 0;
static uint8_t event_queue_w_ptr = 0;

static uint32_t mine_seq = 0;
static uint32_t his_seq = 0;

enum TinyTcpState tiny_tcp_state = TcpStateSENDIdle;

static uint64_t send_freq = 0;
static uint64_t recv_freq = 0;

static bool server = 0;

void tiny_tcp_init(uint64_t _send_freq, uint64_t _recv_freq, bool _server) {
    send_freq = _send_freq;
    recv_freq = _recv_freq;
    server = _server;
}

/*
 * This module is responsible to implement reliable communication on top of LORA modem in simple form.
*/
void tiny_tcp_process(enum ModemEvent modem_event) {
    static struct ModemMsg modem_msg;
    static uint64_t recv_wait_us = 0;

    switch (tiny_tcp_state) {
        case TcpStateRECVSetup: {
            modem_set_frequency(recv_freq);
            modem_set_recv_mode();
            recv_wait_us = time_us_64();
            tiny_tcp_state = TcpStateRECVWait;
        } break;
        case TcpStateRECVWait: {
            if (modem_event == ModemEventRxDone && (modem_get_last_msg((uint8_t *)&modem_msg) == 0)) {
                printf("RECV recv msg seq: %" PRIu32 ", his seq: %" PRIu32 " msg ack: %d\n", modem_msg.seq, his_seq, modem_msg.ack);
                if (modem_msg.seq > his_seq) {
                    his_seq = modem_msg.seq;

                    recv_msgs_queue[recv_msgs_w_ptr] = modem_msg.payload;

                    if ((recv_msgs_w_ptr + 1) % 10 != recv_msgs_r_ptr) {
                        recv_msgs_w_ptr = (recv_msgs_w_ptr + 1) % 10;

                        event_queue[event_queue_w_ptr] = TinyTcpEventRxDone;
                        event_queue_w_ptr = (event_queue_w_ptr + 1) % 10;
                    }
                }
                if (modem_msg.ack == mine_seq) {
                    mine_seq++;
                }

                modem_msg.seq = mine_seq;
                modem_msg.ack = his_seq;
                modem_msg.payload = 0;

                modem_set_frequency(send_freq);
                modem_send((uint8_t *)&modem_msg);
                printf("RECV Send with %d seq, %d ack\n", mine_seq, his_seq);
                tiny_tcp_state = TcpStateRECVWaitAckTxDone;

            } else if (((time_us_64() - recv_wait_us) / 1000) > 40) {
                recv_wait_us = time_us_64();
                tiny_tcp_state = TcpStateRECVWait;
            }
        } break;
        case TcpStateRECVWaitAckTxDone: {
            if (modem_event == ModemEventTxDone) {
                tiny_tcp_state = TcpStateSENDIdle;
            }
        } break;
        case TcpStateSENDIdle: {
            if (send_msgs_w_ptr != send_msgs_r_ptr) {
                tiny_tcp_state = TcpStateSENDSend;
            } else if (!server){
                tiny_tcp_state = TcpStateRECVSetup;
            }
        } break;
        case TcpStateSENDSend: {
            modem_msg.seq = mine_seq;
            modem_msg.ack = his_seq;
            modem_msg.payload = send_msgs_queue[send_msgs_r_ptr];
            modem_set_frequency(send_freq);
            modem_send((uint8_t *)&modem_msg);
            tiny_tcp_state = TcpStateSENDWaitTxDone;
        } break;
        case TcpStateSENDWaitTxDone: {
            if (modem_event == ModemEventTxDone) {
                tiny_tcp_state = TcpStateSENDWaitAck;
                modem_set_frequency(recv_freq);
                modem_set_recv_mode();
                recv_wait_us = time_us_64();
            }
        } break;
        case TcpStateSENDWaitAck: {
            if (modem_event == ModemEventRxDone && (modem_get_last_msg((uint8_t *)&modem_msg) == 0)) {
                bool acked_packet = false;
                printf("SEND recv seq: %d, his: %d, ack: %d\n", modem_msg.seq, his_seq, modem_msg.ack);

                // Check if packet is new - if so then we might get payload from - it should be valid and fresh
                if (modem_msg.seq > his_seq) {
                    his_seq = modem_msg.seq;
                }
                if (modem_msg.ack == mine_seq) {
                    acked_packet = true;
                    mine_seq++;
                    send_msgs_r_ptr = (send_msgs_r_ptr + 1) % 10;

                    event_queue[event_queue_w_ptr] = TinyTcpEventTxDone;
                    event_queue_w_ptr = (event_queue_w_ptr + 1) % 10;
                    printf("\n");
                }

                if (acked_packet) {
                    tiny_tcp_state = TcpStateSENDIdle;
                } else {
                    tiny_tcp_state = TcpStateSENDSend;
                }
            } else if (((time_us_64() - recv_wait_us) / 1000) > 80) {
                tiny_tcp_state = TcpStateSENDSend;
                printf("Timeout\n");
            }
        } break;
    }
}

enum TinyTcpEvent tiny_tcp_get_last_event() {
    if (event_queue_r_ptr == event_queue_w_ptr) {
        return TinyTcpEventNone;
    }
    enum TinyTcpEvent event = event_queue[event_queue_r_ptr];
    event_queue_r_ptr = (event_queue_r_ptr + 1) % 10;
    return event;
}

/* Put data into the queue and return, tiny_tcp_process() function will check the queue and send data */
int send(uint32_t payload) {
    if ((send_msgs_w_ptr + 1) % 10 == send_msgs_r_ptr) {
        return -1;
    }
    send_msgs_queue[send_msgs_w_ptr] = payload;
    send_msgs_w_ptr = (send_msgs_w_ptr + 1) % 10;
    return 0;
}

int recv(uint32_t *payload) {
    if (recv_msgs_w_ptr == recv_msgs_r_ptr) {
        return -1;
    }
    *payload = recv_msgs_queue[recv_msgs_r_ptr];
    recv_msgs_r_ptr = (recv_msgs_r_ptr + 1) % 10;
    return 0;
}

