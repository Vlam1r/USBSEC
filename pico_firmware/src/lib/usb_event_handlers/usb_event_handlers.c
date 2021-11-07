//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

uint8_t setup_packet[8];
uint8_t bugger[1000];

int level = 0;

void define_setup_packet(uint8_t *setup){
    memcpy(setup_packet, setup, 8);
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    level = 0;
    hcd_setup_send(rhport, 0, setup_packet);
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {

}

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    hw_endpoint_t *ep = get_dev_ep(dev_addr, ep_addr);
    spi_send_string("Transfer complete. Result and transferred bytes:");
    uint8_t data[2] = {result, xferred_bytes};
    spi_send_blocking(data, 2, DEBUG_PRINT_AS_HEX);
    if(level == 0){
        level++;
        // First time we want to initiate retrieval of setup packet response
        hcd_edpt_xfer(0, 0, 0x80, bugger, 18);
    }
    else {
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
    }
    //hcd_edpt_xfer(0, dev_addr, ep_addr, bugger, 1);
}

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const * setup, bool in_isr){
    spi_send_blocking(setup, 8, USB_DATA | DEBUG_PRINT_AS_HEX);
}


void dcd_event_xfer_complete_new (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr){
    // TODO
    spi_send_string("Transfer complete. Result and transferred bytes:");
    uint8_t data[2] = {result, xferred_bytes};
    sleep_ms(100);
    spi_send_blocking(data, 2, DEBUG_PRINT_AS_HEX);
}
