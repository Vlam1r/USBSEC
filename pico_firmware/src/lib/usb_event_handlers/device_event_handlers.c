//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const * setup, bool in_isr) {
    uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                       0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                       0x03, 0x01};

    if (((tusb_control_request_t *) setup)->bRequest == 0x5 /*SET ADDRESS*/) {
        dcd_edpt_xfer_new(rhport, 0x80, NULL, 0); //send empty
        return;
    }
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
    int len = spi_receive_blocking(bugger);

    dcd_edpt_xfer_new(0, 0x80, bugger, len);
    dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
}


void dcd_event_xfer_complete_new (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr){
    // TODO
    if(((tusb_control_request_t *)usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {

        //spi_send_blocking(data, 3, DEBUG_PRINT_AS_HEX);
        dcd_edpt0_status_complete(0, usb_dpram->setup_packet); // Update address
    } else {
        //bugger[xferred_bytes] = ep_addr;
        //spi_send_blocking(bugger, xferred_bytes+1, USB_DATA | DEBUG_PRINT_AS_HEX);
    }
}
