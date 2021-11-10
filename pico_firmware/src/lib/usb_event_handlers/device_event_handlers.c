//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const * setup, bool in_isr) {
    if (((tusb_control_request_t *) setup)->bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we do not want to pass it on.
         * Instead, it gets handled here and slave keeps communicating to device on dev_addr 0
         */
        dcd_edpt_xfer_new(rhport, 0x80, NULL, 0); // ACK
        return;
    }
    /*
     * Forward setup packet to slave and get response from device
     */
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
    int len = spi_receive_blocking(bugger);

    dcd_edpt_xfer_new(0, 0x80, bugger, len);
    dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
}


void dcd_event_xfer_complete_new (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr){
    if(((tusb_control_request_t *)usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
    } else {
        // TODO Data not handled!
        //bugger[xferred_bytes] = ep_addr;
        //spi_send_blocking(bugger, xferred_bytes+1, USB_DATA | DEBUG_PRINT_AS_HEX);
        //int len = spi_receive_blocking(bugger);
        //dcd_edpt_xfer_new(0, ep_addr, bugger, len);
    }
}
