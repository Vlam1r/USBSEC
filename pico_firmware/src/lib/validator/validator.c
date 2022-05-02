// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 2/9/22.
//

#include "validator.h"
#include "../debug/debug.h"

void validate_driver_config() {

}

void check_comm(const tusb_desc_interface_t *const itf) {
    if (itf->bInterfaceClass == 0x03 /*HID*/ &&
        itf->bInterfaceProtocol == 0x01 /*Keyboard*/) {
        error("~~~~~~~~~~~~KEYBOARD BANNED~~~~~~~~~~~~");
    }
}
