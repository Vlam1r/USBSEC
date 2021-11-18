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

static tusb_desc_endpoint_t registry[16];
static uint8_t reg_count = 0;

void define_setup_packet(uint8_t *setup) {
    memcpy(&setup_packet, setup, 8);
}

bool print = false;
int curredpt = -1;

void slavework() {

    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(GPIO_SLAVE_WAITING_PIN, 1);
    int len = spi_receive_blocking(bugger);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    gpio_put(GPIO_SLAVE_WAITING_PIN, 0);

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
        level = 0;

        hcd_setup_send(0, 0, (const uint8_t *) &setup_packet);
    } else if (get_flag() & USB_DATA) {
        /*
         * Data is copied into buffer
         */
        level = 3;

        uint8_t reg_idx = bugger[len - 1];
        if (curredpt != reg_idx) hcd_edpt_open(&registry[reg_idx]);
        curredpt = reg_idx;
        hcd_edpt_xfer(0, 0, registry[reg_idx].bEndpointAddress, bugger, len - 1);
    } else {
        slavework();
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 1);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {
    gpio_put(GPIO_SLAVE_DEVICE_ATTACHED_PIN, 0);
}

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    uint8_t data[5] = {0xee, xferred_bytes, setup_packet.wLength, ep_addr, level};
    spi_send_blocking(data, 5, DEBUG_PRINT_AS_HEX);

    if (level == 0) {
        // SETUP
        level = 1;
        hcd_edpt_xfer(0, 0, 0x80, bugger, setup_packet.wLength); // Request response
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
        hcd_edpt_xfer(0, 0, 0x00, NULL, 0); // Request ACK
    } else if (level == 2) {
        // ACK
        spi_send_blocking(bugger, outlen, USB_DATA | DEBUG_PRINT_AS_HEX);
        slavework();
    } else {
        gpio_put(SLAVE_SPI_WAITING_TO_SEND, 1);
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
        gpio_put(SLAVE_SPI_WAITING_TO_SEND, 1);
        slavework();

    }
}
