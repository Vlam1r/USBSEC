// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/15/21.
//

#ifndef PICO_FIRMWARE_EVENT_QUEUE_H
#define PICO_FIRMWARE_EVENT_QUEUE_H

#include "pico/stdlib.h"
#include <string.h>

#define EVENT_QUEUE_MAX_CAPACITY 20

typedef struct {
    uint8_t payload[300];
    uint8_t *length;
} event_t;

uint8_t queue_size(void);

event_t *get_from_event_queue(void);

bool insert_into_event_queue(event_t *e);

#endif //PICO_FIRMWARE_EVENT_QUEUE_H
