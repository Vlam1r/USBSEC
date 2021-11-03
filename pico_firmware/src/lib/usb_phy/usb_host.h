//
// Created by vlamir on 11/2/21.
//

#ifndef PICO_FIRMWARE_USB_HOST_H
#define PICO_FIRMWARE_USB_HOST_H

#include "usb_phy.h"
#include <string.h>

// Hardware information per endpoint
typedef struct hw_endpoint
{
    // Is this a valid struct
    bool configured;

    // Transfer direction (i.e. IN is rx for host but tx for device)
    // allows us to common up transfer functions
    bool rx;

    uint8_t ep_addr;
    uint8_t next_pid;

    // Endpoint control register
    io_rw_32 *endpoint_control;

    // Buffer control register
    io_rw_32 *buffer_control;

    // Buffer pointer in usb dpram
    uint8_t *hw_data_buf;

    // Current transfer information
    bool active;
    uint16_t remaining_len;
    uint16_t xferred_len;

    // User buffer in main memory
    uint8_t *user_buf;

    // Data needed from EP descriptor
    uint16_t wMaxPacketSize;

    // Interrupt, bulk, etc
    uint8_t transfer_type;

#if TUSB_OPT_HOST_ENABLED
    // Only needed for host
    uint8_t dev_addr;

    // If interrupt endpoint
    uint8_t interrupt_num;
#endif
} hw_endpoint_t;

#endif //PICO_FIRMWARE_USB_HOST_H
