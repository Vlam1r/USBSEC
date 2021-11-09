#include <sys/cdefs.h>
//
// Created by vlamir on 11/2/21.
//

#include "messages.h"

static const uint8_t *bugger;

static uint8_t flxg;

uint8_t get_flag() {
    return flxg;
}

static void print_arr_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    if (len % 8 != 0) {
        printf("\n");
    }
}

_Noreturn static void core1_main() {
    while(true) {
        uint32_t hdr = multicore_fifo_pop_blocking();
        spi_send_blocking(bugger, hdr >> 8, hdr & 0xFF);
    }
}

void messages_config(void) {
    multicore_launch_core1(core1_main);

    gpio_init(GPIO_MASTER_SELECT_PIN);
    gpio_set_dir(GPIO_MASTER_SELECT_PIN, GPIO_IN);

    gpio_init(GPIO_SLAVE_IRQ_PIN);
    gpio_set_dir(GPIO_SLAVE_IRQ_PIN, is_master() ? GPIO_IN : GPIO_OUT);
    if (!is_master()) {
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    }

    if (is_master()) {
        printf("--------\n MASTER \n--------\n");
    } else {
        printf("-------\n SLAVE \n-------\n");
    }

    spi_init(spi_default, SPI_BAUDRATE);
    spi_set_slave(spi_default, !is_master());
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

    // picotool binary information
    bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN,
                               PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI))

}

bool is_master(void) {
    return gpio_get(GPIO_MASTER_SELECT_PIN);
}

void spi_send_blocking(const uint8_t *data, uint8_t len, uint8_t flag) {
    assert(len < 255);
    if (!is_master()) {
        printf("Setting irq pin high\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    }
    uint8_t hdr[2] = {len, flag};
    spi_write_blocking(spi_default, hdr, 2);
    spi_write_blocking(spi_default, data, len);
    if(flag & DEBUG_PRINT_AS_HEX) {
        printf("Sent data:\n");
        print_arr_hex(data, len);
    }
    if(flag & DEBUG_PRINT_AS_STRING) {
        printf((char *) data);
        printf("\n");
    }
    if (!is_master()) {
        printf("Setting irq pin low\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    }
}

uint8_t spi_receive_blocking(uint8_t *data) {
    uint8_t len = 0;
    uint8_t flag = 0;
    if (is_master()) {
        //printf("Waiting for irq pin\n");
        while (gpio_get(GPIO_SLAVE_IRQ_PIN) == 0)
            tight_loop_contents();
        //printf("Irq pin high. Proceeding...\n");
    } else
        printf("Waiting for header\n");
    while (len == 0)
        spi_read_blocking(spi_default, 0, &len, 1);
    spi_read_blocking(spi_default, 0, &flag, 1);
    if(is_master()) sleep_us(1); // I <3 when a 1 microsecond sleep makes an algorithm work
    spi_read_blocking(spi_default, 0, data, len & 0xFF);
    if(flag & DEBUG_PRINT_AS_HEX) {
        printf("Received length %d\n", len);
        printf("Received data:\n");
        print_arr_hex(data, len);
    }
    if(flag & DEBUG_PRINT_AS_STRING) {
        printf((char *) data);
        printf("\n");
    }
    flxg = flag;
    return len;
}

void spi_send_string(char *data){
    spi_send_blocking((uint8_t *) data, strlen(data), DEBUG_PRINT_AS_STRING);
}

void spi_send_async(const uint8_t *data, uint8_t len, uint8_t flag) {
    bugger = data;
    multicore_fifo_push_blocking((len << 8) | flag);
}

uint8_t data[1000];
void slavework() {
    int len = spi_receive_blocking(data);
    if(get_flag() & SETUP_DATA) {

    }
}