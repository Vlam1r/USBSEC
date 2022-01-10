#ifndef RP2040_COMMON_H_
#define RP2040_COMMON_H_

#include "common/tusb_common.h"

#include "pico.h"
#include "hardware/structs/usb.h"
#include "hardware/irq.h"
#include "hardware/resets.h"
#include "../usb_event_handlers/usb_event_handlers.h"

#define pico_info(...)  TU_LOG(2, __VA_ARGS__)
#define pico_trace(...) TU_LOG(3, __VA_ARGS__)

// Hardware information per endpoint
/*typedef struct hw_endpoint
{
    // Is this a valid struct
    bool configured;
    
    // Transfer direction (i.e. IN is rx for host but tx for device)
    // allows us to common up transfer functions
    bool rx;
    
    uint8_t ep_addr;
    uint8_t next_pid;

    // Endpoint control register
    io_rw_32 *endpoint_control;

    // Buffer control register
    io_rw_32 *buffer_control;

    // Buffer pointer in usb dpram
    uint8_t *hw_data_buf;

    // Current transfer information
    bool active;
    uint16_t remaining_len;
    uint16_t xferred_len;

    // User buffer in main memory
    uint8_t *user_buf;

    // Data needed from EP descriptor
    uint16_t wMaxPacketSize;

    // Interrupt, bulk, etc
    uint8_t transfer_type;

    // Only needed for host
    uint8_t dev_addr;

    // If interrupt endpoint
    uint8_t interrupt_num;
} hw_endpoint_t;*/

void hw_endpoint_xfer_partial(struct hw_endpoint *ep, uint8_t *buffer, uint16_t total_len, uint16_t flag);

void rp2040_usb_init(void);

void hw_endpoint_xfer_start(struct hw_endpoint *ep, uint8_t *buffer, uint16_t total_len);

bool hw_endpoint_xfer_continue(struct hw_endpoint *ep);

void hw_endpoint_reset_transfer_new(struct hw_endpoint *ep);

void hw_endpoint_buffer_control_update32(struct hw_endpoint *ep, uint32_t and_mask, uint32_t or_mask);

static inline uint32_t hw_endpoint_buffer_control_get_value32(struct hw_endpoint *ep) {
    return *ep->buffer_control;
}

static inline void hw_endpoint_buffer_control_set_value32(struct hw_endpoint *ep, uint32_t value) {
    return hw_endpoint_buffer_control_update32(ep, 0, value);
}

static inline void hw_endpoint_buffer_control_set_mask32(struct hw_endpoint *ep, uint32_t value) {
    return hw_endpoint_buffer_control_update32(ep, ~value, value);
}

static inline void hw_endpoint_buffer_control_clear_mask32(struct hw_endpoint *ep, uint32_t value) {
    return hw_endpoint_buffer_control_update32(ep, ~value, 0);
}

static inline uintptr_t hw_data_offset(uint8_t *buf) {
    // Remove usb base from buffer pointer
    return (uintptr_t) buf ^ (uintptr_t) usb_dpram;
}

extern const char *ep_dir_string[];

struct hw_endpoint *get_dev_ep(uint8_t dev_addr, uint8_t ep_addr);

#endif
