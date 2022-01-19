// Copyright (c) 2021-2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/23/21.
//

#include "spi_data.h"

#include "hardware/spi.h"

/// Shared state on both microcontrollers
///
uint16_t flag;

/// Used to detect beginning of spi transmission.
///
static uint8_t dummy = 0xff;

/// Send array via SPI. Slave/master agnostic
/// \param data Array to be sent
/// \param len Length to be sent
/// \param new_flag New flag to be set on both master and slave
///
void spi_send_blocking(const uint8_t *data, uint16_t len, uint16_t new_flag) {
    flag = new_flag;
    uint8_t hdr[5] = {dummy, len, len >> 8, flag, flag >> 8};
    spi_write_blocking(spi_default, hdr, 5);
    spi_write_blocking(spi_default, data, len);
}

/// Receive array via SPI. Slave/master agnostic*
/// \param data Array to be sent
/// \return Length of transmission
///
uint16_t spi_receive_blocking(uint8_t *data) {
    uint8_t hdr[4];
    uint16_t *len = (uint16_t *) hdr;

    do {
        spi_read_blocking(spi_default, 0, &dummy, 1);
    } while (dummy != 0xff);

    spi_read_blocking(spi_default, 0, hdr, 4);
#if __IS_MASTER__
    /*
     * Assuming spi frequency of 4MHz...
     * Delay of 2us is too little, while 3us is ok. Going for 5us jus to be safe.
     * Possible explanation why delay is needed here:
     * Slave needs some time to set up next spi transmission,
     * so if we start reading too soon we get unexpected behaviour.
     */
    busy_wait_us(5);
#endif
    spi_read_blocking(spi_default, 0, data, *len);

    flag = *(uint16_t *) &hdr[2];
    return *len;
}