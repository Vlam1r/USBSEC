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
#include <hardware/structs/usb.h>
#include "../debug/debug.h"

#define GPIO_MASTER_SELECT_PIN 2
#define GPIO_SLAVE_DEVICE_ATTACHED_PIN 15
#define GPIO_SLAVE_IRQ_PIN 14
#define GPIO_SLAVE_WAITING_PIN 13
#define GPIO_SLAVE_RECEIVE_PIN 12
#define SPI_BAUDRATE (int)(4*1000*1000) // 8MHz is too much at 144MHz clock

typedef enum {
    USB_DATA = 0x1,
    SETUP_DATA = 0x2,
    RESET_USB = 0x4,
    EDPT_OPEN = 0x8,
    /**/
    DEBUG_PRINT_AS_STRING = 0x4000,
    DEBUG_PRINT_AS_HEX = 0x8000
} msg_type;

typedef enum {
    SPI_ROLE_MASTER,
    SPI_ROLE_SLAVE
} spi_role;

typedef void(*void_func_t)(void);

void messages_config(void);

void spi_send_blocking(const uint8_t *data, uint16_t len, uint16_t new_flag);

uint16_t spi_receive_blocking(uint8_t *data);

void spi_send_string(char *data);

spi_role get_role(void);

void spi_send_async(const uint8_t *data, uint8_t len, uint8_t flag);

uint16_t get_flag();

int spi_await(uint8_t *data, uint16_t cond);

void set_spi_pin_handler(void_func_t fun);

void trigger_spi_irq(void);

#endif //PICO_FIRMWARE_MESSAGES_H
