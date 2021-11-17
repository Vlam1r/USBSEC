// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/15/21.
//

#ifndef PICO_FIRMWARE_EVENT_QUEUE_H
#define PICO_FIRMWARE_EVENT_QUEUE_H

#include "pico/stdlib.h"
#include <string.h>
#include <malloc.h>
#include <pico/printf.h>
#include "../debug/debug.h"

#define EVENT_QUEUE_MAX_CAPACITY 20

typedef enum {
    DEVICE_EVENT_SETUP_RECEIVED,
    DEVICE_EVENT_XFER_COMPLETE,
    DEVICE_EVENT_BUS_RESET,
    SLAVE_SPI_WAITING_TO_SEND
} event_type;

typedef struct {
    event_type e_type;
    uint16_t payload_length;
    uint8_t *payload;
    /**/
    uint8_t ep_addr;
} event_t;

uint8_t queue_size(void);

event_t *get_from_event_queue(void);

bool create_event(event_t *e);

void delete_event(event_t *e);

#endif //PICO_FIRMWARE_EVENT_QUEUE_H
