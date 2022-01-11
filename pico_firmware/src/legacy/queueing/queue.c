// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/10/22.
//


#include "queue.h"

static bool q_empty(queue_t *q) {
    return q->begin == q->end;
}

static int q_next(queue_t *q, uint16_t pos) {
    return (pos + 1) % q->CAPACITY;
}

static bool q_full(queue_t *q) {
    return q_next(q, q->end) == q->begin;
}

uint8_t q_size(queue_t *q) {
    return (q->end + q->CAPACITY - q->begin) % q->CAPACITY;
}

void *q_get(queue_t *q) {
    if (q_empty(q)) return NULL;
    int retidx = q->begin;
    q->begin = q_next(q, q->begin);
    return q->LOC + retidx * q->SIZE;
}

void *q_put(queue_t *q, void *e) {
    while (q_full(q)) {
        tight_loop_contents();
    }
    memcpy(q->LOC + q->end * q->SIZE, e, q->SIZE);
    int retidx = q->end;
    q->end = q_next(q, q->end);
    return q->LOC + retidx * q->SIZE;
}

