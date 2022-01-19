// Copyright (c) 2021-2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/23/21.
//

#ifndef PICO_FIRMWARE_SPI_DATA_H
#define PICO_FIRMWARE_SPI_DATA_H

#include "pico.h"

#define SPI_BAUDRATE (int)(4*1000*1000) // 8MHz is too much at 144MHz clock

extern uint16_t flag;

void spi_send_blocking(const uint8_t *data, uint16_t len, uint16_t new_flag);

uint16_t spi_receive_blocking(uint8_t *data);

#endif //PICO_FIRMWARE_SPI_DATA_H
