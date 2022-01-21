// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/16/21.
//

#ifndef PICO_FIRMWARE_EDPT_REGISTRY_H
#define PICO_FIRMWARE_EDPT_REGISTRY_H

#include "pico/stdlib.h"
#include "tusb_common.h"
#include "tusb_types.h"

#define REGISTRY_MAX_SIZE 16

void insert_into_registry(const tusb_desc_endpoint_t *edpt);

const tusb_desc_endpoint_t *get_first_in_registry(void);

const tusb_desc_endpoint_t *get_next_in_registry(const tusb_desc_endpoint_t *edpt);

#endif //PICO_FIRMWARE_EDPT_REGISTRY_H
