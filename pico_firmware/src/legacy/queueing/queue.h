// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/10/22.
//

#ifndef PICO_FIRMWARE_QUEUE_H
#define PICO_FIRMWARE_QUEUE_H

#include "pico/stdlib.h"
#include <string.h>
#include "../debug/debug.h"


typedef struct {
    const uint16_t CAPACITY; // Max number of elements in queue
    const uint16_t SIZE; // Size of element in queue
    void *const LOC; // Location of queue in memory
    uint16_t begin;
    uint16_t end;
} queue_t;


uint8_t q_size(queue_t *q);

void *q_get(queue_t *q);

void *q_put(queue_t *q, void *e);


#endif //PICO_FIRMWARE_QUEUE_H
