//
// Copyright and stuff
//

#include "pio_usb.h"

const uint16_t USB_SYNC = 0b1001100110011010;  // KJKJKJKK

void pio_usb_rx_enable(PIO pio, uint sm, bool en) { // ???
    if (en)
        hw_set_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
    else
        hw_clear_bits(&pio->sm[sm].shiftctrl, PIO_SM0_SHIFTCTRL_AUTOPUSH_BITS);
}

void pio_usb_put16(PIO pio, uint sm, uint16_t data) {
    printf("Sending %d\n", data);
    pio_sm_put_blocking(pio, sm, data << 16);
    //*(io_rw_16 *) &pio->txf[sm] = data;
    /*for(int i = 0; i < 100; i++){
        printf("%d %d\n", pio_sm_get_pc(pio, sm) - 18, pio_sm_get_tx_fifo_level(pio, sm));
        sleep_us(250);
    }*/
}

uint16_t pio_usb_get16(PIO pio, uint sm) {
    /*for(int i = 0; i < 100; i++){
    printf("%d %d\n", pio_sm_get_pc(pio, sm) - 18, pio_sm_get_tx_fifo_level(pio, sm));
    sleep_us(250);
    }*/
    return (uint16_t) pio_sm_get_blocking(pio, sm);
}

int pio_usb_read_SE0_terminated_packet(PIO pio, uint sm, uint8_t *rxbuf) {
    //pio_usb_rx_enable(pio, sm, true); // ???

    // TODO: PIO SHOULD PUSH MSB->LSB!
    uint16_t data = 0;
    int len = 0;
    bit_t last = 0; // TODO: Check
    int count;

    while (true) {
        count = 8;
        printf("%d %d %d ", pio_sm_get_pc(pio, sm) - 15, pio_sm_get_rx_fifo_level(pio, sm), gpio_get(15));
        data = pio_usb_get16(pio, sm);
        printf("0x%x\n", data);

        if (data == 0) {
            //Clean SE0
            break;
        }
        while ((data & 3) == 0 && data != 0) {
            // Align last read correctly as it can be less than full byte
            data >>= 2;
            count--;
        }
        while (count > 0) {
            if ((data & 3) == 0b00) {
                break;
            }
            //Use D+ discard D-
            set_bit(rxbuf, len, nrzi(data & 1, last));
            last = data & 1;
            len++;
            data >>= 2;
            count--;
        }
    }

    assert((len & 7) == 0b000); // Data is transmitted in bytes
    return len;
}

void pio_usb_send_SE0_terminated_packet(PIO pio, uint sm, uint8_t *txbuf, uint len) {

    uint32_t data_array[100];
    int data_len = 0;
    // TODO: PIO SHOULD PULL MSB<-LSB!
    uint32_t data = 0;
    int count = 0;

    int succ_ones = 0; //?
    int BIT_STUFF_LIMIT = 6;
    bit_t bit;
    bit_t prev = 0;

    //pio_usb_put16(pio, sm, USB_SYNC);
    data = USB_SYNC;
    count = 8;

    for (int i = 0; i < len * 8; i++) {

        if (succ_ones == BIT_STUFF_LIMIT) {
            i--; // Repeat this loop
            bit = 0;
            prev = 0;
            succ_ones = 0;
        } else {
            bit = get_bit(txbuf, i);
            succ_ones = (bit * succ_ones) | bit; // Increment if 1, reset if 0
            printf("Bit: %d\tNRZI: %d\n", bit, nrzi(bit, prev));
            bit = nrzi(bit, prev);
            prev = bit;
        }

        data = (data << 2) | ((1 - bit) << 1) | bit;
        count++;

        if (count == 16) {
            count = 0;
            //pio_usb_put16(pio, sm, data);
            //pio_sm_put_blocking(pio, sm, data);
            data_array[data_len++] = data;
            data = 0;
            printf("\n");
        }
    }

    // Push remaining data along with SE0
    data <<= 2 * (16 - count);
    //pio_sm_put_blocking(pio, sm, data);
    //pio_usb_put16(pio, sm, data);
    data_array[data_len++] = data;

    for(int i =0;i<data_len;i++) {
        printf("Sending: 0x%x (%d)\n", data_array[i], pio_sm_get_tx_fifo_level(pio, sm));
        pio_sm_put_blocking(pio, sm, data_array[i]);
    }

    printf("\n");
}

