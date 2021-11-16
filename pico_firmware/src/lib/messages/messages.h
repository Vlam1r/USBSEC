//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>

#define GPIO_MASTER_SELECT_PIN 2
#define GPIO_SLAVE_IRQ_PIN 14
#define SPI_BAUDRATE (int)(4*1000*1000) // 8MHz is too much at 144MHz clock

typedef enum {
    USB_DATA = 0x1,
    SETUP_DATA = 0x2,
    RESET_USB = 0x4,
    EDPT_OPEN = 0x8,
    EVENTS = 0x10,
    DEBUG_PRINT_AS_STRING = 0x40,
    DEBUG_PRINT_AS_HEX = 0x80
} msg_type;

typedef enum {
    SPI_ROLE_MASTER,
    SPI_ROLE_SLAVE
} spi_role;

typedef void(*void_func_t)(void);

void print_arr_hex(const uint8_t *data, int len);

void messages_config(void);

void spi_send_blocking(const uint8_t *data, uint16_t len, uint16_t flag);

uint16_t spi_receive_blocking(uint8_t *data);

void spi_send_string(char *data);

spi_role get_role(void);

uint16_t get_flag(void);

int spi_await(uint8_t *data, uint16_t cond);

void set_spi_pin_handler(void_func_t fun);

void trigger_spi_irq(void);

#endif //PICO_FIRMWARE_MESSAGES_H
