// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/23/22.
//

#ifndef PICO_FIRMWARE_SETUP_RESPONSE_AGGREGATOR_H
#define PICO_FIRMWARE_SETUP_RESPONSE_AGGREGATOR_H

#include "../messages/messages.h"

void register_response(spi_message_t *message);

uint8_t *get_concatenated_response();

uint16_t get_concatenated_response_len();

bool send_packet_upstream();

void set_max_packet_size(uint8_t new_mps);

#endif //PICO_FIRMWARE_SETUP_RESPONSE_AGGREGATOR_H
