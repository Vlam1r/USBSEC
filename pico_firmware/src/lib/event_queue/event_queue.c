// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/15/21.
//

#include "event_queue.h"

static event_t events[EVENT_QUEUE_MAX_CAPACITY];
static int begin = 0;
static int end = 0;

static bool empty(void) {
    return begin == end;
}

static int next(int pos) {
    return (pos + 1) % EVENT_QUEUE_MAX_CAPACITY;
}

static bool full(void) {
    return next(end) == begin;
}

uint8_t queue_size(void) {
    return (end + EVENT_QUEUE_MAX_CAPACITY - begin) % EVENT_QUEUE_MAX_CAPACITY;
}

event_t *get_from_event_queue(void) {
    if (empty()) return NULL;
    int retidx = begin;
    begin = next(begin);
    debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Delete. New range [%d-%d]\n", begin, end);
    return &events[retidx];
}

void create_event(event_t *e) {
    if (full()) panic("EVENT QUEUE FULL!!!");

    memcpy(&events[end], e, sizeof(event_t));
    if (events[end].payload_length != 0) {
        events[end].payload = malloc(events[end].payload_length + 1);
        if (events[end].payload == NULL) {
            panic("OUT OF MEMORY!!!");
        }
        memcpy(events[end].payload, e->payload, events[end].payload_length);
    }
    end = next(end);
    debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Insert %d. New range [%d-%d]\n", e->e_type, begin, end);
}

void delete_event(event_t *e) {
    if (e->payload_length != 0) {
        debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Freeing array of size %d\n", e->payload_length);
        free(e->payload);
        debug_print(PRINT_REASON_EVENT_QUEUE, "[EVENT_QUEUE] Freed array of size %d\n", e->payload_length);
    }
}
