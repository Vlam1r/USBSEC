// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/17/21.
//

#include "debug.h"

static bool flags[PRINT_COUNT];

void debug_print(print_reason reason, char *format, ...) {
    va_list args;
    va_start(args, format);

    if (flags[reason])
        vprintf(format, args);

    va_end(args);
}

void set_print_flag(print_reason reason) {
    flags[reason] = true;
}

/// Prints array in a hexadecimal format, one byte per row. For debugging.
///
/// \param data Array to be printed
/// \param len Array length
void debug_print_array(print_reason reason, const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    if (len % 8 != 0) {
        printf("\n");
    }
}