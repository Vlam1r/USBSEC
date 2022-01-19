// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];
bool permit_0x81 = false;
int count0x81 = 0;
uint8_t other_edpt;

tusb_control_request_t setup;

/*
 * Device Events
 */

static void handle_setup_response(spi_message_t *msg);

void handle_spi_slave_event(void) {

    spi_message_t msg;
    while (dequeue_spi_message(&msg)) {
        uint8_t ep_addr = msg.payload[--msg.payload_length];
        debug_print(PRINT_REASON_SLAVE_DATA,
                    "[SLAVE DATA] Packet for 0x%x of length %d with flag 0x%x\n",
                    ep_addr, msg.payload_length, msg.e_flag);
        assert(msg.e_flag & IS_PACKET);
        if (msg.e_flag & SETUP_DATA) {
            handle_setup_response(&msg);
        } else if (~ep_addr & 0x80) {
            /*
             * OUT endpoint
             */
            memset(bugger, 0, 64);
            bugger[64] = other_edpt; // TODO
            debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", other_edpt);
            spi_message_t reply = {
                    .payload = bugger,
                    .payload_length = 64 + 1,
                    .e_flag = USB_DATA
            };
            enqueue_spi_message(&reply); // TODO Optimize sending of 64 bits

            dcd_edpt_xfer_new(0, ep_addr, bugger, 64);
            debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Listening to port 0x%x\n", ep_addr);
        } else {
            /*
             * IN endpoint
             */
            permit_0x81 = true;
            count0x81++;
            printf("+++++ STARTING PARTIAL XFER ON %d++++\n", ep_addr);
            dcd_edpt_xfer_partial(ep_addr, msg.payload, msg.payload_length, msg.e_flag);
        }
    }
    //debug_print(PRINT_REASON_SLAVE_DATA, "[SLAVE DATA] Done.\n");
}

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *p_setup, bool in_isr) {
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    memcpy(&setup, p_setup, sizeof setup);
    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Received new setup.\n");

    if (setup.bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we have to also change the address on slave.
         * Some devices will break if they don't get their address changed!
         */
        spi_message_t msg = {
                .payload_length = 0,
                .payload = NULL,
                .e_flag = CHG_ADDR
        };
        enqueue_spi_message(&msg);
        return;
    }

    /*
     * Forward setup packet to slave and get response from device
     */
    spi_message_t msg = {
            .payload_length = sizeof(setup),
            .payload = &setup,
            .e_flag = SETUP_DATA | DEBUG_PRINT_AS_HEX
    };
    enqueue_spi_message(&msg);
}

static void handle_setup_response(spi_message_t *msg) {
    uint8_t *arr = msg->payload;
    uint16_t len = msg->payload_length;
    /*
     * Hooks
     */
    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        dcd_edpt_xfer_new(0, 0x80, NULL, 0); // ACK
        return;
    }
    if (((uint8_t *) &setup)[3] == 0x2 && ((uint8_t *) &setup)[6] > 9) { // TODO Harden
        /*
         * Endpoints
         */
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

                spi_message_t reply = {
                        .payload = edpt,
                        .payload_length = edpt->bLength,
                        .e_flag = EDPT_OPEN
                };
                enqueue_spi_message(&reply); // TODO ONLY IF INTERRUPT?
                insert_into_registry(edpt);
            }
            pos += arr[pos];
        }
    }
    if (setup.bRequest == 0x09 /* SET CONFIG */) {
        debug_print(PRINT_REASON_SETUP_REACTION, "Configuration confirmed.\n");
    }

    debug_print(PRINT_REASON_SETUP_REACTION, "[SETUP] Setup handling ended.\n");
    dcd_edpt_xfer_new(0, 0x80, arr, len);
    if (setup.bmRequestType_bit.direction == 1) {
        dcd_edpt_xfer_new(0, 0x00, NULL, 0); //STATUS?
    }
}

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
        spi_message_t msg = {
                .payload_length = xferred_bytes + 1,
                .payload = bugger,
                .e_flag = USB_DATA | DEBUG_PRINT_AS_HEX
        };
        enqueue_spi_message(&msg);
    } else if (permit_0x81) {
        if (xferred_bytes == 13 && count0x81 > 0) {
            debug_print(PRINT_REASON_XFER_COMPLETE,
                        "+++[XFER COMPLETE] EMERGENCY BAILOUT BECAUSE SCSI COMPLETED EARLY+++\n");
            return;
        }
        memset(arr, 0, 64);
        arr[64] = ep_addr;
        debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Sent to 0x%x.\n", ep_addr);
        spi_message_t msg = {
                .payload_length = 64 + 1,
                .payload = arr,
                .e_flag = USB_DATA
        };
        enqueue_spi_message(&msg);
    }
    debug_print(PRINT_REASON_XFER_COMPLETE, "[XFER COMPLETE] Leaving.\n");
}

void device_event_bus_reset() {
    debug_print(PRINT_REASON_IRQ, "[IRQ] --- RESETTING BUS ---\n");
    while (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) tight_loop_contents();
    spi_message_t msg = {
            .payload = NULL,
            .payload_length = 0,
            .e_flag = RESET_USB
    };
    enqueue_spi_message(&msg);
}
