//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include <hardware/structs/usb.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "../debug/debug.h"
#include "masterslave.h"
#include "malloc.h"
#include "string.h"
#include "hardware/spi.h"
#include "spi_data.h"

#include "pico/binary_info.h"

#define SPI_QUEUE_MAX_CAPACITY 20

enum gpio_pin {
    GPIO_MASTER_SELECT_PIN = 2,             // High for master, low for slave
    /**/
    /**
     * \set_by Slave
     * \description High when device is connected to slave
     */
    GPIO_SLAVE_DEVICE_ATTACHED_PIN = 15,
    /**
     * \set_by Master
     * \description Initiate syncing
     */
    GPIO_SYNCING = 14,
    /**
     * \set_by Slave
     * \description Slave ready to receive over SPI
     */
    GPIO_SLAVE_WAITING_PIN = 13,
    GPIO_SLAVE_RECEIVE_PIN = 12,            // Master -> Slave: Data to send
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
    /**/
    IS_PACKET = 0x0100,
    FIRST_PACKET = 0x0200,
    LAST_PACKET = 0x0400,
    /**/
    DEBUG_PRINT_AS_HEX = 0x8000
} msg_type;

typedef struct {
    uint16_t e_flag;
    uint16_t payload_length;
    uint8_t *payload;
} spi_message_t;

typedef void(*void_func_t)(void);

void messages_config(void);

spi_role get_role(void);

void set_spi_pin_handler(void_func_t fun);

//void send_message(const uint8_t *data, uint16_t len, uint16_t new_flag);

//uint16_t recieve_message(uint8_t *data);

void enqueue_spi_message(spi_message_t *message);

bool dequeue_spi_message(spi_message_t *message);

void sync(void);

#endif //PICO_FIRMWARE_MESSAGES_H
