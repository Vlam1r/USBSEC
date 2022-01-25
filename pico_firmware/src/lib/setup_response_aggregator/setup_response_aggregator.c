// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/23/22.
//

#include "setup_response_aggregator.h"
#include <string.h>
#include "../usb_event_handlers/usb_event_handlers.h"

/*
 * Microsoft LifeChat LX-300 had a response of over 300 bytes
 */
#define MAX_SETUP_RESPONSE_BUGGER 1024

static uint8_t bugger[MAX_SETUP_RESPONSE_BUGGER];
static uint8_t mps = 64;
static uint8_t idx = 0;
static uint16_t len = 0;

void register_response(spi_message_t *message) {
    memcpy(bugger + len, message->payload, message->payload_length);
    len += message->payload_length;
}

uint8_t *get_concatenated_response(void) {
    return bugger;
}

uint16_t get_concatenated_response_len(void) {
    return len;
}

bool send_packet_upstream(void) {
    uint8_t xfer_len = len < mps ? len : mps;
    dcd_edpt_xfer_new(0, 0x80, bugger + mps * idx, xfer_len);
    len -= xfer_len;
    idx++;

    if (len == 0) {
        idx = 0;
        return true;
    }

    return false;
}

void set_max_packet_size(uint8_t new_mps) {
    mps = new_mps;
}
