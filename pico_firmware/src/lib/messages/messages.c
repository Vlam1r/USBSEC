#include <sys/cdefs.h>
//
// Created by vlamir on 11/2/21.
//

#include "messages.h"
#include "spi_data.h"

#include "hardware/spi.h"
#include "pico/binary_info.h"
#include <stdio.h>
#include "../debug/debug.h"
#include "../queueing/event_queue.h"

void_func_t handler = NULL;

void set_spi_pin_handler(void_func_t fun) {
    handler = fun;
}

/// Processes the GPIO interrupt
/// \param pin Pin which triggered interrupt event
/// \param event Event type
static void gpio_irq(uint pin, uint32_t event) {
    switch (pin) {
        case GPIO_SLAVE_IRQ_PIN:
            assert(event == GPIO_IRQ_EDGE_RISE);
            assert (get_role() == SPI_ROLE_MASTER);
            if (handler != NULL) {
                handler();
            }
            break;
        case GPIO_SLAVE_RECEIVE_PIN:
            assert(event == GPIO_IRQ_EDGE_RISE);
            assert (get_role() == SPI_ROLE_SLAVE);
            if (handler != NULL) {
                handler();
            }
            break;
        default:
            panic("Invalid GPIO Interrupt event.");
    }
}

/// Prepare a GPIO pint to be used as IO between microcontrollers
/// \param pin Pin to be used
/// \param in_role Which role will recieve input on the pin
///
static inline void init_gpio_pin(uint pin, int in_role) {
    gpio_init(pin);
    gpio_set_dir(pin, (get_role() == in_role) ? GPIO_IN : GPIO_OUT);
    if (get_role() != in_role) {
        gpio_put(pin, 0);
    }
}

/// Setup master/slave SPI
///
void messages_config(void) {

    // Setup debug IO over UART
    //
#ifdef __IS_MASTER__
    stdio_uart_init();
    assert(get_role() == SPI_ROLE_MASTER);
#endif

    // Setup master select
    //
    //gpio_init(GPIO_MASTER_SELECT_PIN);
    //gpio_set_dir(GPIO_MASTER_SELECT_PIN, GPIO_IN);

    // Setup IO pins
    //
    init_gpio_pin(GPIO_SLAVE_IRQ_PIN, SPI_ROLE_MASTER);
    init_gpio_pin(GPIO_SLAVE_WAITING_PIN, SPI_ROLE_MASTER);
    init_gpio_pin(GPIO_SLAVE_DEVICE_ATTACHED_PIN, SPI_ROLE_MASTER);
    init_gpio_pin(GPIO_SLAVE_RECEIVE_PIN, SPI_ROLE_SLAVE);

    // Setup GPIO events
    //
    if (get_role() == SPI_ROLE_MASTER) {
        gpio_set_irq_enabled_with_callback(GPIO_SLAVE_IRQ_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_irq);
    } else {
        gpio_set_irq_enabled_with_callback(GPIO_SLAVE_RECEIVE_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_irq);
    }

    // Setup SPI and its pins
    //
    spi_init(spi_default, SPI_BAUDRATE);
    spi_set_slave(spi_default, get_role() == SPI_ROLE_SLAVE);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

    // picotool binary information
    /*bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN,
                               PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI))*/

    // Debug printing
    //
    if (get_role() == SPI_ROLE_MASTER) {
        debug_print(PRINT_REASON_PREAMBLE, "\n--------\n MASTER \n--------\n");
    } else {
        debug_print(PRINT_REASON_PREAMBLE, "\n-------\n SLAVE \n-------\n");
    }
}

/// Checks whether microcontroller is master or slave
///
/// \return Spi role of the current microcontroller
///
spi_role get_role(void) {
    //return gpio_get(GPIO_MASTER_SELECT_PIN) ? SPI_ROLE_MASTER : SPI_ROLE_SLAVE;
#ifdef __IS_MASTER__
    assert(gpio_get(GPIO_MASTER_SELECT_PIN));
    return SPI_ROLE_MASTER;
#else
    assert(!gpio_get(GPIO_MASTER_SELECT_PIN));
    return SPI_ROLE_SLAVE;
#endif
}

/// Send a message to other microcontroller.
/// \param data Data buffer to be sent
/// \param len Amount of data to be sent
/// \param new_flag New flag on both microcontrollers
///
void send_message(const uint8_t *data, uint16_t len, uint16_t new_flag) {

    // On master we have to prepare slave for data reception before sending
    // Master sets GPIO_SLAVE_RECEIVE_PIN high
    // Slave responds by setting GPIO_SLAVE_WAITING_PIN high
    //
    // On slave this isn't necessary as slave can't send over SPI if master doesn't read
    //
    if (get_role() == SPI_ROLE_MASTER) {
        gpio_put(GPIO_SLAVE_RECEIVE_PIN, 1);
        //debug_print(PRINT_REASON_SPI_MESSAGES, "[SPI] Waiting for slave to get ready.\n");
        while (!gpio_get(GPIO_SLAVE_WAITING_PIN))
            tight_loop_contents();
        gpio_put(GPIO_SLAVE_RECEIVE_PIN, 0);
    }

    // Call role agnostic spi transmission
    //
    spi_send_blocking(data, len, new_flag);

    // Debug printing
    //
    if (new_flag & DEBUG_PRINT_AS_HEX) {
        debug_print(PRINT_REASON_SPI_MESSAGES,
                    "[SPI] Sent message of length %d, from %s to %s.\n",
                    len,
                    get_role() == SPI_ROLE_MASTER ? "MASTER" : "slave",
                    get_role() == SPI_ROLE_SLAVE ? "MASTER" : "slave");
        if (len > 0) {
            debug_print_array(PRINT_REASON_SPI_MESSAGES, data, len);
        }
    }
}


/// Receives message from other microcontroller.
/// \param data Data buffer to write message in
/// \return Length of transmission
///
uint16_t recieve_message(uint8_t *data) {

    if (get_role() == SPI_ROLE_SLAVE) {
        gpio_put(GPIO_SLAVE_WAITING_PIN, 1);
    }
    // Call role agnostic spi transmission
    //
    int len = spi_receive_blocking(data);

    if (get_role() == SPI_ROLE_SLAVE) {
        gpio_put(GPIO_SLAVE_WAITING_PIN, 0);
    }

    // Debug printing
    //
    if (flag & DEBUG_PRINT_AS_HEX) {
        debug_print(PRINT_REASON_SPI_MESSAGES,
                    "[SPI] Received message of length %d, from %s to %s.\n",
                    len,
                    get_role() == SPI_ROLE_SLAVE ? "MASTER" : "slave",
                    get_role() == SPI_ROLE_MASTER ? "MASTER" : "slave");
        if (len > 0) {
            debug_print_array(PRINT_REASON_SPI_MESSAGES, data, len);
        }
    }

    return len;
}

/// Retreive current flag.
/// \returns Message state shared between microcontrollers.
///
uint16_t get_flag(void) {
    return flag;
}

void send_event_to_master(uint8_t *bugger, uint16_t len, uint8_t ep_addr, uint16_t e_flag) {
    assert(get_role() == SPI_ROLE_SLAVE);
    event_t e = {
            .e_flag = e_flag,
            .payload = bugger,
            .payload_length = len,
            .ep_addr = ep_addr
    };
    create_event(&e);
    gpio_put(GPIO_SLAVE_IRQ_PIN, 1);
}
