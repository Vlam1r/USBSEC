//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_MESSAGES_H
#define PICO_FIRMWARE_MESSAGES_H

#include <hardware/structs/usb.h>
#include "pico/stdlib.h"

enum gpio_pin {
    GPIO_MASTER_SELECT_PIN = 2,             // High for master, low for slave
    /**/
    GPIO_SLAVE_DEVICE_ATTACHED_PIN = 15,    // High when device is connected to slave
    GPIO_SLAVE_IRQ_PIN = 14,                // Slave -> Master: Data to send
    GPIO_SLAVE_WAITING_PIN = 13,            // High when slave is waiting to receive over spi
    GPIO_SLAVE_RECEIVE_PIN = 12,            // Master -> Slave: Data to send
    /**/
    GPIO_LED_PIN = 25                       // Onboard LED Control
};
static_assert(GPIO_LED_PIN == PICO_DEFAULT_LED_PIN, "");

typedef enum {
    SPI_ROLE_MASTER,
    SPI_ROLE_SLAVE
} spi_role;

typedef enum {
    USB_DATA = 0x1,
    SETUP_DATA = 0x2,
    RESET_USB = 0x4,
    EDPT_OPEN = 0x8,
    SLAVE_DATA = 0x10,
    CHG_ADDR = 0x20,
    /**/
    LAST_PACKET = 0x1000,
    /**/
    DEBUG_PRINT_AS_HEX = 0x8000
} msg_type;

typedef void(*void_func_t)(void);

void messages_config(void);

spi_role get_role(void);

void set_spi_pin_handler(void_func_t fun);

void send_message(const uint8_t *data, uint16_t len, uint16_t new_flag);

uint16_t recieve_message(uint8_t *data);

uint16_t get_flag(void);

#endif //PICO_FIRMWARE_MESSAGES_H
