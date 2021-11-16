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
    return (end - begin) % EVENT_QUEUE_MAX_CAPACITY;
}

event_t *get_from_event_queue(void) {
    if (empty()) return NULL;
    int retidx = begin;
    begin = (begin + 1) % EVENT_QUEUE_MAX_CAPACITY;
    return &events[retidx];
}

bool insert_into_event_queue(event_t *e) {
    if (full()) return false;
    memcpy(&events[end], e, sizeof(event_t));
    end = next(end);
    return true;
}
