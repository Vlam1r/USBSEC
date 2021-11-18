// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/17/21.
//

#ifndef PICO_FIRMWARE_DEBUG_H
#define PICO_FIRMWARE_DEBUG_H

#include <pico/printf.h>
#include <stdarg.h>

typedef enum {
    PRINT_REASON_SPI_MESSAGES_IN,
    PRINT_REASON_SPI_MESSAGES_OUT,
    PRINT_REASON_EVENT_QUEUE,
    PRINT_REASON_EVENT,
    PRINT_REASON_IRQ,
    PRINT_REASON_PREAMBLE,
    PRINT_REASON_USB_EXCHANGES,
    PRINT_REASON_DCD_BUFFER,
    PRINT_REASON_SETUP_REACTION,
    /**/
    PRINT_REASON_COUNT
} print_reason;

void debug_print(print_reason reason, char *format, ...);

void debug_print_array(print_reason reason, const uint8_t *data, uint32_t len);

void set_print_flag(print_reason reason);

#endif //PICO_FIRMWARE_DEBUG_H
