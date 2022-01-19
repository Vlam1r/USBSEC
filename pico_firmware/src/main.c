// Copyright (c) 2022. Vladimir Viktor Mirjanic

#include <hardware/vreg.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "lib/messages/messages.h"
#include "lib/rp2040/rp2040_usb.h"
#include "lib/debug/debug.h"
#include "pico/multicore.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

void core1_loop(void) {
    gpio_init(6);
    gpio_set_dir(6, GPIO_OUT);
    gpio_init(4);
    gpio_set_dir(4, GPIO_OUT);
    while (true) {
        sync();
    }
}

void core0_loop(void) {
    gpio_init(5);
    gpio_set_dir(5, GPIO_OUT);
    while (true) {
        gpio_put(5, 0);
        switch (get_role()) {
            case SPI_ROLE_SLAVE:
                hcd_rp2040_irq_new();
                gpio_put(5, 1);
                slavework();
                break;
            case SPI_ROLE_MASTER:
                handle_spi_slave_event();
                gpio_put(5, 1);
                dcd_rp2040_irq_new();
                break;
        }
    }
}

#pragma clang diagnostic pop

int main() {
    set_sys_clock_khz(144000, true); // Slight overclock to a multiple of 12MHz (default 125MHz)

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Debug printing
    //
    set_print_flag(PRINT_REASON_PREAMBLE);
    set_print_flag(PRINT_REASON_DCD_BUFFER);

    set_print_flag(PRINT_REASON_IRQ);
    set_print_flag(PRINT_REASON_USB_EXCHANGES);
    //set_print_flag(PRINT_REASON_SPI_MESSAGES);
    set_print_flag(PRINT_REASON_XFER_COMPLETE);
    //set_print_flag(PRINT_REASON_SETUP_REACTION);
    set_print_flag(PRINT_REASON_SLAVE_DATA);
    set_print_flag(PRINT_REASON_SYNC);

    messages_config();

    if (get_role() == SPI_ROLE_MASTER) {
        // Master is device
        dcd_init_new(0);
    } else {
        // Slave is host
        hcd_init(0);
    }

    multicore_launch_core1(core1_loop);
    core0_loop();
}

