//
// Created by vlamir on 11/2/21.
//

#include "usb_host.h"


bool hcd_setup_send(uint8_t rhport, uint8_t dev_addr, uint8_t const setup_packet[8])
{
    (void) rhport;

    // Copy data into setup packet buffer
    memcpy((void*)&usbh_dpram->setup_packet[0], setup_packet, 8);

    // Configure EP0 struct with setup info for the trans complete
    struct hw_endpoint *ep = _hw_endpoint_allocate(0);

    // EP0 out
    _hw_endpoint_init(ep, dev_addr, 0x00, ep->wMaxPacketSize, 0, 0);
    assert(ep->configured);

    ep->remaining_len = 8;
    ep->active        = true;

    // Set device address
    usb_hw->dev_addr_ctrl = dev_addr;

    uint32_t const flags = SIE_CTRL_BASE | USB_SIE_CTRL_SEND_SETUP_BITS | USB_SIE_CTRL_START_TRANS_BITS;

    usb_hw->sie_ctrl = flags;

    return true;
}
