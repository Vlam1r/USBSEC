// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/10/22.
//

#include "spi_queue.h"

queue_t tx, rx;

void init_spi_queues(void) {
    queue_init(&tx, sizeof(spi_message_t), SPI_QUEUE_MAX_CAPACITY);
    queue_init(&rx, sizeof(spi_message_t), SPI_QUEUE_MAX_CAPACITY);
}

void enqueue_spi_message(spi_message_t *message) {
    queue_add_blocking(&tx, message);
}

void dequeue_spi_message(spi_message_t *message) {
    queue_remove_blocking(&rx, message);
}

void sync(void) {
    // TODO
}