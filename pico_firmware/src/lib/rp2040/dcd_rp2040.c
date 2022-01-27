/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

//#include "tusb_option.h"

#include "pico.h"
#include "rp2040_usb.h"
#include "../debug/debug.h"

// Current implementation force vbus detection as always present, causing device think it is always plugged into host.
// Therefore it cannot detect disconnect event, mistaken it as suspend.
// Note: won't work if change to 0 (for now)
#define FORCE_VBUS_DETECT   1

/*------------------------------------------------------------------*/
/* Low level controller
 *------------------------------------------------------------------*/

#define usb_hw_set hw_set_alias(usb_hw)
#define usb_hw_clear hw_clear_alias(usb_hw)

// Init these in dcd_init
static uint8_t *next_buffer_ptr;

// USB_MAX_ENDPOINTS Endpoints, direction TUSB_DIR_OUT for out and TUSB_DIR_IN for in.
static struct hw_endpoint hw_endpoints[USB_MAX_ENDPOINTS][2];

static inline struct hw_endpoint *hw_endpoint_get_by_num(uint8_t num, tusb_dir_t dir) {
    return &hw_endpoints[num][dir];
}

static struct hw_endpoint *hw_endpoint_get_by_addr(uint8_t ep_addr) {
    uint8_t num = tu_edpt_number(ep_addr);
    tusb_dir_t dir = tu_edpt_dir(ep_addr);
    return hw_endpoint_get_by_num(num, dir);
}

static void _hw_endpoint_alloc(struct hw_endpoint *ep, uint8_t transfer_type) {
    // size must be multiple of 64
    uint16_t size = (ep->wMaxPacketSize + 63) / 64 * 64;

    // double buffered Bulk endpoint
    if (transfer_type == TUSB_XFER_BULK) {
        size *= 2u;
    }

    ep->hw_data_buf = next_buffer_ptr;
    next_buffer_ptr += size;

    assert(((uintptr_t) next_buffer_ptr & 0b111111u) == 0);
    uint dpram_offset = hw_data_offset(ep->hw_data_buf);
    assert(hw_data_offset(next_buffer_ptr) <= USB_DPRAM_MAX);

    // Fill in endpoint control register with buffer offset
    uint32_t const reg = EP_CTRL_ENABLE_BITS | (transfer_type << EP_CTRL_BUFFER_TYPE_LSB) | dpram_offset;

    *ep->endpoint_control = reg;
}

static void hw_endpoint_init(uint8_t ep_addr, uint16_t wMaxPacketSize, uint8_t transfer_type) {
    struct hw_endpoint *ep = hw_endpoint_get_by_addr(ep_addr);

    const uint8_t num = tu_edpt_number(ep_addr);
    const tusb_dir_t dir = tu_edpt_dir(ep_addr);

    ep->ep_addr = ep_addr;

    // For device, IN is a tx transfer and OUT is an rx transfer
    ep->rx = (dir == TUSB_DIR_OUT);

    ep->next_pid = 0u;
    ep->wMaxPacketSize = wMaxPacketSize;
    ep->transfer_type = transfer_type;

    // Every endpoint has a buffer control register in dpram
    if (dir == TUSB_DIR_IN) {
        ep->buffer_control = &usb_dpram->ep_buf_ctrl[num].in;
    } else {
        ep->buffer_control = &usb_dpram->ep_buf_ctrl[num].out;
    }

    // Clear existing buffer control state
    *ep->buffer_control = 0;

    if (num == 0) {
        // EP0 has no endpoint control register because the buffer offsets are fixed
        ep->endpoint_control = NULL;

        // Buffer offset is fixed (also double buffered)
        ep->hw_data_buf = (uint8_t *) &usb_dpram->ep0_buf_a[0];
    } else {
        // Set the endpoint control register (starts at EP1, hence num-1)
        if (dir == TUSB_DIR_IN) {
            ep->endpoint_control = &usb_dpram->ep_ctrl[num - 1].in;
        } else {
            ep->endpoint_control = &usb_dpram->ep_ctrl[num - 1].out;
        }

        // alloc a buffer and fill in endpoint control register
        _hw_endpoint_alloc(ep, transfer_type);
    }
}

static void hw_endpoint_xfer(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
    struct hw_endpoint *ep = hw_endpoint_get_by_addr(ep_addr);
    hw_endpoint_xfer_start(ep, buffer, total_bytes);
}

static void hw_handle_buff_status_bit(uint32_t remaining_buffers, uint i) {

    uint bit = 1u << i;

    // clear this in advance
    usb_hw_clear->buf_status = bit;

    // IN transfer for even i, OUT transfer for odd i
    struct hw_endpoint *ep = hw_endpoint_get_by_num(i >> 1u, !(i & 1u));

    // Continue xfer
    bool done = hw_endpoint_xfer_continue(ep);
    remaining_buffers &= ~bit;
    if (done) {
        // Notify
        dcd_event_xfer_complete_new(0, ep->ep_addr, ep->xferred_len, XFER_RESULT_SUCCESS, true);
        hw_endpoint_reset_transfer_new(ep);
    }
}

