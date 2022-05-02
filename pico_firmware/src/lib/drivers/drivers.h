// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 2/8/22.
//

#ifndef PICO_FIRMWARE_DRIVERS_H
#define PICO_FIRMWARE_DRIVERS_H

#include <pico.h>
#include "../messages/messages.h"
#include "../usb_event_handlers/usb_event_handlers.h"

typedef enum {
    DRIVER_ILLEGAL,
    DRIVER_MASS_STORAGE,
    DRIVER_HID
} driver_t;

void register_driver_for_edpt(const tusb_desc_endpoint_t *edpt, const tusb_desc_interface_t *itf, uint8_t new_mps);

void handle_spi_slave_event_with_driver(spi_message_t *message, uint8_t edpt);

void handle_device_xfer_complete_with_driver(uint8_t edpt, uint32_t xferred_bytes, uint8_t result);

#endif //PICO_FIRMWARE_DRIVERS_H
