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
    //if(!is_master())
        stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(2500); //To establish console in VM

    messages_config();

    if(is_master()) {
        // master

        //dcd_init_new(0);
        //dcd_int_enable_new(0);

        //spi_write_blocking(spi_default, data, 8);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);

        uint8_t arr[5] = {0,1,2,3,4};
        spi_send_blocking(arr, 5, 21);

        int hdr = spi_receive_blocking(arr);

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        while(true);
    }
    else {
        // slave
        spi_set_slave(spi_default, true);

        uint8_t arr[5] = {0,0,0,0,0};
        int hdr = spi_receive_blocking(arr);
        arr[0]=9;
        sleep_ms(1000);
        spi_send_blocking(arr, 3, 33);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
/*
        hcd_init(0);
        hcd_int_enable(0);

        while(true) {
            uint16_t hdr = spi_receive_blocking(data);
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            hdr++;
            hcd_setup_send(0, 0, data);
            uint8_t arr[18] = {0x12, 0x01, 0x00, 0x01,0xff,0xff,0xff,0x40,
                               0x47,0x05,0x80,0x00,0x01,0x00,0x00,0x00,
                               0x00,0x01};

            spi_send_blocking(arr, 18, 2);

        }
        */
    }

}


#pragma clang diagnostic pop