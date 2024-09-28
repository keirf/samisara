/*
 * usb.h
 * 
 * USB stack entry points and callbacks.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* Max Packet Size */
#define USB_FS_MPS 64
#define USB_HS_MPS 512
extern unsigned int usb_bulk_mps;

/* Class-specific callback hooks */
struct usb_class_ops {
    void (*reset)(void);
    void (*configure)(void);
};
extern const struct usb_class_ops usb_class_ops;

/* USB Endpoints for HID communications. */
#define EP_TX 1

/* Main entry points for USB processing. */
void usb_init(void);
void usb_deinit(void);
void usb_process(void);

/* Does OUT endpoint have data ready? If so return packet length, else -1. */
int ep_rx_ready(uint8_t ep);

/* Consume the next OUT packet, returning @len bytes. 
 * REQUIRES: ep_rx_ready(@ep) >= @len */
void usb_read(uint8_t ep, void *buf, uint32_t len);

/* Is IN endpoint ready for next packet? */
bool_t ep_tx_ready(uint8_t ep);

/* Queue the next IN packet, with the given payload data. 
 * REQUIRES: ep_tx_ready(@ep) == TRUE */
void usb_write(uint8_t ep, const void *buf, uint32_t len);

/* Is the USB enumerated at High Speed? */
bool_t usb_is_highspeed(void);

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
