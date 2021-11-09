//
// Created by vlamir on 11/6/21.
//

#include <hardware/structs/usb.h>
#include <hardware/irq.h>
#include "usb_event_handlers.h"

static uint8_t setup_packet[8];
uint8_t bugger[1000];

volatile uint8_t level = 0;

/*
 * Host events
 */


void define_setup_packet(uint8_t *setup){
    memcpy(setup_packet, setup, 8);
}

void slavework() {
    int len = spi_receive_blocking(bugger);
    //spi_send_blocking(bugger, len, get_flag());
    if(get_flag() & SETUP_DATA) {
        define_setup_packet(bugger);
        usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
        hcd_setup_send(0, 0, setup_packet);
    } else if (get_flag() & USB_DATA){
        hcd_edpt_xfer(0, 0, 0x80, bugger, len);
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    //if(level == 0){
    level = 0;
    hcd_setup_send(rhport, 0, setup_packet);
    /*}
    else {
        spi_send_blocking(&level, 1, DEBUG_PRINT_AS_HEX);
        uint8_t setup2[8] = {0x80, 0x6, 0x0, 0x2, 0x0, 0x0, 0x2c, 0x0};
        hcd_setup_send(0, 0, setup2);
    }*/
    /*//hcd_edpt_xfer(0, 0, 0x00, bugger, 0x40);
    uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                       0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                       0x03, 0x01};
    //spi_send_blocking(arr, 18, USB_DATA | DEBUG_PRINT_AS_HEX);

    uint8_t setup2[8] = {0x00, 0x05, 0x06, 0x00 ,0x00 ,0x00, 0x00, 0x00};
    //hcd_edpt_xfer(0, 0x00, 0x00, setup2, 8); //Send second setup*/
}

void hcd_event_device_remove(uint8_t rhport, bool in_isr) {

}

enum {
    SIE_CTRL_BASE = USB_SIE_CTRL_SOF_EN_BITS      | USB_SIE_CTRL_KEEP_ALIVE_EN_BITS |
                    USB_SIE_CTRL_PULLDOWN_EN_BITS | USB_SIE_CTRL_EP0_INT_1BUF_BITS
};

void hcd_event_xfer_complete(uint8_t dev_addr, uint8_t ep_addr, uint32_t xferred_bytes, int result, bool in_isr) {
    //spi_send_string("Transfer complete.");
    uint8_t data[2] = {result, xferred_bytes};
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
    }
        /*gpio_put(PICO_DEFAULT_LED_PIN,0);
        level = 2;
        usb_hw->sie_ctrl = SIE_CTRL_BASE | USB_SIE_CTRL_RESET_BUS_BITS;
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        uint8_t setup2[8] = {0x80, 0x6, 0x0, 0x2, 0x0, 0x0, 0x2c, 0x0};
        hcd_setup_send(0, 0, setup2);
        //hcd_edpt_xfer(0, 0, 0x80, setup2, 8);
    } else if (level == 2){
        level = 3;
    } else if (level == 3){
        level = 4;
        uint16_t len = ((tusb_control_request_t *) setup_packet)->wLength;
        hcd_edpt_xfer(0, 0, 0x80, bugger, len);
    } else {
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
    }*/
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

    if(((tusb_control_request_t *)setup)->bRequest == 0x5 /*SET ADDRESS*/) {
        dcd_edpt_xfer_new(rhport, 0x80, NULL, 0); //send empty
        level++;
        return;
    }
    gpio_put(PICO_DEFAULT_LED_PIN,1);
    {
        //irq_set_enabled(USBCTRL_IRQ, false);
        spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        int len;
        do{
            len = spi_receive_blocking(bugger);
        } while(!(get_flag() & USB_DATA));
        //gpio_put(PICO_DEFAULT_LED_PIN,0);
        //spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        //spi_receive_blocking(bugger);*/

        dcd_edpt_xfer_new(0, 0x80, bugger, len);
        if(len == 64)
            gpio_put(PICO_DEFAULT_LED_PIN,0);
        dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
        //irq_set_enabled(USBCTRL_IRQ, false);
        //spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        //gpio_put(PICO_DEFAULT_LED_PIN,0);
        //int len = spi_receive_blocking(bugger);
        //dcd_edpt_xfer_new(0, 0x80, bugger, len);
        //irq_set_enabled(USBCTRL_IRQ, true);
        //dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
        level++;
    } /*else if (level == 1){
        level++;
        //dcd_edpt_xfer_new(0, 0x80, arr, 18);
        //dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
        spi_send_blocking(setup, 8, SETUP_DATA);
        int len = spi_receive_blocking(bugger);
        dcd_edpt_xfer_new(0, 0x80, bugger, len);
    }*/
}


void dcd_event_xfer_complete_new (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr){
    // TODO
    if(((tusb_control_request_t *)usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        //spi_send_blocking(data, 3, DEBUG_PRINT_AS_HEX);
        dcd_edpt0_status_complete(0, usb_dpram->setup_packet); // Update address
    }
    //spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
}
