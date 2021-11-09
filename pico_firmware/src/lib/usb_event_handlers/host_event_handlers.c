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
    //spi_send_blocking(bugger, len, get_flag());
    if(get_flag() & SETUP_DATA) {
        define_setup_packet(bugger);
        if(!attached) return;
        usb_hw->sie_ctrl |= USB_SIE_CTRL_RESET_BUS_BITS;
        hcd_setup_send(0, 0, setup_packet);
    } else if (get_flag() & USB_DATA){
        //hcd_edpt_xfer(0, 0, bugger[len-1] ^ 0x80, bugger, len-1);
    }
}

void hcd_event_device_attach(uint8_t rhport, bool in_isr) {
    //if(level == 0){
    level = 0;
    attached = true;
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
    } else {
        spi_send_blocking(bugger, xferred_bytes, USB_DATA | DEBUG_PRINT_AS_HEX);
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
