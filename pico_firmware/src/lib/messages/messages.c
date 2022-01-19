// Copyright (c) 2022. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/2/21.
//

#include "messages.h"

#include <hardware/structs/usb.h>
#include <pico/stdlib.h>
#include "pico/util/queue.h"
#include "../debug/debug.h"
#include "malloc.h"
#include "string.h"
#include "spi_data.h"
#include "hardware/spi.h"

queue_t tx, rx;
uint8_t bugger[1025];

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
    if (get_role() == SPI_ROLE_MASTER) {
        stdio_uart_init();
    }

    // Setup IO pins
    //
    init_gpio_pin(GPIO_SYNC, SPI_ROLE_SLAVE);
    init_gpio_pin(GPIO_SLAVE_WAITING_PIN, SPI_ROLE_MASTER);
    init_gpio_pin(GPIO_SLAVE_DEVICE_ATTACHED_PIN, SPI_ROLE_MASTER);
    //init_gpio_pin(GPIO_SYNC_DIR, SPI_ROLE_SLAVE);

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
    }

    // Init spi queues
    //
    queue_init(&tx, sizeof(spi_message_t), SPI_QUEUE_MAX_CAPACITY);
    queue_init(&rx, sizeof(spi_message_t), SPI_QUEUE_MAX_CAPACITY);
}

static void queue_add_with_copy(queue_t *q, spi_message_t *message) {
    uint8_t *payload_copy = malloc(message->payload_length);
    assert(payload_copy != NULL);
    memcpy(payload_copy, message->payload, message->payload_length);
    message->payload = payload_copy;
    queue_add_blocking(q, message);
}

/// Send a message to other microcontroller.
/// \param data Data buffer to be sent
/// \param len Amount of data to be sent
/// \param new_flag New flag on both microcontrollers
///
static void send_message(const uint8_t *data, uint16_t len, uint16_t new_flag) {

    // On master we have to prepare slave for data reception before sending
    // Master sets GPIO_SYNC_DIR high
    // Slave responds by setting GPIO_SLAVE_WAITING_PIN high
    //
    // On slave this isn't necessary as slave can't send over SPI if master doesn't read
    //
    if (get_role() == SPI_ROLE_MASTER) {
        //debug_print(PRINT_REASON_SPI_MESSAGES, "[SPI] Waiting for slave to get ready.\n");
        while (!gpio_get(GPIO_SLAVE_WAITING_PIN))
            tight_loop_contents();
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
static uint16_t recieve_message(uint8_t *data) {

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

void enqueue_spi_message(spi_message_t *message) {
    queue_add_with_copy(&tx, message);
}

bool dequeue_spi_message(spi_message_t *message) {
    return queue_try_remove(&rx, message);
}

void sync(void) {
    spi_message_t msg;
    int rec_count;
    gpio_put(6, 1);
    switch (get_role()) {
        case SPI_ROLE_MASTER:
            while (!queue_is_empty(&tx)) {
                queue_remove_blocking(&tx, &msg);
                debug_print(PRINT_REASON_SYNC, "[SYNC] Sending from TX queue with flag 0x%x\n", msg.e_flag);
                send_message(msg.payload, msg.payload_length, msg.e_flag);
            }
            gpio_put(6, 0);
            //debug_print(PRINT_REASON_SYNC, "[SYNC] Sending slave data query\n");
            send_message(NULL, 0, SLAVE_DATA_QUERY);
            assert(recieve_message(bugger) == 1);
            rec_count = bugger[0];
            while (rec_count--) {
                msg.payload = bugger;
                msg.payload_length = recieve_message(bugger);
                debug_print(PRINT_REASON_SYNC, "[SYNC] Received message from slave\n");
                debug_print_array(PRINT_REASON_SYNC, bugger, msg.payload_length);
                msg.e_flag = flag;
                queue_add_with_copy(&rx, &msg);
            }
            break;
        case SPI_ROLE_SLAVE:
            while (true) {
                msg.payload = bugger;
                msg.payload_length = recieve_message(bugger);
                msg.e_flag = flag;
                if (flag & SLAVE_DATA_QUERY) break;
                queue_add_with_copy(&rx, &msg);
            }
            gpio_put(6, 0);
            bugger[0] = queue_get_level_unsafe(&tx);
            rec_count = bugger[0];
            send_message(bugger, 1, 0);
            while (rec_count--) {
                queue_remove_blocking(&tx, &msg);
                spi_send_blocking(msg.payload, msg.payload_length, msg.e_flag);
            }
            break;
    }
}