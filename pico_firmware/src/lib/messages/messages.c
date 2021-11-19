#include <sys/cdefs.h>
#include <hardware/structs/usb.h>
//
// Created by vlamir on 11/2/21.
//

#include "messages.h"

static const uint8_t *bugger;

static uint16_t flag;
static uint8_t dummy = 0xff;
void_func_t handler = NULL;
static uint64_t timeout_us = UINT64_MAX;

void set_spi_pin_handler(void_func_t fun) {
    handler = fun;
}

/// Retreive current flag
/// \returns Flag set by SPI transmission
uint16_t get_flag(void) {
    return flag;
}

/// Prints array in a hexadecimal format, one byte per row. For debugging.
///
/// \param data Array to be printed
/// \param len Array length
void print_arr_hex(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    if (len % 8 != 0) {
        printf("\n");
    }
}

static void gpio_irq(uint pin, uint32_t event) {
    switch (pin) {
        case GPIO_SLAVE_IRQ_PIN:
            assert(event == GPIO_IRQ_EDGE_RISE);
            assert(get_role() == SPI_ROLE_SLAVE);
            assert(handler != NULL);
            spi_send_blocking(NULL, 0, 0);
            handler();
            break;
        case GPIO_SLAVE_IDLE_PIN:
            assert(event == GPIO_IRQ_LEVEL_HIGH);
            assert(get_role() == SPI_ROLE_MASTER);
            assert(handler != NULL);
            handler();
        default:
            break;
    }
}

void set_idle(void) {
    assert(get_role() == SPI_ROLE_SLAVE);
    gpio_put(GPIO_SLAVE_IDLE_PIN, 1);
}

void clear_idle(void) {
    assert(get_role() == SPI_ROLE_SLAVE);
    gpio_put(GPIO_SLAVE_IDLE_PIN, 1);
}

bool slave_is_idle(void) {
    return gpio_get(GPIO_SLAVE_IDLE_PIN);
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
    gpio_init(GPIO_SLAVE_IDLE_PIN);
    gpio_set_dir(GPIO_SLAVE_IRQ_PIN, (get_role() == SPI_ROLE_SLAVE) ? GPIO_IN : GPIO_OUT);
    gpio_set_dir(GPIO_SLAVE_IDLE_PIN, (get_role() == SPI_ROLE_MASTER) ? GPIO_IN : GPIO_OUT);
    if (get_role() == SPI_ROLE_MASTER) {
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
        gpio_set_irq_enabled_with_callback(GPIO_SLAVE_IRQ_PIN, GPIO_IRQ_LEVEL_HIGH, true, gpio_irq);
    } else {
        gpio_put(GPIO_SLAVE_IDLE_PIN, 0);
        gpio_set_irq_enabled_with_callback(GPIO_SLAVE_IRQ_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_irq);
    }

    if (get_role() == SPI_ROLE_MASTER) {
        printf("--------\n MASTER \n--------\n");
    } else {
        printf("-------\n SLAVE \n-------\n");
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
    /*if (get_role() == SPI_ROLE_SLAVE) {
        printf("Setting irq pin high\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
    }*/
    uint8_t hdr[5] = {dummy, len >> 8, len, flag >> 8, flag};
    spi_write_blocking(spi_default, hdr, 5);
    if (get_role() == SPI_ROLE_MASTER) {
        busy_wait_ms(100);
    }
    spi_write_blocking(spi_default, data, len);
    if (flag & DEBUG_PRINT_AS_HEX) {
        printf("Sent data:\n");
        print_arr_hex(data, len);
    }
    if (flag & DEBUG_PRINT_AS_STRING) {
        printf((char *) data);
        printf("\n");
    }
    /*if (get_role() == SPI_ROLE_SLAVE) {
        printf("Setting irq pin low\n");
        gpio_put(GPIO_SLAVE_IRQ_PIN, 0);
    }*/
}

uint16_t spi_receive_blocking(uint8_t *data) {
    uint16_t len = 0;
    /*if (get_role() == SPI_ROLE_MASTER) {
        while (gpio_get(GPIO_SLAVE_IRQ_PIN) == 0) {
            tight_loop_contents();
        }
    } else {
        printf("Waiting for header\n");
    }*/
    uint64_t start_time = time_us_64();
    do {
        if (time_us_64() - start_time > timeout_us) {
            printf("%ld -> %ld | %ld\n", time_us_64(), start_time, timeout_us);
            return -1;
        }
        spi_read_blocking(spi_default, 0, &dummy, 1);
    } while (dummy != 0xff);
    uint8_t hdr[4];
    spi_read_blocking(spi_default, 0, hdr, 4);
    len = (hdr[0] << 8) + hdr[1];
    flag = (hdr[2] << 8) + hdr[3];
    for (int i = 0; i < 1000; i++) tight_loop_contents();
    spi_read_blocking(spi_default, 0, data, len & 0xFF);
    if (flag & DEBUG_PRINT_AS_HEX) {
        printf("Received length %d\n", len);
        printf("Received data:\n");
        print_arr_hex(data, len);
    }
    if (flag & DEBUG_PRINT_AS_STRING) {
        printf((char *) data);
        printf("\n");
    }
    return len;
}

void spi_send_string(char *data) {
    spi_send_blocking((uint8_t *) data, strlen(data) + 1, DEBUG_PRINT_AS_STRING);
}


int spi_await_with_timeout(uint8_t *data, uint16_t cond, uint64_t timeout_us_new) {
    int len;
    timeout_us = timeout_us_new;
    printf("TIMEOUT SET TO %ld\n", timeout_us);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    do {
        len = spi_receive_blocking(data);
        if (len == -1) return len;
    } while ((flag & cond) != cond);
#pragma clang diagnostic pop
    return len;
}

int spi_await(uint8_t *data, uint16_t cond) {
    return spi_await_with_timeout(data, cond, UINT64_MAX);
}