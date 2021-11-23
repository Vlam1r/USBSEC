// Copyright (c) 2021. Vladimir Viktor Mirjanic

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

    uint8_t hdr[5] = {dummy, len >> 8, len, flag >> 8, flag};
    spi_write_blocking(spi_default, hdr, 5);
#ifdef __IS_MASTER__
    busy_wait_ms(100);
#endif
    spi_write_blocking(spi_default, data, len);
}

/// Receive array via SPI. Slave/master agnostic
/// \param data Array to be sent
/// \param len Length to be sent
/// \param new_flag New flag to be set on both master and slave
///
uint16_t spi_receive_blocking(uint8_t *data) {
    uint16_t len = 0;

    do {
        spi_read_blocking(spi_default, 0, &dummy, 1);
    } while (dummy != 0xff);

    uint8_t hdr[4];
    spi_read_blocking(spi_default, 0, hdr, 4);
    len = (hdr[0] << 8) + hdr[1];
    flag = (hdr[2] << 8) + hdr[3];

    for (int i = 0; i < 1000; i++) tight_loop_contents();

    spi_read_blocking(spi_default, 0, data, len & 0xFF);

    return len;
}