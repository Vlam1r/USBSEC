// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 1/10/22.
//

#ifndef PICO_FIRMWARE_MASTERSLAVE_H
#define PICO_FIRMWARE_MASTERSLAVE_H

typedef enum {
    SPI_ROLE_MASTER,
    SPI_ROLE_SLAVE
} spi_role;

/// Checks whether microcontroller is master or slave
///
/// \return Spi role of the current microcontroller
///
static inline spi_role get_role(void) {
#ifdef __IS_MASTER__
    return SPI_ROLE_MASTER;
#else
    return SPI_ROLE_SLAVE;
#endif
}

#endif //PICO_FIRMWARE_MASTERSLAVE_H
