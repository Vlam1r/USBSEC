//
// Created by vlamir on 10/27/21.
//

#ifndef PICO_FIRMWARE_BIT_H
#define PICO_FIRMWARE_BIT_H

#include "pico/stdlib.h"

typedef uint8_t bit_t;

inline bool is_bit(uint8_t b)
{
    return b == (b & 0b1);
}

inline uint8_t get_bit(const bit_t *a, int i)
{
    return (a[i >> 3] >> (0b111 ^ (i & 0b111)))& 1;
}

inline void set_bit(bit_t *a, int i, int b)
{
    assert(is_bit(b));
    a[i >> 3] = (a[i >> 3] & ~(1 << (0b111 ^ (i & 0b111)))) | (b << (0b111 ^ (i & 0b111)));
}

inline uint8_t reverse(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

#endif //PICO_FIRMWARE_BIT_H
