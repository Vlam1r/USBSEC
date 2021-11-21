//
// Created by vlamir on 11/6/21.
//

#ifndef PICO_FIRMWARE_USB_EVENT_HANDLERS_H
#define PICO_FIRMWARE_USB_EVENT_HANDLERS_H

#include <pico/stdlib.h>
#include <string.h>
#include "../messages/messages.h"
#include "tusb_common.h"
#include "tusb_types.h"
#include <hardware/structs/usb.h>
#include "../event_queue/event_queue.h"
#include "../edpt_registry/edpt_registry.h"
#include "../debug/debug.h"

typedef struct hw_endpoint {
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

    // Only needed for host
    uint8_t dev_addr;

    // If interrupt endpoint
    uint8_t interrupt_num;
} hw_endpoint_t;

typedef enum {
    DCD_EVENT_INVALID = 0,
    DCD_EVENT_BUS_RESET,
    DCD_EVENT_UNPLUGGED,
    DCD_EVENT_SOF,
    DCD_EVENT_SUSPEND, // TODO LPM Sleep L1 support
    DCD_EVENT_RESUME,

    DCD_EVENT_SETUP_RECEIVED,
    DCD_EVENT_XFER_COMPLETE,

    // Not an DCD event, just a convenient way to defer ISR function
    USBD_EVENT_FUNC_CALL,

    DCD_EVENT_COUNT
} dcd_eventid_t;

void define_setup_packet(uint8_t *setup);

// Helper to send device attach event
void hcd_event_device_attach(uint8_t rhport, bool in_isr);

// Helper to send device removal event
void hcd_event_device_remove(uint8_t rhport, bool in_isr);

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr);

void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr);

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr);

extern bool hcd_setup_send(uint8_t rhport, uint8_t dev_addr, uint8_t const setup_packet[8]);

void device_event_bus_reset(void);

extern void dcd_event_bus_signal(uint8_t rhport, dcd_eventid_t eid, bool in_isr);

bool dcd_edpt_open_new(uint8_t rhport, tusb_desc_endpoint_t const *desc_edpt);

extern hw_endpoint_t *get_dev_ep(uint8_t dev_addr, uint8_t ep_addr);

bool hcd_init(uint8_t rhport);

void hcd_int_enable(uint8_t rhport);

bool hcd_edpt_open(tusb_desc_endpoint_t const *ep_desc);

// Call to send data
void hw_endpoint_xfer_start(struct hw_endpoint *ep, uint8_t *buffer /* user_buf*/, uint16_t total_len);

bool hcd_edpt_xfer(uint8_t rhport, uint8_t dev_addr, uint8_t ep_addr, uint8_t *buffer, uint16_t buflen);

bool dcd_edpt_xfer_new(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes);

void dcd_init_new(uint8_t rhport);

void dcd_int_enable_new(uint8_t rhport);

void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const *request);

void slavework(void);

void masterwork(void);

void spi_handler_init(void);


#endif //PICO_FIRMWARE_USB_EVENT_HANDLERS_H
