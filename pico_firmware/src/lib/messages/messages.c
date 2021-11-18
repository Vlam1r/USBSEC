#include <sys/cdefs.h>
//
// Created by vlamir on 11/2/21.
//

#include "messages.h"

static uint16_t flag;
static uint8_t dummy = 0xff;
void_func_t handler = NULL;

void set_spi_pin_handler(void_func_t fun) {
    handler = fun;
}

/// Retreive current flag
/// \returns Flag set by SPI transmission
uint16_t get_flag(void) {
    return flag;
}


static void gpio_irq(uint pin, uint32_t event) {
    switch (pin) {
        case GPIO_SLAVE_IRQ_PIN:
            assert(event == GPIO_IRQ_EDGE_RISE);
            if (get_role() == SPI_ROLE_SLAVE) {
                if (handler != NULL) {
                    spi_send_blocking(NULL, 0, 0);
                    handler();
                }
            }
            break;
        default:
            break;
    }
}

void trigger_spi_irq(void) {
    assert(get_role() == SPI_ROLE_MASTER);
    gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    spi_receive_blocking(NULL);
    gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
}

/// Setup master/slave SPI
///
void messages_config(void) {
    gpio_init(GPIO_MASTER_SELECT_PIN);
    gpio_set_dir(GPIO_MASTER_SELECT_PIN, GPIO_IN);

    gpio_init(GPIO_SLAVE_IRQ_PIN);
    gpio_init(GPIO_SLAVE_WAITING_PIN);
    gpio_init(GPIO_SLAVE_DEVICE_ATTACHED_PIN);
    gpio_set_dir(GPIO_SLAVE_IRQ_PIN, (get_role() == SPI_ROLE_SLAVE) ? GPIO_IN : GPIO_OUT);
    gpio_set_dir(GPIO_SLAVE_WAITING_PIN, (get_role() == SPI_ROLE_MASTER) ? GPIO_IN : GPIO_OUT);
    gpio_set_dir(GPIO_SLAVE_DEVICE_ATTACHED_PIN, (get_role() == SPI_ROLE_MASTER) ? GPIO_IN : GPIO_OUT);
    if (get_role() == SPI_ROLE_MASTER) {
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
        gpio_set_irq_enabled_with_callback(GPIO_SLAVE_IRQ_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_irq);
    } else {
        gpio_put(GPIO_SLAVE_WAITING_PIN, 0);
    }

    if (get_role() == SPI_ROLE_MASTER) {
        debug_print(PRINT_REASON_PREAMBLE, "\n--------\n MASTER \n--------\n");
    } else {
        debug_print(PRINT_REASON_PREAMBLE, "\n-------\n SLAVE \n-------\n");
    }

    spi_init(spi_default, SPI_BAUDRATE);
    spi_set_slave(spi_default, get_role() == SPI_ROLE_SLAVE);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

    // picotool binary information
    bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN,
                               PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI))

}

/// Checks whether microcontroller is master or slave
///
/// \return True when Master, False when slave
spi_role get_role(void) {
    return gpio_get(GPIO_MASTER_SELECT_PIN) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
}

/// Send array via SPI. Slave/master agnostic
/// \param data Array to be sent
/// \param len Length to be sent
/// \param new_flag New flag to be set on both master and slave
void spi_send_blocking(const uint8_t *data, uint16_t len, uint16_t new_flag) {
    flag = new_flag;

    if (get_role() == SPI_ROLE_MASTER) {
        while (!gpio_get(GPIO_SLAVE_WAITING_PIN))
            tight_loop_contents();
    }

    uint8_t hdr[5] = {dummy, len >> 8, len, flag >> 8, flag};
    spi_write_blocking(spi_default, hdr, 5);
    if (get_role() == SPI_ROLE_MASTER) {
        busy_wait_ms(100);
    }
    spi_write_blocking(spi_default, data, len);
    if (flag & DEBUG_PRINT_AS_HEX) {
        debug_print(PRINT_REASON_SPI_MESSAGES_OUT, "Sent data:\n");
        debug_print_array(PRINT_REASON_SPI_MESSAGES_OUT, data, len);
    }
    if (flag & DEBUG_PRINT_AS_STRING) {
        debug_print(PRINT_REASON_SPI_MESSAGES_OUT, (char *) data);
        debug_print(PRINT_REASON_SPI_MESSAGES_OUT, "\n");
    }
}

uint16_t spi_receive_blocking(uint8_t *data) {
    uint16_t len = 0;

    do {
        spi_read_blocking(spi_default, 0, &dummy, 1);
    } while (dummy != 0xff);

    uint8_t hdr[4];
    spi_read_blocking(spi_default, 0, hdr, 4);
    len = (hdr[0] << 8) + hdr[1];
    flag = (hdr[2] << 8) + hdr[3];

    for (int i = 0; i < 1000; i++) tight_loop_contents();

    spi_read_blocking(spi_default, 0, data, len & 0xFF);

    if (flag & DEBUG_PRINT_AS_HEX) {
        debug_print(PRINT_REASON_SPI_MESSAGES_IN, "Received length %d\n", len);
        debug_print(PRINT_REASON_SPI_MESSAGES_IN, "Received data:\n");
        debug_print_array(PRINT_REASON_SPI_MESSAGES_IN, data, len);
    }
    if (flag & DEBUG_PRINT_AS_STRING) {
        debug_print(PRINT_REASON_SPI_MESSAGES_IN, (char *) data);
        debug_print(PRINT_REASON_SPI_MESSAGES_IN, "\n");
    }
    return len;
}

void spi_send_string(char *data) {
    spi_send_blocking((uint8_t *) data, strlen(data) + 1, DEBUG_PRINT_AS_STRING);
}


int spi_await(uint8_t *data, uint16_t cond) {
    int len;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    do {
        len = spi_receive_blocking(data);
        if (len == -1) return len;
    } while ((flag & cond) != cond);
#pragma clang diagnostic pop
    return len;
}