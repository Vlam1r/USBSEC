//
// Created by vlamir on 11/6/21.
//

#include "usb_event_handlers.h"

static uint8_t bugger[1000];

/*
 * Device Events
 */

void dcd_event_setup_received_new(uint8_t rhport, uint8_t const *setup, bool in_isr) {
    assert(rhport == 0);
    assert(in_isr);
    event_t e = {
            .e_type = DEVICE_EVENT_SETUP_RECEIVED,
            .payload_length = 8,
            .payload = (uint8_t *) setup
    };
    debug_print(PRINT_REASON_USB_EXCHANGES, "Recieved setup:\n");
    debug_print_array(PRINT_REASON_USB_EXCHANGES, setup, 8);
    create_event(&e);
}

void device_event_bus_reset(void) {
    event_t e = {
            .e_type = DEVICE_EVENT_BUS_RESET,
            .payload_length = 0,
            .payload = NULL
    };
    create_event(&e);
}

void dcd_event_xfer_complete_new(uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr) {
    assert(result == 0);
    assert(rhport == 0);
    event_t e = {
            .e_type = DEVICE_EVENT_XFER_COMPLETE,
            .payload_length = xferred_bytes,
            .payload = bugger,
            .ep_addr = ep_addr
    };
    if (~ep_addr & 0x80) {
        debug_print(PRINT_REASON_USB_EXCHANGES, "Recieved data:\n");
        debug_print_array(PRINT_REASON_USB_EXCHANGES, bugger, xferred_bytes);
    }
    create_event(&e);
}

static void register_spi_slave_event(void) {
    event_t e = {
            .e_type = SLAVE_SPI_WAITING_TO_SEND,
            .payload_length = 0,
            .payload = NULL
    };
    create_event(&e);
}

static void handle_setup_event(uint8_t const *setup) {

    tusb_control_request_t *const req = (tusb_control_request_t *) setup;
    if (req->bRequest == 0x5 /*SET ADDRESS*/) {
        /*
         * If request is SET_ADDRESS we do not want to pass it on.
         * Instead, it gets handled here and slave keeps communicating to device on dev_addr 0
         */
        dcd_edpt_xfer_new(0, 0x80, NULL, 0); // ACK
        return;
    }

    /*
     * Forward setup packet to slave and get response from device
     */
    spi_send_blocking(setup, 8, SETUP_DATA | DEBUG_PRINT_AS_HEX);

    uint8_t arr[100];
    int len = spi_await(arr, USB_DATA);

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
                dcd_edpt_xfer_new(0, edpt->bEndpointAddress, bugger, 64);
                spi_send_blocking((const uint8_t *) edpt, edpt->bLength, EDPT_OPEN); // TODO ONLY IF INTERRUPT
                insert_into_registry(edpt);
                spi_await(arr, USB_DATA);
            }
            pos += arr[pos];
        }
    }
    if (req->bRequest == 0x09 /* SET CONFIG */) {
        debug_print(PRINT_REASON_SETUP_REACTION, "Configuration confirmed.\n");
    }
    dcd_edpt_xfer_new(0, 0x80, arr, len);
    dcd_edpt_xfer_new(0, 0x00, NULL, 0);
}

static void handle_xfer_complete(uint8_t ep_addr, uint8_t *data, uint32_t xferred_bytes) {

    if (((tusb_control_request_t *) usb_dpram->setup_packet)->bRequest == 0x5 /*SET ADDRESS*/) {
        debug_print(PRINT_REASON_SETUP_REACTION,
                    "Setting address to %d, [%d] %d\n",
                    ((const tusb_control_request_t *) usb_dpram->setup_packet)->wValue,
                    ep_addr,
                    xferred_bytes);
        dcd_edpt0_status_complete(0, (const tusb_control_request_t *) usb_dpram->setup_packet); // Update address
        return;
    }

    if (ep_addr == 0 || ep_addr == 0x80) return;
    printf("\n+-----\n|Completed transfer on %d\n+-----\n", ep_addr);
    if (xferred_bytes == 0) return;


    if (~ep_addr & 0x80) {
        data[xferred_bytes] = 1; //TODO
        spi_send_blocking(data, xferred_bytes + 1, USB_DATA | DEBUG_PRINT_AS_HEX);
        spi_await(data, USB_DATA);
        //trigger_spi_irq();
        dcd_edpt_xfer_new(0, ep_addr, data, 64);
    } else {
        uint8_t arr[100];
        memset(arr, 0, 64);
        arr[64] = 0;
        printf("Poll %d [0x%x]\n", 0, 0x81);
        spi_send_blocking(arr, 64 + 1, USB_DATA);

        printf("Waiting idle \n");
        int len = spi_await(arr, USB_DATA);
        //trigger_spi_irq();
        dcd_edpt_xfer_new(0, 0x81, bugger, len);
    }
}

void spi_handler_init(void) {
    set_spi_pin_handler(register_spi_slave_event);
}

static void handle_spi_slave_event(void) {

}

void masterwork(void) {

    if (!gpio_get(GPIO_SLAVE_DEVICE_ATTACHED_PIN)) {
        /*
         * No device attached. Nothing to do.
         */
        return;
    }

    event_t *e = get_from_event_queue();
    if (e == NULL) return;
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    switch (e->e_type) {

        case DEVICE_EVENT_SETUP_RECEIVED:
            debug_print(PRINT_REASON_EVENT, "[EVENT] Handling setup event\n");
            handle_setup_event(e->payload);
            break;

        case DEVICE_EVENT_XFER_COMPLETE:
            debug_print(PRINT_REASON_EVENT, "[EVENT] Handling xfer event\n");
            handle_xfer_complete(e->ep_addr, e->payload, e->payload_length);
            break;

        case DEVICE_EVENT_BUS_RESET:
            debug_print(PRINT_REASON_EVENT, "[EVENT] Handling bus reset\n");
            spi_send_blocking(NULL, 0, RESET_USB);
            sleep_ms(1000);
            break;

        case SLAVE_SPI_WAITING_TO_SEND:
            handle_spi_slave_event();
            break;

        default:
            panic("Unknown event type %d", e->e_type);
    }

    printf("BANDAID PRINT\n");
    delete_event(e);
    debug_print(PRINT_REASON_EVENT, "Exiting masterwork\n");
}
