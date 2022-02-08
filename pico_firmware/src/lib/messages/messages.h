// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include "pico.h"
#include "masterslave.h"

#define SPI_QUEUE_MAX_CAPACITY 20

enum gpio_pin {
    /**
     * \set_by Slave
     * \description High when device is connected to slave
     */
    GPIO_SLAVE_DEVICE_ATTACHED_PIN = 15,
    /**
     * \set_by Master
     * \description Initiate syncing
     */
    GPIO_SYNC = 14,
    /**
     * \set_by Slave
     * \description Slave ready to receive over SPI
     */
    GPIO_SLAVE_WAITING_PIN = 13,
    /**/
    GPIO_LED_PIN = 25                       // Onboard LED Control
};
static_assert(GPIO_LED_PIN == PICO_DEFAULT_LED_PIN, "");

typedef enum {
    USB_DATA = 0x1,
    SETUP_DATA = 0x2,
    RESET_USB = 0x4,
    EDPT_OPEN = 0x8,
    SLAVE_DATA_QUERY = 0x10,
    CHG_ADDR = 0x20,
    CHG_EPX_PACKETSIZE = 0x40,
    /**/
    IS_PACKET = 0x0100,
    FIRST_PACKET = 0x0200,
    LAST_PACKET = 0x0400,
    /**/
    DEBUG_PRINT_AS_STRING = 0x4000,
    DEBUG_PRINT_AS_HEX = 0x8000
} msg_type;

typedef struct {
    uint16_t e_flag;
    uint16_t payload_length;
    uint8_t *payload;
} spi_message_t;


void messages_config(void);

void enqueue_spi_message(spi_message_t *message);

bool dequeue_spi_message(spi_message_t *message);

void send_string_message(const char *string);

void knock_on_slave_edpt(uint8_t edpt, uint8_t len);

void sync(void);

void fake_spi(void);

#endif //PICO_FIRMWARE_MESSAGES_H
