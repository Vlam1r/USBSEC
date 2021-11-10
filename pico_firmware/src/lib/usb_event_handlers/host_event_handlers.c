//
// Created by vlamir on 11/9/21.
//

#include "usb_event_handlers.h"

/*
 * Host events
 */

static uint8_t setup_packet[8];
static uint8_t attached = false;
static uint8_t bugger[1000];
static uint8_t level = 0;

void define_setup_packet(uint8_t *setup){
    memcpy(setup_packet, setup, 8);
}

void slavework() {
    gpio_put(PICO_DEFAULT_LED_PIN,1);
    int len = spi_receive_blocking(bugger);
    gpio_put(PICO_DEFAULT_LED_PIN,0);
    if(get_flag() & SETUP_DATA) {
        define_setup_packet(bugger);
        if(!attached) return;
        usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
        hcd_setup_send(0, 0, setup_packet);
    } else if (get_flag() & USB_DATA){
        hcd_edpt_xfer(0, 0, bugger[len-1] ^ 0x80, bugger, len-1);
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    level = 0;
    attached = true;
    hcd_setup_send(rhport, 0, setup_packet);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {
    // Intentionally empty
    // TODO maybe send IRQ to master?
}


void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    //uint8_t data[2] = {result, xferred_bytes};
    //spi_send_blocking(data, 2, DEBUG_PRINT_AS_HEX);
    if(level == 0) {
        level = 1;
        uint16_t len = ((tusb_control_request_t *) setup_packet)->wLength;
        hcd_edpt_xfer(0, 0, 0x80, bugger, len);
    }
    else if (level == 1) {
        level = 2;
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
        //hcd_edpt_xfer(0, 0, 0x00, bugger, 0);
        slavework();
    } else {
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
    }
}
