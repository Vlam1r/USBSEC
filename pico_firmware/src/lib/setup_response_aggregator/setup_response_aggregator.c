// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/23/22.
//

#include "setup_response_aggregator.h"
#include <string.h>
#include "../usb_event_handlers/usb_event_handlers.h"

static uint8_t bugger[64];
static uint8_t mps = 64;
static uint8_t idx = 0;
static int16_t len = 0;

void register_response(spi_message_t *message) {
    memcpy(bugger + idx * mps, message->payload, message->payload_length);
    len += message->payload_length;
    idx++;
}

uint8_t *get_concatenated_response(void) {
    return bugger;
}

uint16_t get_concatenated_response_len(void) {
    return len;
}

void send_packets_upstream(void) {
    idx = 0;
    while (len > 0) {
        dcd_edpt_xfer_new(0, 0x80, bugger + idx * mps, len < mps ? len : mps);
        len -= mps;
        idx++;
    }
    idx = 0;
    len = 0;
}

void set_max_packet_size(uint8_t new_mps) {
    mps = new_mps;
}