static void hw_handle_buff_status(void) {
    /*
     * We want to check buffers in this order: EP0 OUT, EP0 IN, forall x. EPx OUT, forall x. EPx IN
     * First because EP0 always has priority.
     * Second because out/in pairs are common, and we want to handle out before we request data in.
     */
    uint32_t remaining_buffers = usb_hw->buf_status;
    debug_print(PRINT_REASON_DCD_BUFFER, "Bugger: 0b%b\n", remaining_buffers);

    // EP0 OUT
    if (remaining_buffers & 0b10) hw_handle_buff_status_bit(remaining_buffers, 1);

    // EP0 IN
    if (remaining_buffers & 0b1) hw_handle_buff_status_bit(remaining_buffers, 0);

    // EPx OUT
    for (uint i = 3; remaining_buffers && i < USB_MAX_ENDPOINTS * 2; i += 2) {
        uint bit = 1u << i;
        if (remaining_buffers & bit) {
            hw_handle_buff_status_bit(remaining_buffers, i);

            remaining_buffers &= ~4u; // TODO Harden or is this needed
        }
    }

    // EPx IN
    for (uint i = 2; remaining_buffers && i < USB_MAX_ENDPOINTS * 2; i += 2) {
        uint bit = 1u << i;
        if (remaining_buffers & bit) {
            hw_handle_buff_status_bit(remaining_buffers, i);
        }
    }
}

static void reset_ep0_pid(void) {
    // If we have finished this transfer on EP0 set pid back to 1 for next
    // setup transfer. Also clear a stall in case
    uint8_t addrs[] = {0x0, 0x80};
    for (uint i = 0; i < TU_ARRAY_SIZE(addrs); i++) {
        struct hw_endpoint *ep = hw_endpoint_get_by_addr(addrs[i]);
        ep->next_pid = 1u;
    }
}

static void reset_non_control_endpoints(void) {
    // Disable all non-control
    for (uint8_t i = 0; i < USB_MAX_ENDPOINTS - 1; i++) {
        usb_dpram->ep_ctrl[i].in = 0;
        usb_dpram->ep_ctrl[i].out = 0;
    }

    // clear non-control hw endpoints
    tu_memclr(hw_endpoints[1], sizeof(hw_endpoints) - 2 * sizeof(hw_endpoint_t));
    next_buffer_ptr = &usb_dpram->epx_data[0];
}

void dcd_rp2040_irq_new(void) {
    uint32_t const status = usb_hw->ints;
    uint32_t handled = 0;

    if (status == 0) return;

    debug_print(PRINT_REASON_IRQ, "[IRQ] Master interrupt: %0#10lx\n", status);

    // SE0 for 2.5 us or more (will last at least 10ms)
    if (status & USB_INTS_BUS_RESET_BITS) {

        handled |= USB_INTS_BUS_RESET_BITS;

        usb_hw->dev_addr_ctrl = 0;
        reset_non_control_endpoints();
        device_event_bus_reset();
        usb_hw_clear->sie_status = USB_SIE_STATUS_BUS_RESET_BITS;
    }

    if (status & USB_INTS_SETUP_REQ_BITS) {
        handled |= USB_INTS_SETUP_REQ_BITS;
        uint8_t const *setup = (uint8_t const *) &usb_dpram->setup_packet;
        // reset pid to both 1 (data and ack)
        reset_ep0_pid();
        // Pass setup packet to tiny usb
        dcd_event_setup_received_new(0, setup, true);
        usb_hw_clear->sie_status = USB_SIE_STATUS_SETUP_REC_BITS;
    }

    if (status & USB_INTS_BUFF_STATUS_BITS) {
        handled |= USB_INTS_BUFF_STATUS_BITS;
        hw_handle_buff_status();
    }

    /* Note from pico datasheet 4.1.2.6.4 (v1.2)
     * If you enable the suspend interrupt, it is likely you will see a suspend interrupt when
     * the device is first connected but the bus is idle. The bus can be idle for a few ms before
     * the host begins sending start of frame packets. You will also see a suspend interrupt
     * when the device is disconnected if you do not have a VBUS detect circuit connected. This is
     * because without VBUS detection, it is impossible to tell the difference between
     * being disconnected and suspended.
     */
    if (status & USB_INTS_DEV_SUSPEND_BITS) {
        handled |= USB_INTS_DEV_SUSPEND_BITS;
        //dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
        usb_hw_clear->sie_status = USB_SIE_STATUS_SUSPENDED_BITS;
    }

    if (status & USB_INTS_DEV_RESUME_FROM_HOST_BITS) {
        handled |= USB_INTS_DEV_RESUME_FROM_HOST_BITS;
        //dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
        usb_hw_clear->sie_status = USB_SIE_STATUS_RESUME_BITS;
    }

    if (status ^ handled) {
        panic("Unhandled IRQ 0x%x\n", (uint) (status ^ handled));
    }
}

