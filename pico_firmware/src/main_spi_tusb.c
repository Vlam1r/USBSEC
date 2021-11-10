#include <stdio.h>
#include <string.h>
#include <hardware/vreg.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/messages/messages.h"
#include "lib/rp2040/rp2040_usb.h"

uint8_t data[1000];

int main() {
    set_sys_clock_khz(144000, true);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    messages_config();

    if (get_role() == SPI_ROLE_MASTER) {
        // master
        dcd_init_new(0);
        dcd_int_enable_new(0);

    } else {
        // slave
        hcd_init(0);
        hcd_int_enable(0);
        slavework();
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        /*
         * Nothing else to do.
         * Everything is event-driven.
         */
        tight_loop_contents();
    }
#pragma clang diagnostic pop
}
