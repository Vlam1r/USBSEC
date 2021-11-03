//
// Created by vlamir on 11/2/21.
//

#include "messages.h"

#define BUGGER_LENGTH 1024
uint8_t buffer[BUGGER_LENGTH];

void messages_config(void)
{
    gpio_init(GPIO_MASTER_SELECT_PIN);
    gpio_set_dir(GPIO_MASTER_SELECT_PIN, GPIO_IN);

    gpio_init(GPIO_SLAVE_IRQ_PIN);
    gpio_set_dir(GPIO_SLAVE_IRQ_PIN, is_master() ? GPIO_IN : GPIO_OUT);
    if(!is_master()) {
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    }

    if(is_master()) {
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
    bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI))

}

bool is_master(void)
{
    return gpio_get(GPIO_MASTER_SELECT_PIN);
}

void spi_send_blocking(const uint8_t *data, uint8_t len, uint8_t flag)
{
    assert(len < 255);
    if(!is_master()) {
        printf("Setting irq pin high\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    }
    spi_write_blocking(spi_default, &len, 1);
    spi_write_blocking(spi_default, data, len);
    if(!is_master()) {
        printf("Setting irq pin low\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    }
}

uint8_t spi_receive_blocking(uint8_t *data)
{
    uint8_t hdr = 0;
    if (is_master()) {
        printf("Waiting for irq pin\n");
        while(gpio_get(GPIO_SLAVE_IRQ_PIN) == 0)
            tight_loop_contents();
        printf("Irq pin high. Proceeding...\n");
    } else
        printf("Waiting for header\n");
    while(hdr == 0)
        spi_read_blocking(spi_default, 0, &hdr, 1);
    printf("Received length %d\n", hdr);
    assert(hdr < BUGGER_LENGTH);
    spi_read_blocking(spi_default, 0, data, hdr & 0xFF);
    printf("Received data:\n");
    for(int i=0;i<hdr;i++){
        printf("0x%x ", data[i]);
    }
    printf("\n");
    return hdr;
}