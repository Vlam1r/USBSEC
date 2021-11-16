//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];
bool cfg_set = false;

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr) {
    tusb_control_request_t *const req = (tusb_control_request_t *) setup;
    if (req->bRequest == 0x5 /*SET ADDRESS*/) {
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
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX | 0x00); // TODO!

    int len = spi_await(bugger, USB_DATA);

    /*
     * Hooks
     */
    if (setup[3] == 0x2 && setup[6] > 9) { // TODO Harden
        // Endpoints
        printf("doing endpoints.\n");
        int pos = 0;
        while (pos < len) {
            if (bugger[pos + 1] == 0x05) {
                const tusb_desc_endpoint_t *const edpt = (const tusb_desc_endpoint_t *const) &bugger[pos];
                dcd_edpt_open_new(rhport, edpt);
                // Start read
                printf("0x%x\n", edpt->bEndpointAddress);
                if (edpt->bEndpointAddress == 2) {
                    dcd_edpt_xfer_new(rhport, edpt->bEndpointAddress, bugger, 64);
                }
                spi_send_blocking((const uint8_t *) edpt, edpt->bLength, EDPT_OPEN); // TODO ONLY IF INTERRUPT
                spi_await(bugger, USB_DATA);
            }
            pos += bugger[pos];
        }
    }
    if (req->bRequest == 0x09 /* SET CONFIG */) {
        cfg_set = true;
        printf("Configuration confirmed.\n");
    }

    dcd_edpt_xfer_new(0, 0x80, bugger, len);
    dcd_edpt_xfer_new(rhport, 0x00, NULL, 0);
}


void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr) {

    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        //spi_send_blocking(&usb_dpram->setup_packet, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);
        //assert(spi_await(bugger, USB_DATA) == 0);
        printf("Setting address to %d, [%d] %d\n", ((const tusb_control_request_t *) usb_dpram->setup_packet)->wValue,
               ep_addr,
               xferred_bytes);
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
        return;
    }

    if (ep_addr == 0 || ep_addr == 0x80) return;


    printf("+-----\n|Completed transfer on %d\n+-----\n", ep_addr);

    if (ep_addr & 0x80) return;
    if (xferred_bytes == 0 || result != 0) return;

    dcd_edpt_xfer_new(rhport, ep_addr, bugger, 64);

    // TODO HARDEN THIS INTERACTION
    /*bugger[xferred_bytes] = 1;
    spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
    spi_await(bugger, USB_DATA);
    memset(bugger, 0, 64);
    bugger[64] = 0;
    spi_send_blocking(bugger, 64 + 1, USB_DATA);
    int len = spi_await(bugger, USB_DATA);*/
    bugger[xferred_bytes] = 1;
    spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
    spi_await(bugger, USB_DATA);
    printf("Polling start");
    bool gottem = false;
    while (!gottem) {
        gottem = true;
        for (int i = 0; i < 2; i++) {
            memset(bugger, 0, 64);
            bugger[64] = i;
            printf("Poll %d", i);
            trigger_spi_irq();
            spi_send_blocking(bugger, 64 + 1, USB_DATA);
        }
        trigger_spi_irq();
        spi_send_blocking(NULL, 0, EVENTS);
        spi_receive_blocking(bugger);
        int count = bugger[0];
        while (count--) {
            gottem = false;
            int len = spi_receive_blocking(bugger);
            dcd_edpt_xfer_new(rhport, bugger[len - 1], bugger, len - 1);
        }
    }

}

void device_event_bus_reset() {
    printf("Resetting\n");
    spi_send_blocking(NULL, 0, RESET_USB);
}
