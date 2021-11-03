//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include <stdio.h>

#define GPIO_MASTER_SELECT_PIN 0
#define GPIO_SLAVE_IRQ_PIN 14
#define SPI_BAUDRATE 8*1000*1000 // 9MHz is too much at 144MHz clock

void messages_config(void);
void spi_send_blocking(const uint8_t *data, uint8_t len, uint8_t flag);
uint8_t spi_receive_blocking(uint8_t *data);
bool is_master(void);

#endif //PICO_FIRMWARE_MESSAGES_H
