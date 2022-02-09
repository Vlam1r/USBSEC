// Copyright (c) 2021-2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/17/21.
//

#include "debug.h"
#include <pico/printf.h>
#include <pico/mutex.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"

/*
#define __IS_MASTER__
#define __DEBUG_LEVEL__ 1
*/


static bool flags[PRINT_REASON_COUNT];
static mutex_t serial_write_mtx;

void debug_print(print_reason reason, char *format, ...) {
#ifdef __IS_MASTER__
    if (!flags[reason]) return;
    mutex_enter_blocking(&serial_write_mtx);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    mutex_exit(&serial_write_mtx);
#endif
}

static void set_print_flag(print_reason reason) {
    flags[reason] = true;
}

/// Prints array in a hexadecimal format, one byte per row. For debugging.
///
/// \param reason What this data is
/// \param data Array to be printed
/// \param len Array length
void debug_print_array(print_reason reason, const uint8_t *data, uint32_t len) {
#ifdef __IS_MASTER__
    if (!flags[reason]) return;
    if (len == 0) {
        printf("NULL\n");
        return;
    }

    mutex_enter_blocking(&serial_write_mtx);
    for (int i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    if (len % 8 != 0) {
        printf("\n");
    }
    mutex_exit(&serial_write_mtx);
#endif
}

void init_debug_printing() {
#ifdef __IS_MASTER__
    mutex_init(&serial_write_mtx);
    stdio_uart_init();
    switch (__DEBUG_LEVEL__) {
        case 3:
            set_print_flag(PRINT_REASON_USB_EXCHANGES);
            set_print_flag(PRINT_REASON_SYNC);
            set_print_flag(PRINT_REASON_DCD_BUFFER);
            set_print_flag(PRINT_REASON_SPI_MESSAGES);
            set_print_flag(PRINT_REASON_CONTROLLER_RAW);
        case 2:
            set_print_flag(PRINT_REASON_IRQ);
            set_print_flag(PRINT_REASON_SLAVE_DATA);
            set_print_flag(PRINT_REASON_XFER_COMPLETE);
        case 1:
            set_print_flag(PRINT_REASON_SETUP_REACTION);
        case 0:
            set_print_flag(PRINT_REASON_PREAMBLE);
        default:
            break;
    }
#endif
}

void error(const char *fmt, ...) {
    gpio_put(25, 1);
#ifdef __IS_MASTER__
    puts("\n*** ERROR ***\n");
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
#endif
    panic("An unexpected error has occurred.");
}

#pragma clang diagnostic pop