/*------------------------------------------------------------------*/
/* Controller API
 *------------------------------------------------------------------*/

// connect by enabling internal pull-up resistor on D+/D-
void dcd_connect(uint8_t rhport) {
    (void) rhport;
    usb_hw_set->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;
}

void dcd_init_new(uint8_t rhport) {
    assert(rhport == 0);

    // Reset hardware to default state
    rp2040_usb_init();

#if FORCE_VBUS_DETECT
    // Force VBUS detect so the device thinks it is plugged into a host
    usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS;
#endif

    // Init control endpoints
    tu_memclr(hw_endpoints[0], 2 * sizeof(hw_endpoint_t));
    hw_endpoint_init(0x0, 64, TUSB_XFER_CONTROL);
    hw_endpoint_init(0x80, 64, TUSB_XFER_CONTROL);

    // Init non-control endpoints
    reset_non_control_endpoints();

    // Initializes the USB peripheral for device mode and enables it.
    // Don't need to enable the pull up here. Force VBUS
    usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;

    // Enable individual controller IRQS here. Processor interrupt enable will be used
    // for the global interrupt enable...
    // Note: Force VBUS detect cause disconnection not detectable
    usb_hw->sie_ctrl = USB_SIE_CTRL_EP0_INT_1BUF_BITS;
    usb_hw->inte = USB_INTS_BUFF_STATUS_BITS | USB_INTS_BUS_RESET_BITS | USB_INTS_SETUP_REQ_BITS |
                   USB_INTS_DEV_SUSPEND_BITS | USB_INTS_DEV_RESUME_FROM_HOST_BITS |
                   (FORCE_VBUS_DETECT ? 0 : USB_INTS_DEV_CONN_DIS_BITS);

    dcd_connect(rhport);
}

// disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport) {
    (void) rhport;
    usb_hw_clear->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;
}

/*------------------------------------------------------------------*/
/* DCD Endpoint port
 *------------------------------------------------------------------*/

void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const *request) {
    (void) rhport;

    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
        request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD &&
        request->bRequest == TUSB_REQ_SET_ADDRESS) {
        usb_hw->dev_addr_ctrl = (uint8_t) request->wValue;
    }
}

bool dcd_edpt_open_new(uint8_t rhport, tusb_desc_endpoint_t const *desc_edpt) {
    assert(rhport == 0);
    hw_endpoint_init(desc_edpt->bEndpointAddress, desc_edpt->wMaxPacketSize.size, desc_edpt->bmAttributes.xfer);
    return true;
}

void dcd_edpt_close_all(uint8_t rhport) {
    (void) rhport;

    // may need to use EP Abort
    reset_non_control_endpoints();
}

bool dcd_edpt_xfer_new(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes) {
    assert(rhport == 0);
    if (ep_addr & 0x80) {
        debug_print(PRINT_REASON_USB_EXCHANGES, "Sending to host at 0x%x array of len %d:\n", ep_addr, total_bytes);
        debug_print_array(PRINT_REASON_USB_EXCHANGES, buffer, total_bytes);
    }
    hw_endpoint_xfer(ep_addr, buffer, total_bytes);
    return true;
}

void dcd_edpt_xfer_partial(uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes, uint16_t flag) {
    struct hw_endpoint *ep = hw_endpoint_get_by_addr(ep_addr);
    hw_endpoint_xfer_partial(ep, buffer, total_bytes, flag);
}

void dcd_edpt_stall_new(uint8_t rhport, uint8_t ep_addr) {
    (void) rhport;

    if (tu_edpt_number(ep_addr) == 0) {
        // A stall on EP0 has to be armed so it can be cleared on the next setup packet
        usb_hw_set->ep_stall_arm = (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) ? USB_EP_STALL_ARM_EP0_IN_BITS
                                                                         : USB_EP_STALL_ARM_EP0_OUT_BITS;
    }

    struct hw_endpoint *ep = hw_endpoint_get_by_addr(ep_addr);

    // stall and clear current pending buffer
    // may need to use EP_ABORT
    hw_endpoint_buffer_control_set_value32(ep, USB_BUF_CTRL_STALL);
}

void dcd_edpt_clear_stall_new(uint8_t rhport, uint8_t ep_addr) {
    (void) rhport;

    if (tu_edpt_number(ep_addr)) {
        struct hw_endpoint *ep = hw_endpoint_get_by_addr(ep_addr);

        // clear stall also reset toggle to DATA0, ready for next transfer
        ep->next_pid = 0;
        hw_endpoint_buffer_control_clear_mask32(ep, USB_BUF_CTRL_STALL);
    }
}

void dcd_int_handler_new(uint8_t rhport) {
    (void) rhport;
    dcd_rp2040_irq_new();
}
