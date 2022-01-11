#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "lib/usb_phy/usb_common.h"
#include "lib/usb_phy/usb_phy.h"
#include "lib/messages/messages.h"

#include "lib/rp2040/rp2040_usb.h"

int main() {
    set_sys_clock_khz(144000, true);

    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(2500); //To establish console in VM

    messages_config();
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    if(is_master()) {
        // master

        uint8_t arr[5] = {0,1,2,3,4};
        spi_send_blocking(arr, 5, 21);

        spi_receive_blocking(arr);
    }
    else {
        // slave

        spi_set_slave(spi_default, true);

        uint8_t arr[5] = {0,0,0,0,0};
        int hdr = spi_receive_blocking(arr);
        arr[0]=9;
        sleep_ms(1000);
        spi_send_blocking(arr, 3, 33);
    }

    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}
