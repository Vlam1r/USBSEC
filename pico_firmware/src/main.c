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
    while (true) {
        tight_loop_contents();
        sync();
    }
}

void core0_loop(void) {
    while (true) {
        tight_loop_contents();
        switch (get_role()) {
            case SPI_ROLE_SLAVE:
                slavework();
                hcd_rp2040_irq_new();
                break;
            case SPI_ROLE_MASTER:
                handle_spi_slave_event();
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

    /*set_print_flag(PRINT_REASON_IRQ);
    set_print_flag(PRINT_REASON_USB_EXCHANGES);
    set_print_flag(PRINT_REASON_SPI_MESSAGES);
    set_print_flag(PRINT_REASON_XFER_COMPLETE);
    set_print_flag(PRINT_REASON_SETUP_REACTION);
    set_print_flag(PRINT_REASON_SLAVE_DATA);*/

    messages_config();
    multicore_launch_core1(core1_loop);

    if (get_role() == SPI_ROLE_MASTER) {
        // Master is device
        dcd_init_new(0);
        dcd_int_enable_new(0);
    } else {
        // Slave is host
        hcd_init(0);
        hcd_int_enable(0);
    }

    core0_loop();
}

