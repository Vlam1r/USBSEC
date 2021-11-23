//
// Created by vlamir on 11/9/21.
//

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

bool print = false;
int curredpt = -1;

void slavework() {
    gpio_put(GPIO_LED_PIN, 1);
    gpio_put(GPIO_SLAVE_WAITING_PIN, 1);
    int len = recieve_message(bugger);
    gpio_put(GPIO_SLAVE_WAITING_PIN, 0);

    if (get_flag() & RESET_USB) {
        /*
         * Reset USB device
         */
        gpio_put(GPIO_LED_PIN, 0);
        dev_addr = 0; // TODO is this needed?
        usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
    } else if (get_flag() & EDPT_OPEN) {
        /*
         * Open sent endpoint
         */
        memcpy(&registry[reg_count++], bugger, len);
        hcd_edpt_open(bugger);
    } else if (get_flag() & SETUP_DATA) {
        /*
         * Data is copied into setup
         */
        define_setup_packet(bugger);
        level = 0;
        hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
    } else if (get_flag() & USB_DATA) {
        /*
         * Data is copied into buffer
         */
        level = 3;
        //gpio_put(PICO_DEFAULT_LED_PIN, 1);
        hcd_edpt_xfer(0, dev_addr, bugger[len - 1], bugger, len - 1);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    } else if (get_flag() & SLAVE_DATA) {
        /*
         * Empty data (event) queue to master
         */
        uint8_t queue_len = queue_size();
        send_message(&queue_len, 1, SLAVE_DATA);
        while (queue_size() > 0) {
            event_t *e = get_from_event_queue();
            e->payload[e->payload_length] = e->ep_addr;
            send_message(e->payload, e->payload_length + 1, SLAVE_DATA);
            delete_event(e);
        }
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    } else if (get_flag() & CHG_ADDR) {
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

}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    set_spi_pin_handler(slavework);
    //dev_addr = 0;
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 1);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {
    set_spi_pin_handler(NULL);
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 0);
}

void hcd_event_xfer_complete(uint8_t dev_addr_curr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    assert(result == 0);
    if (level == 0) {
        // Setup packet response. Send IN or OUT token packet
        level = 1;
        if (setup_packet.bmRequestType_bit.direction == 0) {
            // Data transport not supported
            //gpio_put(PICO_DEFAULT_LED_PIN, 1);
            //hcd_edpt_xfer(0, dev_addr_curr, 0x00, NULL, 0);
            hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, 64);
        } else {
            hcd_edpt_xfer(0, dev_addr_curr, 0x80, bugger, setup_packet.wLength);
        }
    } else if (level == 1) {
        // Data packet response.
        level = 2;
        outlen = xferred_bytes;
        if (setup_packet.bmRequestType_bit.direction == 0) {
            event_t e = {
                    .e_type = HOST_EVENT_XFER_COMPLETE,
                    .payload = NULL,
                    .payload_length = 0,
                    .ep_addr = ep_addr
            };
            create_event(&e);
            gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
            //hcd_edpt_xfer(0, dev_addr_curr, 0x80, NULL, 0); // Request ACK
        } else {
            hcd_edpt_xfer(0, dev_addr_curr, 0x00, NULL, 0); // Request ACK
        }
    } else if (level == 2) {
        // Ack sent
        event_t e = {
                .e_type = HOST_EVENT_XFER_COMPLETE,
                .payload = bugger,
                .payload_length = outlen,
                .ep_addr = ep_addr
        };
        create_event(&e);
        gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    } else {
        /*
         * Non-control transfer
         */
        event_t e = {
                .e_type = HOST_EVENT_XFER_COMPLETE,
                .payload = bugger,
                .payload_length = xferred_bytes,
                .ep_addr = ep_addr
        };
        create_event(&e);
        gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    }
}
