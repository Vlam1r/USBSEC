//
// Copyright and stuff
//

#include <stdio.h>

#include "pico/stdlib.h"
//#include "lib/usb_phy/usb_common.h"

#include "lib/usb_pio/pio_usb.h"
//#include "lib/usb_phy/usb_phy.h"

const int PIN_USB_D_MINUS = 14;
const int PIN_USB_D_PLUS = 15;
const int USB_FULL_SPEED_BAUD = 12 * 1000 * 1000; // 12 Mbps

int main(void) {
    // Overclocking Pico to 144MHz
    // 144MHz = 12MHz * 4 * 3 so PIO clkdiv will be integer
    set_sys_clock_khz(144000, true);
    stdio_init_all();

    sleep_ms(3000);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    printf("USB Serial begin\n");

    //usb_device_init();

    // Wait until configured
    /*while (!configured) {
        tight_loop_contents();
    }*/

    // Get ready to rx from host
    //usb_start_transfer(usb_get_endpoint_configuration(EP1_OUT_ADDR), NULL, 64);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &usb_program);

    usb_program_init(pio, sm, offset, PIN_USB_D_MINUS, PIN_USB_D_PLUS, USB_FULL_SPEED_BAUD);

    uint8_t setup[3] = {0b10010110, 0b00011001, 0b00001100};
    uint8_t response[20];
    int len;

    printf("Start send setup packet\n");
    pio_usb_send_SE0_terminated_packet(pio, sm, setup, 3);

    printf("Start receive packet\n");
    len = pio_usb_read_SE0_terminated_packet(pio, sm, response);
    printf("Done!\n");
    // Everything is interrupt driven so just loop here
    while (true) {
        printf("%d", len);
        sleep_ms(1000);
        tight_loop_contents();
    }

}