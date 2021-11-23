#include <stdio.h>
#include <hardware/vreg.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/messages/messages.h"
#include "lib/rp2040/rp2040_usb.h"
#include "lib/debug/debug.h"

uint8_t data[1000];

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main() {
    set_sys_clock_khz(144000, true);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Debug printing
    //
    set_print_flag(PRINT_REASON_PREAMBLE);
    set_print_flag(PRINT_REASON_IRQ);
    set_print_flag(PRINT_REASON_USB_EXCHANGES);
    set_print_flag(PRINT_REASON_DCD_BUFFER);
    set_print_flag(PRINT_REASON_SPI_MESSAGES);
    set_print_flag(PRINT_REASON_XFER_COMPLETE);
    set_print_flag(PRINT_REASON_SETUP_REACTION);
    set_print_flag(PRINT_REASON_SLAVE_DATA);

    messages_config();

    if (get_role() == SPI_ROLE_MASTER) {
        // Master is device
        dcd_init_new(0);
        dcd_int_enable_new(0);
    } else {
        // Slave is host
        hcd_init(0);
        hcd_int_enable(0);
    }

    while (true) {
        tight_loop_contents();
    }
}

#pragma clang diagnostic pop
