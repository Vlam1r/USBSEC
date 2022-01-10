// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/15/21.
//

#include "event_queue.h"

static event_t events[EVENT_QUEUE_MAX_CAPACITY];

queue_t q = {
        .SIZE = sizeof(event_t),
        .CAPACITY = EVENT_QUEUE_MAX_CAPACITY,
        .LOC = events,
        .begin = 0,
        .end = 0
};

uint8_t queue_size(void) {
    return q_size(&q);
}

event_t *get_from_event_queue(void) {
    return q_get(&q);
}

void create_event(event_t *e) {

    event_t *inserted = q_put(&q, e);

    // Also copy the payload
    if (inserted->payload_length != 0) {
        inserted->payload = malloc(inserted->payload_length + 1);
        if (inserted->payload == NULL) {
            panic("OUT OF MEMORY!!!");
        }
        memcpy(inserted->payload, e->payload, inserted->payload_length);
    }
    //debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Insert %d. New range [%d-%d]\n", e->e_flag, begin, end);
}

void delete_event(event_t *e) {
    if (e->payload_length != 0) {
        debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Freeing array of size %d\n", e->payload_length);
        free(e->payload);
        debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Freed array of size %d\n", e->payload_length);
    }
}
