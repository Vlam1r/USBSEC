// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 3/7/22.
//

#include <hardware/gpio.h>
#include "direction_ctrl.h"

#define IN_PIN 22
#define OUT_PIN 21

bool inited = false;


void set_dir(bool in) {
    return;
    if (!inited) {
        gpio_init(IN_PIN);
        gpio_init(OUT_PIN);
        gpio_set_dir(IN_PIN, GPIO_OUT);
        gpio_set_dir(OUT_PIN, GPIO_OUT);
        inited = true;
    }
    gpio_put(IN_PIN, in ? 1 : 0);
    gpio_put(OUT_PIN, in ? 0 : 1);
}