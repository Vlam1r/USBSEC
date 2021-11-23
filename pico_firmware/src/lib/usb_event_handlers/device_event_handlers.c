//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];
bool handling_setup = false;
bool permit_0x81 = false;
int count0x81 = 0;
uint8_t other_edpt;

/*
 * Device Events
 */

static void handle_spi_slave_event(void) {
    if (handling_setup) return;
    uint8_t arr[100];
    spi_send_blocking(NULL, 0, SLAVE_DATA);
    uint8_t count;
    assert(spi_receive_blocking(&count) == 1);
    debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Reading %d packets.\n", count);
    while (count--) {
        int len = spi_receive_blocking(arr);
        uint8_t ep_addr = arr[--len];
        debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Packet for 0x%x\n", ep_addr);
        if (~ep_addr & 0x80) {
            memset(arr, 0, 64);
            arr[64] = other_edpt; // TODO
            debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", other_edpt);
            spi_send_blocking(arr, 64 + 1, USB_DATA);

            dcd_edpt_xfer_new(0, ep_addr, bugger, 64);
            debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Listening to port 0x%x\n", ep_addr);
        } else {
            permit_0x81 = true;
            count0x81++;
            dcd_edpt_xfer_new(0, ep_addr, arr, len);
        }
    }
    debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Done.\n");
}

int get_only_response(uint8_t *data) {
    while (!gpio_get(GPIO_SLAVE_IRQ_PIN)) {
        tight_loop_contents();
    }
    spi_send_blocking(NULL, 0, SLAVE_DATA);
    uint8_t count;
    int len = spi_receive_blocking(&count);
    debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Received len %d, count %d\n", len, count);
    assert(len == 1);
    assert(count == 1);
    return spi_receive_blocking(data);
}

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr) {
    //debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Setup handling started.\n");
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    tusb_control_request_t *const req = (tusb_control_request_t *) setup;
    set_spi_pin_handler(handle_spi_slave_event);
    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Received new setup.\n");
    if (req->bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we do not want to pass it on. TODO
         * Instead, it gets handled here and slave keeps communicating to device on dev_addr 0
         */
        spi_send_blocking(NULL, 0, CHG_ADDR);
        uint8_t arr[10];
        get_only_response(arr);
        dcd_edpt_xfer_new(0, 0x80, NULL, 0); // ACK
        return;
    }

    /*
     * Forward setup packet to slave and get response from device
     */
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);

    uint8_t arr[100];
    //int len = spi_await(arr, USB_DATA);
    int len = get_only_response(arr) - 1;

    /*
     * Hooks
     */
    if (setup[3] == 0x2 && setup[6] > 9) { // TODO Harden
        // Endpoints
        debug_print(PRINT_REASON_SETUP_REACTION, "Started handling endpoints.\n");
        int pos = 0;
        while (pos < len) {
            if (arr[pos + 1] == 0x05) {
                const tusb_desc_endpoint_t *const edpt = (const tusb_desc_endpoint_t *const) &arr[pos];
                dcd_edpt_open_new(0, edpt);
                // Start read
                debug_print(PRINT_REASON_SETUP_REACTION, "New endpoint registered: 0x%x\n", edpt->bEndpointAddress);
                if (~edpt->bEndpointAddress & 0x80)
                    dcd_edpt_xfer_new(0, edpt->bEndpointAddress, bugger, 64); // Query OUT edpt
                else
                    other_edpt = edpt->bEndpointAddress;
                spi_send_blocking((const uint8_t *) edpt, edpt->bLength, EDPT_OPEN); // TODO ONLY IF INTERRUPT?
                insert_into_registry(edpt);
            }
            pos += arr[pos];
        }
    }
    if (req->bRequest == 0x09 /* SET CONFIG */) {
        debug_print(PRINT_REASON_SETUP_REACTION, "Configuration confirmed.\n");
    }
    handling_setup = false;
    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Setup handling ended.\n");
    dcd_edpt_xfer_new(0, 0x80, arr, len);
    if (req->bmRequestType_bit.direction == 1) {
        dcd_edpt_xfer_new(0, 0x00, NULL, 0); //STATUS?
    }
}

bool first_time = true;


void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr) {

    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/ && ep_addr == 0x80) {
        debug_print(PRINT_REASON_SETUP_REACTION,
                    "Setting address to %d, [%d] %d\n",
                    ((const tusb_control_request_t *) usb_dpram->setup_packet)->wValue,
                    ep_addr,
                    xferred_bytes);
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
        return;
    }

    if (ep_addr == 0 || ep_addr == 0x80) return;
    debug_print(PRINT_REASON_XFER_COMPLETE,
                "[XFER COMPLETE] Completed transfer on 0x%x with %d bytes\n",
                ep_addr, xferred_bytes);

    uint8_t arr[100];
    if (~ep_addr & 0x80) {
        permit_0x81 = false;
        count0x81 = 0;
        bugger[xferred_bytes] = ep_addr; //TODO
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", ep_addr);
        spi_send_blocking(bugger, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
        if (first_time) {
            //first_time = false;
        }
    } else if (permit_0x81) {
        if (xferred_bytes == 13 && count0x81 > 0) {
            debug_print(PRINT_REASON_XFER_COMPLETE,
                        "+++[XFER COMPLETE] EMERGENCY BAILOUT BECAUSE SCSI COMPLETED EARLY+++\n");
            return;
        }
        memset(arr, 0, 64);
        arr[64] = ep_addr;
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", ep_addr);
        spi_send_blocking(arr, 64 + 1, USB_DATA);
    }
    debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Leaving.\n");
}

void device_event_bus_reset() {
    printf("Resetting\n");
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    spi_send_blocking(NULL, 0, RESET_USB);
}
