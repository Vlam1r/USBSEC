// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 2/8/22.
//

#include "drivers.h"

#include "../debug/debug.h"
#include "../usb_event_handlers/usb_event_handlers.h"

typedef enum {
    DRIVER_ILLEGAL,
    DRIVER_MASS_STORAGE,
    DRIVER_HID
} driver_t;

static driver_t edpt_driver[32];

static bool permit_0x81 = false;
static int count0x81 = 0;
static uint8_t other_edpt;
static uint8_t bMaxPacketSize = 0;

static uint8_t bugger[1024];


/*
 * MSC DRIVER
 */

void msc_spi(spi_message_t *message, uint8_t edpt) {
    if (~edpt & 0x80) {
        /*
         * OUT endpoint
         */
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", other_edpt);
        knock_on_slave_edpt(other_edpt, bMaxPacketSize); // TODO other_edpt
        dcd_edpt_xfer_new(0, edpt, bugger, bMaxPacketSize);
        debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Listening to port 0x%x\n", edpt);
    } else {
        /*
         * IN endpoint
         */
        permit_0x81 = true;
        count0x81++;
        debug_print(PRINT_REASON_USB_EXCHANGES, "+++++ STARTING PARTIAL XFER ON %d++++\n", edpt);
        dcd_edpt_xfer_partial(edpt, message->payload, message->payload_length, message->e_flag);
    }
}

void msc_xfer(uint8_t edpt, uint32_t xferred_bytes, uint8_t result) {
    uint8_t arr[100];
    if (~edpt & 0x80) {
        permit_0x81 = false;
        count0x81 = 0;
        bugger[xferred_bytes] = edpt; //TODO
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", edpt);
        spi_message_t msg = {
                .payload_length = xferred_bytes + 1,
                .payload = bugger,
                .e_flag = USB_DATA | DEBUG_PRINT_AS_HEX
        };
        enqueue_spi_message(&msg);
    } else if (permit_0x81) {
        if (xferred_bytes == 13 && count0x81 > 0) {
            debug_print(PRINT_REASON_XFER_COMPLETE,
                        "+++[XFER COMPLETE] EMERGENCY BAILOUT BECAUSE SCSI COMPLETED EARLY+++\n");
            return;
        }
        memset(arr, 0, 64);
        arr[64] = edpt;
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", edpt);
        spi_message_t msg = {
                .payload_length = 64 + 1,
                .payload = arr,
                .e_flag = USB_DATA
        };
        enqueue_spi_message(&msg);
    }
    debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Leaving.\n");
}

/*
 * HID DRIVER
 */


void hid_spi(spi_message_t *message, uint8_t edpt) {
    dcd_edpt_xfer_new(0, edpt, message->payload, message->payload_length);
}

void hid_xfer(uint8_t edpt, uint32_t xferred_bytes, uint8_t result) {
    knock_on_slave_edpt(edpt, bMaxPacketSize);
}

/*
 * API
 */

static uint8_t idx(uint8_t edpt) {
    if (edpt & 0x80) {
        edpt &= ~0x80;
        edpt |= 0x10;
    }
    return edpt;
}

void register_driver_for_edpt(uint8_t edpt, uint8_t itf, uint8_t new_mps) {

    bMaxPacketSize = new_mps;


    switch (itf) {
        case 0x03:
            edpt_driver[idx(edpt)] = DRIVER_HID;
            break;
        case 0x08:
            edpt_driver[idx(edpt)] = DRIVER_MASS_STORAGE;
            if (edpt & 0x80)
                dcd_edpt_xfer_new(0, edpt, bugger, 64); // Query OUT edpt
            else {
                other_edpt = edpt;
            }
            break;
        default:
            error("Invalid interface number 0x%x", itf);
    }
}

void handle_spi_slave_event_with_driver(spi_message_t *message, uint8_t edpt) {
    switch (edpt_driver[idx(edpt)]) {
        case DRIVER_MASS_STORAGE:
            msc_spi(message, edpt);
            break;
        case DRIVER_HID:
            hid_spi(message, edpt);
            break;
        default:
            error("Unregistered edpt 0x%x", edpt);
    }
}

void handle_device_xfer_complete_with_driver(uint8_t edpt, uint32_t xferred_bytes, uint8_t result) {
    switch (edpt_driver[idx(edpt)]) {
        case DRIVER_MASS_STORAGE:
            msc_xfer(edpt, xferred_bytes, result);
            break;
        case DRIVER_HID:
            hid_xfer(edpt, xferred_bytes, result);
            break;
        default:
            error("Unregistered edpt 0x%x", edpt);
    }
}
