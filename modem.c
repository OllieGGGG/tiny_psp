#include "stdio.h"
#include "string.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "sx127x.h"
#include "config.h"
#include "modem.h"
#include "gpio.h"

#define ERROR_CHECK(x) {if (x != 0) return -1;}

static enum ModemEvent event_queue[MODEM_EVENT_QUEUE_SIZE];
static uint8_t event_queue_r_ptr = 0;
static uint8_t event_queue_w_ptr = 0;

static uint8_t rx_msgs_queue[RX_MSGS_QUEUE_SIZE][MSG_SIZE];
static uint8_t rx_msgs_r_ptr = 0;
static uint8_t rx_msgs_w_ptr = 0;

static sx127x modem;

static void lora_dio0_cb(uint gpio, uint32_t events) {
    sx127x_handle_interrupt(&modem);
}

static void tx_callback(void *ctx) {
    if (((event_queue_w_ptr + 1) % MODEM_EVENT_QUEUE_SIZE) == event_queue_r_ptr) {
        return;
    }
    event_queue[event_queue_w_ptr] = ModemEventTxDone;
    event_queue_w_ptr = (event_queue_w_ptr + 1) % MODEM_EVENT_QUEUE_SIZE;
}

static void rx_callback(void *ctx, uint8_t *data, uint16_t sz) {
    if (((event_queue_w_ptr + 1) % MODEM_EVENT_QUEUE_SIZE) == event_queue_r_ptr) {
        return;
    }
    event_queue[event_queue_w_ptr] = ModemEventRxDone;
    event_queue_w_ptr = (event_queue_w_ptr + 1) % MODEM_EVENT_QUEUE_SIZE;

    memcpy(&rx_msgs_queue[rx_msgs_w_ptr], data, MSG_SIZE);
    rx_msgs_w_ptr = (rx_msgs_w_ptr + 1) % RX_MSGS_QUEUE_SIZE;
}

int modem_init() {
    spi_init(spi0, 6 * 1000 * 1000);
    gpio_set_function(LORA_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LORA_MISO, GPIO_FUNC_SPI);
    gpio_set_function(LORA_SCK, GPIO_FUNC_SPI);

    gpio_init(LORA_CS);
    gpio_set_dir(LORA_CS, GPIO_OUT);
    gpio_put(LORA_CS, 1);

    gpio_init(LORA_RESET);
    gpio_set_dir(LORA_RESET, GPIO_OUT);

    gpio_put(LORA_RESET, 0);
    sleep_ms(100);
    gpio_put(LORA_RESET, 1);
    sleep_ms(100);

    sx127x_create(NULL, &modem);
    ERROR_CHECK(sx127x_set_opmod(SX127X_MODE_STANDBY, SX127X_MODULATION_LORA, &modem));
    ERROR_CHECK(sx127x_rx_set_lna_boost_hf(true, &modem));
    ERROR_CHECK(sx127x_rx_set_lna_gain(SX127X_LNA_GAIN_G1, &modem));
    ERROR_CHECK(sx127x_tx_set_pa_config(SX127X_PA_PIN_BOOST, 20, &modem));
    ERROR_CHECK(sx127x_set_frequency(862000000, &modem));
    ERROR_CHECK(sx127x_lora_reset_fifo(&modem));
    ERROR_CHECK(sx127x_lora_set_bandwidth(SX127X_BW_250000, &modem));
    ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, &modem));
    ERROR_CHECK(sx127x_lora_set_spreading_factor(SX127X_SF_7, &modem));
    ERROR_CHECK(sx127x_lora_set_syncword(18, &modem));
    ERROR_CHECK(sx127x_set_preamble_length(8, &modem));

    sx127x_tx_set_callback(tx_callback, &modem, &modem);
    sx127x_rx_set_callback(rx_callback, &modem, &modem);

    sx127x_implicit_header_t header = {
      .enable_crc = true,
      .coding_rate = SX127X_CR_4_5,
      .length = MSG_SIZE
    };
    sx127x_lora_set_implicit_header(&header, &modem);

    gpio_init(LORA_DIO0);
    gpio_set_dir(LORA_DIO0, GPIO_IN);
    gpio_pull_up(LORA_DIO0);
    gpio_mine_register_callback(LORA_DIO0, GPIO_IRQ_EDGE_RISE, &lora_dio0_cb);

    return 0;
}

enum ModemEvent modem_get_last_event() {
    if (event_queue_r_ptr == event_queue_w_ptr) {
        return ModemEventNone;
    }
    enum ModemEvent modem_event = event_queue[event_queue_r_ptr];
    event_queue_r_ptr = (event_queue_r_ptr + 1) % MODEM_EVENT_QUEUE_SIZE;
    return modem_event;
}

int modem_get_last_msg(uint8_t *buf) {
    if (rx_msgs_r_ptr == rx_msgs_w_ptr) {
        return -1;
    }
    memcpy(buf, &rx_msgs_queue[rx_msgs_r_ptr], MSG_SIZE);
    rx_msgs_r_ptr = (rx_msgs_r_ptr + 1) % RX_MSGS_QUEUE_SIZE;
    return 0;
}

void modem_clear_events() {
    event_queue_r_ptr = 0;
    event_queue_w_ptr = 0;
    rx_msgs_r_ptr = 0;
    rx_msgs_w_ptr = 0;
}

int modem_send(uint8_t *buf) {
    int res = sx127x_lora_tx_set_for_transmission(buf, MSG_SIZE, &modem);
    sx127x_set_opmod(SX127X_MODE_TX, SX127X_MODULATION_LORA, &modem);
    return 0;
}

int modem_set_recv_mode() {
    ERROR_CHECK(sx127x_set_opmod(SX127X_MODE_RX_CONT, SX127X_MODULATION_LORA, &modem));
    return 0;
}

int modem_set_standby_mode() {
    ERROR_CHECK(sx127x_set_opmod(SX127X_MODE_STANDBY, SX127X_MODULATION_LORA, &modem));
    return 0;
}

int modem_set_frequency(uint64_t freq) {
    ERROR_CHECK(sx127x_set_opmod(SX127X_MODE_STANDBY, SX127X_MODULATION_LORA, &modem));
    ERROR_CHECK(sx127x_set_frequency(freq, &modem));
    return 0;
}
