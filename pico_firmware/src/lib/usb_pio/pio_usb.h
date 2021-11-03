//
// Copyright and stuff
//

#ifndef PIO_USB_H
#define PIO_USB_H

#include "usb.pio.h"
#include "../bit/bit.h"
#include  <stdio.h>

extern const uint16_t USB_SYNC;

// This both encodes and decodes NRZI
inline bit_t nrzi(bit_t a, bit_t b) {
    assert(is_bit(a));
    assert(is_bit(b));
    uint8_t c = 0b1 ^ a ^ b;
    assert(is_bit(c));
    return c;
}

//-------------------------------------------------

void pio_usb_rx_enable(PIO pio, uint sm, bool en);


void pio_usb_put16(PIO pio, uint sm, uint16_t data);
uint16_t pio_usb_get16(PIO pio, uint sm);

void pio_usb_send_SE0_terminated_packet(PIO pio, uint sm, uint8_t *txbuf, uint len);
int pio_usb_read_SE0_terminated_packet(PIO pio, uint sm, uint8_t *rxbuf);

#endif