// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/9/21.
//

#include <malloc.h>
#include "usb_event_handlers.h"

/*
 * Host events
 */

static tusb_control_request_t setup_packet;
static uint8_t bugger[1000];
static uint8_t level = 0;
static uint8_t outlen = 0;
static uint8_t dev_addr = 0;

static tusb_desc_endpoint_t registry[16];
static uint8_t reg_count = 0;

void define_setup_packet(uint8_t *setup) {
    memcpy(&setup_packet, setup, 8);
}

void slavework() {

    spi_message_t message;
    if (dequeue_spi_message(&message)) {

        if (message.e_flag & RESET_USB) {
            /*
             * Reset USB device
             */
            dev_addr = 0; // TODO is this needed?
            usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
        } else if (message.e_flag & EDPT_OPEN) {
            /*
             * Open sent endpoint
             */
            memcpy(&registry[reg_count++], message.payload, message.payload_length);
            hcd_edpt_open(message.payload);
        } else if (message.e_flag & SETUP_DATA) {
            /*
             * Data is copied into setup
             */
            define_setup_packet(message.payload);
            level = 0;
            hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
        } else if (message.e_flag & USB_DATA) {
            /*
             * Data is copied into buffer
             */
            level = 3;
            //gpio_put(PICO_DEFAULT_LED_PIN, 1);
            memcpy(bugger, message.payload, message.payload_length - 1);
            hcd_edpt_xfer(0,
                          dev_addr,
                          message.payload[message.payload_length - 1],
                          bugger,
                          message.payload_length - 1);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        } else if (message.e_flag & CHG_ADDR) {
            tusb_control_request_t req = {
                    .bmRequestType = 0,
                    .wValue = 7,
                    .bRequest = 0x5 // SET ADDRESS
            };
            define_setup_packet(&req);
            level = 0;
            //gpio_put(PICO_DEFAULT_LED_PIN, 1);
            hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
            dev_addr = 7;
        }
        free(message.payload);
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 1);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 0);
}

static void send_event_to_master(uint16_t len, uint8_t ep_addr, uint16_t flag) {
    if (len == 0) {
        spi_message_t msg = {
                .payload_length = 1,
                .payload = &ep_addr,
                .e_flag = IS_PACKET | flag
        };
        enqueue_spi_message(&msg);
    } else {
        bugger[len] = ep_addr;
        spi_message_t msg = {
                .payload_length = len + 1,
                .payload = bugger,
                .e_flag = IS_PACKET | flag
        };
        enqueue_spi_message(&msg);
    }
}

void hcd_event_xfer_complete(uint8_t dev_addr_curr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    assert(result == 0 || result == 4);
    if (level == 0) {
        // Setup packet response. Send IN or OUT token packet
        level = 1;
        if (setup_packet.bmRequestType_bit.direction == 0) {
            // Data transport not supported
            hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, 64);
        } else {
            hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, setup_packet.wLength);
        }
    } else if (level == 1) {
        // Data packet response.
        level = 2;
        outlen = xferred_bytes;
        if (setup_packet.bmRequestType_bit.direction == 0) {

            send_event_to_master(0, ep_addr, FIRST_PACKET | LAST_PACKET | SETUP_DATA);
        } else {
            hcd_edpt_xfer(0, dev_addr_curr, 0x00, NULL, 0); // Request ACK
        }
    } else if (level == 2) {
        // Ack sent
        send_event_to_master(outlen, ep_addr, FIRST_PACKET | LAST_PACKET | SETUP_DATA);
    } else {
        /*
         * Non-control transfer
         */
        uint16_t new_flag = 0;
        if (result == 0) new_flag |= LAST_PACKET;
        if (result == 4) new_flag |= FIRST_PACKET;
        send_event_to_master(xferred_bytes, ep_addr, new_flag);
        //Problematic as xferred bytes is unbounded here.
    }
}