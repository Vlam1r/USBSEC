// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/17/21.
//

#ifndef PICO_FIRMWARE_DEBUG_H
#define PICO_FIRMWARE_DEBUG_H

#include <pico/printf.h>
#include <stdarg.h>

typedef enum {
    SPI_MESSAGES_IN,
    SPI_MESSAGES_OUT,
    EVENT_QUEUE,
    EVENT,
    IRQ,
    /**/
    PRINT_COUNT
} print_reason;

void debug_print(print_reason reason, char *format, ...);

void debug_print_array(print_reason reason, const uint8_t *data, int len);

void set_print_flag(print_reason reason);

#endif //PICO_FIRMWARE_DEBUG_H
