// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/6/21.
//

#include <malloc.h>
#include "usb_event_handlers.h"
#include "../debug/debug.h"
#include "../setup_response_aggregator/setup_response_aggregator.h"
#include "../drivers/drivers.h"
#include "../validator/validator.h"

static uint8_t bMaxPacketSize = 64;
static tusb_control_request_t setup;

static enum {
    READING_RESPONSE,
    WRITING_RESPONSE
} setup_response_dir = READING_RESPONSE;

/*
 * Device Events
 */

static void handle_setup_response();

static void send_empty_message() {
    static const spi_message_t dummy = {
            .payload = NULL,
            .payload_length = 0,
            .e_flag = 0
    };
    //enqueue_spi_message(&dummy);
}

void handle_spi_slave_event(void) {
    if (setup_response_dir == WRITING_RESPONSE) {
        /*
         * We handled a response, and now we are forwarding it to host.
         */
        bool done = send_packet_upstream();
        if (done) {
            setup_response_dir = READING_RESPONSE;
        }
        return;
    }

    spi_message_t msg;
    if (dequeue_spi_message(&msg)) {

        while (msg.e_flag == DEBUG_PRINT_AS_STRING) {
            printf("Slave says: ");
            printf((char *) msg.payload);
            printf("\n");
            free(msg.payload);
            if (!dequeue_spi_message(&msg)) {
                return;
            }

        }

        uint8_t ep_addr = msg.payload[--msg.payload_length];
        debug_print(PRINT_REASON_SLAVE_DATA,
                    "[SLAVE DATA] Packet for 0x%x of length %d with flag 0x%x\n",
                    ep_addr, msg.payload_length, msg.e_flag);
        runtime_assert(msg.e_flag & IS_PACKET);
        if (msg.e_flag & SETUP_DATA) {
            register_response(&msg);
            if (msg.e_flag & LAST_PACKET) {
                handle_setup_response();
                setup_response_dir = WRITING_RESPONSE;
                if (setup.bmRequestType_bit.direction == 1) {
                    dcd_edpt_xfer_new(0, 0x00, NULL, 0); //STATUS?
                }
            }

        } else {
            handle_spi_slave_event_with_driver(&msg, ep_addr);
        }
        free(msg.payload);
    }
    //debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Done.\n");
}

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *p_setup, bool in_isr) {
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    memcpy(&setup, p_setup, sizeof setup);
    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Received new setup.\n");
    debug_print_array(PRINT_REASON_SETUP_REACTION, (const uint8_t *) &setup, 8);

    if (setup.bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we have to also change the address on slave.
         * Some devices will break if they don't get their address changed!
         */
        spi_message_t msg = {
                .payload_length = 0,
                .payload = NULL,
                .e_flag = CHG_ADDR
        };
        enqueue_spi_message(&msg);
        return;
    }

    if (setup.bRequest == 0x6 && setup.wValue == 0x0600 /* DEVICE QUALIFIER */) {
        /*
         * Currently ignoring this request as it causes stalls downstream.
         */
        dcd_edpt_xfer_new(0, 0x80, 0, 0);
        return;
    }

    if (setup.bmRequestType == 0x21 && setup.bRequest == 0x9) {
        printf("!!!!!!!!!!!!REPORT!!!!!!!!!!!!!!\n");
        dcd_edpt_xfer_new(0, 0x00, 0, 0);
        return;
    }

    /*
     * Forward setup packet to slave and get response from device
     */
    spi_message_t msg = {
            .payload_length = sizeof(setup),
            .payload = (uint8_t *) &setup,
            .e_flag = SETUP_DATA | DEBUG_PRINT_AS_HEX
    };
    enqueue_spi_message(&msg);
}

static void handle_setup_response() {
    uint8_t *arr = get_concatenated_response();
    uint16_t len = get_concatenated_response_len();

    debug_print(PRINT_REASON_SETUP_REACTION, "Received setup response.\n");
    debug_print_array(PRINT_REASON_SETUP_REACTION, arr, len);
    /*
     * Set correct maxPacketSize on slave
     */
    if (setup.bRequest == 0x6 && setup.wValue == 0x100 /*DEVICE*/) {
        spi_message_t reply = {
                .payload_length = 1,
                .payload = &arr[7],
                .e_flag = CHG_EPX_PACKETSIZE | DEBUG_PRINT_AS_HEX
        };
        enqueue_spi_message(&reply);
        bMaxPacketSize = arr[7];
        set_max_packet_size(arr[7]);
    }

    /*
     * Setup address special case
     */
    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        return;
    }

    /*
     * Handle endpoints and open them on slave
     */
    if (setup.wValue == 0x0200 && setup.wLength > 9) {
        debug_print(PRINT_REASON_SETUP_REACTION, "Started handling endpoints.\n");
        int pos = 0;
        uint8_t itf = 0;
        while (pos < len) {
            debug_print(PRINT_REASON_SETUP_REACTION, "+--\n|Len %d\n|Type 0x%x\n+--", arr[pos], arr[pos + 1]);
            if (arr[pos + 1] == 0x04) {
                /*
                 * INTERFACE DESCRIPTOR
                 */
                itf = arr[pos + 5]; // todo maybe parse properly
            }

            if (arr[pos + 1] == 0x05) {
                /*
                 * ENDPOINT DESCRIPTOR
                 */
                const tusb_desc_endpoint_t *const edpt = (const tusb_desc_endpoint_t *const) &arr[pos];
                dcd_edpt_open_new(0, edpt);
                // Start read
                debug_print(PRINT_REASON_SETUP_REACTION, "New endpoint registered: 0x%x\n", edpt->bEndpointAddress);
                register_driver_for_edpt(edpt->bEndpointAddress, itf, bMaxPacketSize);

                spi_message_t reply = {
                        .payload = (uint8_t *) edpt,
                        .payload_length = edpt->bLength,
                        .e_flag = EDPT_OPEN | DEBUG_PRINT_AS_HEX
                };
                //send_empty_message();
                enqueue_spi_message(&reply);
            }
            pos += arr[pos];
        }
    }

    if (setup.bRequest == 0x09 /* SET CONFIG */) {
        debug_print(PRINT_REASON_SETUP_REACTION, "Configuration confirmed.\n");
        /*
         * Validation
         */
        validate_driver_config();
    }

    /*
     * Setting up interrupt finished. Need to request a read
     */
    if (setup.wValue == 0x2200 /* REQUEST HID REPORT*/) {
        send_empty_message();
        knock_on_slave_edpt(setup.wIndex + 0x81, 9); // todo
    }

    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Setup handling ended.\n");
}

void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr) {

    if (setup.bRequest == 0x5 /*SET ADDRESS*/ && ep_addr == 0x80) {
        debug_print(PRINT_REASON_SETUP_REACTION,
                    "Setting address to %d, [%d] %d\n",
                    ((const tusb_control_request_t *) usb_dpram->setup_packet)->wValue,
                    ep_addr,
                    xferred_bytes);
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
        return;
    }

    if (ep_addr == 0 || ep_addr == 0x80) return;
    debug_print(PRINT_REASON_XFER_COMPLETE,
                "[XFER COMPLETE] Completed transfer on 0x%x with %d bytes\n",
                ep_addr, xferred_bytes);

    handle_device_xfer_complete_with_driver(ep_addr, xferred_bytes, result);
}

void device_event_bus_reset() {
    debug_print(PRINT_REASON_IRQ, "[IRQ] --- RESETTING BUS ---\n");
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    spi_message_t msg = {
            .payload = NULL,
            .payload_length = 0,
            .e_flag = RESET_USB
    };
    enqueue_spi_message(&msg);
}
