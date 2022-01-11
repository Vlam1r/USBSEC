// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/10/22.
//

#ifndef PICO_FIRMWARE_SPI_QUEUE_H
#define PICO_FIRMWARE_SPI_QUEUE_H

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "../debug/debug.h"

#define SPI_QUEUE_MAX_CAPACITY 20

typedef struct {
    uint16_t e_flag;
    uint16_t payload_length;
    uint8_t *payload;
} spi_message_t;


#endif //PICO_FIRMWARE_SPI_QUEUE_H
