//
// Created by vlamir on 11/6/21.
//

#include <hardware/structs/usb.h>
#include "usb_event_handlers.h"

static uint8_t setup_packet[8];
uint8_t bugger[1000];

static int level = 0;

/*
 * Host events
 */

void define_setup_packet(uint8_t *setup){
    memcpy(setup_packet, setup, 8);
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    level = 0;
    hcd_setup_send(rhport, 0, setup_packet);
    uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                       0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                       0x03, 0x01};
    spi_send_blocking(arr, 18, USB_DATA | DEBUG_PRINT_AS_HEX);

    /**/
    uint8_t setup2[8] = {0x00, 0x05, 0x06, 0x00 ,0x00 ,0x00, 0x00, 0x00};
    hcd_edpt_xfer(0, 0x00, 0x00, setup2, 8); //Send second setup
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {

}

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    hw_endpoint_t *ep = get_dev_ep(dev_addr, ep_addr);
    //spi_send_string("Transfer complete.");
    uint8_t data[2] = {result, xferred_bytes};
    spi_send_blocking(data, 2, DEBUG_PRINT_AS_HEX);
    if(level == 0){
        level++;
        // First time we want to initiate retrieval of setup packet response
        uint16_t len = ((tusb_control_request_t *) setup_packet)->wLength;
        hcd_edpt_xfer(0, 0, 0x80, bugger, len);
    }
    else {
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);

    }
    //hcd_edpt_xfer(0, dev_addr, ep_addr, bugger, 1);
}

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const * setup, bool in_isr){
    uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                       0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                       0x03, 0x01};
    /*tusb_desc_endpoint_t ep0_desc =
            {
                    .bLength          = sizeof(tusb_desc_endpoint_t),
                    .bDescriptorType  = TUSB_DESC_ENDPOINT,
                    .bEndpointAddress = 0,
                    .bmAttributes     = { .xfer = TUSB_XFER_CONTROL },
                    .wMaxPacketSize   = { .size = 64 },
                    .bInterval        = 0
            };

    dcd_edpt_open_new(0, &ep0_desc);*/
    if(level == 0) {
        dcd_edpt_xfer_new(0, 0x80, arr, 18);
        dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
        level++;
    } else if (level == 1) {
        //REPLY TO SET_ADDRESS
        dcd_edpt_xfer_new(rhport, 0x80, NULL, 0); //send empty
        //dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
        level++;
    } else if (level == 2){
        level++;
        dcd_edpt_xfer_new(0, 0x80, arr, 18);
        dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
    }
}


void dcd_event_xfer_complete_new (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr){
    // TODO
    uint8_t data[3] = {0x65, result, xferred_bytes};
    if(level == 2){
        //spi_send_blocking(data, 3, DEBUG_PRINT_AS_HEX);
        dcd_edpt0_status_complete(0, usb_dpram->setup_packet); // Update address
    }
    //spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
}
