#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/usb_phy/usb_common.h"
#include "lib/usb_phy/usb_phy.h"
#include "lib/messages/messages.h"

#include "lib/rp2040/rp2040_usb.h"

uint8_t data[1000];

int main() {
    set_sys_clock_khz(144000, true);

    if(!is_master()) {
        stdio_init_all();
    }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(2500); //To establish console in VM

    messages_config();

    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    if(is_master()) {
        // master
        sleep_ms(500);
        dcd_init_new(0);
        dcd_int_enable_new(0);

        //uint8_t setup[8] = {0x80, 0x6, 0x0, 0x1, 0x0, 0x0, 0x40, 0x0};
        //spi_send_blocking(setup, 8, USB_DATA | DEBUG_PRINT_AS_HEX);

        while(true) {
            uint8_t len = spi_receive_blocking(data);
            if(len > 0) {
                dcd_edpt_xfer_new(0, 0, data, len);
                // Send received USB data to host
                gpio_put(PICO_DEFAULT_LED_PIN,0);
            }
        }
    }
    else {
        // slave

        //hcd_init(0);
        //hcd_int_enable(0);

        //spi_receive_blocking(data);
        //define_setup_packet(data);

        //spi_send_string("Slave established");



        while(true) {
            uint8_t len = spi_receive_blocking(data);
            if(len > 0) {
                // Send received USB data to host
                gpio_put(PICO_DEFAULT_LED_PIN,0);
                uint8_t arr[18] = {0x12, 0x01, 0x10, 0x02, 0x00, 0x00, 0x00, 0x40,
                                   0x81, 0x07, 0x81, 0x55, 0x00, 0x01, 0x01, 0x02,
                                   0x03, 0x01};

                spi_send_blocking(arr, 18, USB_DATA | DEBUG_PRINT_AS_HEX);
            }
        }

    }

}
