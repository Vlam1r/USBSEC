//
// Created by vlamir on 11/9/21.
//

#include "usb_event_handlers.h"

/*
 * Host events
 */

static tusb_control_request_t setup_packet;
static uint8_t attached = false;
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

void slavework() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    int len = spi_receive_blocking(bugger);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    if (get_flag() & RESET_USB) {
        usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
    }

    if (get_flag() & EDPT_OPEN) {
        /*
         * Open sent endpoint
         */
        memcpy(&registry[reg_count++], bugger, len);
        spi_send_blocking(NULL, 0, USB_DATA);
        slavework();
    } else if (get_flag() & SETUP_DATA) {
        /*
         * Data is copied into setup
         */
        define_setup_packet(bugger);
        if (!attached) return;
        level = 0;

        if (!(get_flag() & RESET_USB)) {
            hcd_setup_send(0, dev_addr, (const uint8_t *) &setup_packet);
        }
    } else if (get_flag() & USB_DATA) {
        /*
         * Data is copied into buffer
         */
        level = 3;
        uint8_t reg_idx = bugger[len - 1];
        hcd_edpt_open(&registry[reg_idx]);
        hcd_edpt_xfer(0, dev_addr, registry[reg_idx].bEndpointAddress, bugger, len - 1);
    } else if (get_flag() & EVENTS) {
        /*
         * Handle event queue
         */
        uint8_t event_count = queue_size();
        spi_send_blocking(&event_count, 1, USB_DATA | DEBUG_PRINT_AS_HEX);
        while (event_count--) {
            event_t *e = get_from_event_queue();
            spi_send_blocking((const uint8_t *) e, sizeof(event_t), USB_DATA);
        }
    } else {
        slavework();
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    if (attached) return;
    level = 0;
    attached = true;
    set_spi_pin_handler(slavework);
    hcd_setup_send(rhport, dev_addr, (const uint8_t *) &setup_packet);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {
    attached = false;
    // TODO maybe send IRQ to master?
}

void hcd_event_xfer_complete(uint8_t dev_addr_old, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    uint8_t data[5] = {0xff, xferred_bytes, setup_packet.wLength, ep_addr, level};
    spi_send_blocking(data, 5, DEBUG_PRINT_AS_HEX);

    if (level == 0) {
        // SETUP
        level = 1;
        hcd_edpt_xfer(0, dev_addr_old, 0x80, bugger, setup_packet.wLength); // Request response
    } else if (level == 1) {
        // DATA
        level = 2;
        if (xferred_bytes == 0) {
            /*
             * If no data is to be transferred we shouldn't request ACK
             */
            spi_send_blocking(NULL, 0, USB_DATA | DEBUG_PRINT_AS_HEX);
            slavework();
            return;
        }
        outlen = xferred_bytes;
        hcd_edpt_xfer(0, dev_addr_old, 0x00, NULL, 0); // Request ACK
    } else if (level == 2) {
        // ACK
        spi_send_blocking(bugger, outlen, USB_DATA | DEBUG_PRINT_AS_HEX);
        slavework();
    } else if (level == 3) {
        //level++;
        //hcd_edpt_open(&registry[0]); // TODO HARDEN
        //hcd_edpt_xfer(0, dev_addr, 0x81, bugger, 0xff);
        //spi_send_string("DONE");
        //bugger[xferred_bytes] = ep_addr;
        //spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
        if (!(ep_addr & 0x80)) return;
        event_t e;
        e.length = xferred_bytes + 1;
        assert(xferred_bytes < 256);
        memcpy(e.payload, bugger, xferred_bytes);
        e.payload[xferred_bytes] = ep_addr;
        insert_into_event_queue(&e);
        //slavework();

    } else {
        // USB
        spi_send_string("DONE");
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
        slavework();
    }
}
