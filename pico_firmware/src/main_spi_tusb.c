#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/usb_phy/usb_common.h"
#include "lib/usb_phy/usb_phy.h"
#include "lib/messages/messages.h"

#include "lib/rp2040/rp2040_usb.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

uint8_t data[1000];

typedef enum {WAIT_HOST_SETUP, WAIT_DEVICE_SETUP, WAIT_HOST, WAIT_DEVICE} state;

int main() {
    set_sys_clock_khz(144000, true);
    if(!is_master())
        stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(2500); //To establish console in VM

    messages_config();

    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    if(is_master()) {
        // master

        //spi_write_blocking(spi_default, data, 8);
        dcd_init_new(0);
        dcd_int_enable_new(0);

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        while(true);
    }
    else {
        // slave

        //hcd_init(0);
        //hcd_int_enable(0);

        while(true) {
            uint16_t hdr = spi_receive_blocking(data);
            //hcd_setup_send(0, 0, data);
            uint8_t arr[18] = {0x12, 0x01, 0x00, 0x01,0xff,0xff,0xff,0x40,
                               0x47,0x05,0x80,0x00,0x01,0x00,0x00,0x00,
                               0x00,0x01};

            spi_send_blocking(arr, 18, 2);

            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        }

    }

}

//Setup packet:
// 0x80 0x6 0x0 0x1 0x0 0x0 0x40 0x0

#pragma clang diagnostic pop