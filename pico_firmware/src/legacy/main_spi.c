#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/gpio.h"

#include "lib/usb_phy/usb_common.h"
#include "lib/usb_phy/usb_phy.h"
#include "lib/messages/messages.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#define BUF_LEN 0x100

uint8_t out_buf[BUF_LEN], in_buf[BUF_LEN];

typedef enum {WAIT_HOST_SETUP, WAIT_DEVICE_SETUP, WAIT_HOST, WAIT_DEVICE} state;

int main() {
    set_sys_clock_khz(144000, true);

    messages_config();

    // picotool binary information
    bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI));

    gpio_init(0);
    gpio_set_dir(0, GPIO_IN);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    out_buf[0] = 0;
    in_buf[0] = 0;


    if(gpio_get(0)) {
        // master

        usb_device_init();

        // Wait until configured
        while (!configured) {
            tight_loop_contents();
        }

        state s = WAIT_HOST_SETUP;

        // Get ready to rx from host
        //usb_start_transfer(usb_get_endpoint_configuration(EP1_OUT_ADDR), NULL, 64);

        while(true) {
            // Write the output buffer to MOSI, and at the same time read from MISO.

            switch(s) {
                case WAIT_HOST_SETUP:
                    break;
                case WAIT_DEVICE_SETUP:
                    break;
                case WAIT_HOST:
                    break;
                    case WAIT_DEVICE:
                        break;
            }

            spi_write_read_blocking(spi_default, out_buf, in_buf, 1);
            out_buf[0] = 1-out_buf[0];
            gpio_put(PICO_DEFAULT_LED_PIN, in_buf[0]);
            sleep_ms(1000);
        }
    }
    else {
        // slave
        spi_set_slave(spi_default, true);

        usb_host_init();

        while(true)  {
            // Write the output buffer to MISO, and at the same time read from MOSI.
            uint16_t hdr = spi_receive_blocking(in_buf);
            if(hdr & 0x0100) {
                //setup packet
                memcpy(usbh_dpram->setup_packet, in_buf, 8);
                //Send setup and receive data
                usb_hw->sie_ctrl = USB_SIE_CTRL_SEND_SETUP_BITS  | USB_SIE_CTRL_RECEIVE_DATA_BITS;
            }
        }
    }

}


#pragma clang diagnostic pop