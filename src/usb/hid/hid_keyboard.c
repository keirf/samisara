/*
 * hid_keyboard.c
 * 
 * Keyboard HID class handling.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

static struct kbd_state {
    uint8_t led;
    uint8_t idle;
    uint8_t protocol;
} kbd_state, default_kbd_state = { 0, 0, 1 };

uint8_t kbd_led(void)
{
    return kbd_state.led;
}

static void kbd_initialise(void)
{
    kbd_state = default_kbd_state;
    usb_configure_ep(0x81, EPT_INTERRUPT, USB_FS_MPS);
}

static bool_t kbd_report_leds(struct usb_device_request *req)
{
    if (req->bRequest & HID_REQ_SET) {
        if (req->wLength >= 1)
            kbd_state.led = ep0.data[0];
    } else {
        ep0.data[0] = kbd_state.led;
        ep0.data_len = 1;
    }

    TRC("%02x\n", kbd_state.led);
    return TRUE;
}

static bool_t kbd_handle_report(struct usb_device_request *req)
{
    switch (req->wValue >> 8) {
    case HID_REPORT_TYPE_OUT:
        return kbd_report_leds(req);
    default: /* Unknown */
        TRC("bad report type %02x\n", req->wValue >> 8);
        break;
    }

    return FALSE;
}

static bool_t kbd_handle_idle(struct usb_device_request *req)
{
    if (req->bRequest & HID_REQ_SET) {
        kbd_state.idle = req->wValue >> 8;
    } else {
        ep0.data[0] = kbd_state.idle;
        ep0.data_len = 1;
    }

    TRC("%02x\n", kbd_state.idle);
    return TRUE;
}

static bool_t kbd_handle_protocol(struct usb_device_request *req)
{
    if (req->bRequest & HID_REQ_SET) {
        kbd_state.protocol = req->wValue;
    } else {
        ep0.data[0] = kbd_state.protocol;
        ep0.data_len = 1;
    }

    TRC("%02x\n", kbd_state.protocol);
    return TRUE;
}

const static struct usb_interface_descriptor kbd_interface_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_interface_descriptor),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = 3, /* HID */
    .bInterfaceSubClass = 1, /* Boot Interface */
    .bInterfaceProtocol = 1 /* Keyboard */
};

const static struct usb_endpoint_descriptor kbd_endpoint_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x81,
    .bmAttributes = 0x03, /* Interrupt */
    .wMaxPacketSize = 8,
    .bInterval = 10 /* 10ms */
};

const static struct usb_hid_descriptor kbd_hid_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_hid_descriptor),
    .bDescriptorType = HID_DT,
    .bcdHID = 0x0110, /* 1.10 */
    .bNumDescriptors = 1,
    .bReportDescriptorType = HID_DT_REPORT,
    .wReportDescriptorLength = 63
};

/* This is the bog standard 6KRO keyboard report, compatible with the
 * Keyboard Boot Protocol. It's straight out of the spec, and all basic
 * keyboards use this as is or with tiny tweaks. */
const static uint8_t kbd_hid_report[] aligned(2) = {
    0x05, 0x01, /* Usage Page (Generic Desktop) */
    0x09, 0x06, /* Usage (Keyboard) */
    0xa1, 0x01, /* Collection (Application) */
    0x05, 0x07, /* Usage Page (Keyboard/Keypad) */
    0x19, 0xe0, /* Usage Minimum (224) */
    0x29, 0xe7, /* Usage Maximum (231) */
    0x15, 0x00, /* Logical Minimum (0) */
    0x25, 0x01, /* Logical Maximum (1) */
    0x75, 0x01, /* Report Size (1) */
    0x95, 0x08, /* Report Count (8) */
    0x81, 0x02, /* Input (Data, Variable, Absolute) ; Modifier */
    0x95, 0x01, /* Report Count (1) */
    0x75, 0x08, /* Report Size (8) */
    0x81, 0x01, /* Input (Constant) ; Reserved bytes */
    0x95, 0x05, /* Report Count (5) */
    0x75, 0x01, /* Report Size (1) */
    0x05, 0x08, /* Usage Page (LEDs) */
    0x19, 0x01, /* Usage Minimum (1) */
    0x29, 0x05, /* Usage Maximum (5) */
    0x91, 0x02, /* Output (Data, Variable, Absolute) ; LED report */
    0x95, 0x01, /* Report Count (1) */
    0x75, 0x03, /* Report Size (3) */
    0x91, 0x01, /* Output (Constant) ; LED report padding */
    0x95, 0x06, /* Report Count (6) */
    0x75, 0x08, /* Report Size (8) */
    0x15, 0x00, /* Logical Minimum (0) */
    0x25, 0x65, /* Logical Maximum (101) */
    0x05, 0x07, /* Usage Page (Keyboard/Keypad) */
    0x19, 0x00, /* Usage Minimum (0) */
    0x29, 0x65, /* Usage Maximum (101) */
    0x81, 0x00, /* Input (Data, Array) ; Key arrays (6 bytes) */
    0xc0        /* End Collection */
};

unsigned int kbd_build_configuration_descriptor(uint8_t *dat)
{
    uint8_t *p = dat;
    memcpy(p, &kbd_interface_descriptor, sizeof(kbd_interface_descriptor));
    p += sizeof(kbd_interface_descriptor);
    memcpy(p, &kbd_hid_descriptor, sizeof(kbd_hid_descriptor));
    p += sizeof(kbd_hid_descriptor);
    memcpy(p, &kbd_endpoint_descriptor, sizeof(kbd_endpoint_descriptor));
    p += sizeof(kbd_endpoint_descriptor);
    return p - dat;
}

const struct interface interface_keyboard = {
    .name = "Keyboard",
    .report_descriptor = kbd_hid_report,
    .report_descriptor_length = sizeof(kbd_hid_report),
    .build_configuration_descriptor = kbd_build_configuration_descriptor,
    .initialise = kbd_initialise,
    .handle_report = kbd_handle_report,
    .handle_idle = kbd_handle_idle,
    .handle_protocol = kbd_handle_protocol
};

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
