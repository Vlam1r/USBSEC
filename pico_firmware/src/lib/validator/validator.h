// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 2/9/22.
//

#ifndef PICO_FIRMWARE_VALIDATOR_H
#define PICO_FIRMWARE_VALIDATOR_H

#include "../drivers/drivers.h"

void validate_driver_config(void);

void check_comm(const tusb_desc_interface_t *itf);

#endif //PICO_FIRMWARE_VALIDATOR_H
