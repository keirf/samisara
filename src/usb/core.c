/*
 * core.c
 * 
 * USB core.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

unsigned int usb_bulk_mps = USB_FS_MPS;

struct ep0 ep0;

const static struct usb_device_descriptor device_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200, /* USB 2.0 */
    .bMaxPacketSize0 = 64,
    .idVendor = 0x1209, /* pid.codes Open Source projects */
    .idProduct = 0x0001, /* Test Pid */
    .bcdDevice = 0x0100, /* 1.0 */
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerial = 3,
    .bNumConfigurations = 1
};

static char serial_string[32];
const static char * const string_descriptors[] = {
    "\x09\x04", /* LANGID: US English */
    "Keir Fraser",
    "Samisara",
    serial_string
};

void usb_init(void)
{
    snprintf(serial_string, sizeof(serial_string),
             "SS%08X%08X%08X", ser_id[0], ser_id[1], ser_id[2]);
    hw_usb_init();
}

void usb_deinit(void)
{
    hw_usb_deinit();
}

static unsigned int build_configuration_descriptor(uint8_t *dat)
{
    unsigned int sz = hid_build_configuration_descriptor(dat);
    ((struct usb_configuration_descriptor *)dat)->wTotalLength = sz;
    return sz;
}

static bool_t handle_control_request(void)
{
    struct usb_device_request *req = &ep0.req;
    bool_t handled = TRUE;

    if (ep0_data_out() && (req->wLength > sizeof(ep0.data))) {

        WARN("Ctl OUT too long: %u>%u\n", req->wLength, sizeof(ep0.data));
        handled = FALSE;

    } else if ((req->bmRequestType == 0x80)
               && (req->bRequest == USB_REQ_GET_STATUS)) {

        /* USB_REQ_GET_STATUS (Device) */
        ep0.data_len = 2;
        memset(ep0.data, 0, ep0.data_len);

    } else if ((req->bmRequestType == 0x80)
               && (req->bRequest == USB_REQ_GET_DESCRIPTOR)) {

        uint8_t type = req->wValue >> 8;
        uint8_t idx = req->wValue;
        if ((type == USB_DT_DEVICE) && (idx == 0)) {
            ep0.data_len = device_descriptor.bLength;
            memcpy(ep0.data, &device_descriptor, ep0.data_len);
        } else if ((type == USB_DT_DEVICE_QUALIFIER) && (idx == 0)) {
            /* No High Speed. */
            handled = FALSE;
        } else if ((type == USB_DT_CONFIGURATION) && (idx == 0)) {
            ep0.data_len = build_configuration_descriptor(ep0.data);
        } else if ((type == USB_DT_STRING) &&
                   (idx < ARRAY_SIZE(string_descriptors))) {
            const char *s = string_descriptors[idx];
            uint16_t *odat = (uint16_t *)ep0.data;
            int i = 0;
            if (idx == 0) {
                odat[i++] = 4+(USB_DT_STRING<<8);
                memcpy(&odat[i++], s, 2);
            } else {
                odat[i++] = (1+strlen(s))*2+(USB_DT_STRING<<8);
                while (*s)
                    odat[i++] = *s++;
            }
            ep0.data_len = i*2;
        } else {
            WARN("[Unknown device desc %u,%u]\n", type, idx);
            handled = FALSE;
        }

    } else if ((req->bmRequestType == 0x81)
               && (req->bRequest == USB_REQ_GET_DESCRIPTOR)) {

        handled = hid_get_descriptor();

    } else if ((req->bmRequestType == 0x00)
               && (req->bRequest == USB_REQ_SET_ADDRESS)) {

        usb_setaddr(req->wValue & 0x7f);

    } else if ((req->bmRequestType == 0x00)
              && (req->bRequest == USB_REQ_SET_CONFIGURATION)) {

        handled = hid_set_configuration();

    } else if ((req->bmRequestType&0x7f) == 0x21) {

        handled = hid_handle_class_request();

    } else {

        uint8_t *pkt = (uint8_t *)req;
        int i;
        WARN("(%02x %02x %02x %02x %02x %02x %02x %02x)",
               pkt[0], pkt[1], pkt[2], pkt[3],
               pkt[4], pkt[5], pkt[6], pkt[7]);
        if (ep0_data_out()) {
            WARN("[");
            for (i = 0; i < ep0.data_len; i++)
                WARN("%02x ", ep0.data[i]);
            WARN("]");
        }
        WARN("\n");
        handled = FALSE;

    }

    if (ep0_data_in() && (ep0.data_len > req->wLength))
        ep0.data_len = req->wLength;

    return handled;
}

static void usb_write_ep0(void)
{
    uint32_t len;

    if ((ep0.tx.todo < 0) || !ep_tx_ready(0))
        return;

    len = min_t(uint32_t, ep0.tx.todo, EP0_MPS);
    usb_write(0, ep0.tx.p, len);

    ep0.tx.p += len;
    ep0.tx.todo -= len;

    if (ep0.tx.todo == 0) {
        /* USB Spec 1.1, Section 5.5.3: Data stage of a control transfer is
         * complete when we have transferred the exact amount of data specified
         * during Setup *or* transferred a short/zero packet. */
        if (!ep0.tx.trunc || (len < EP0_MPS))
            ep0.tx.todo = -1;
    }
}

void handle_rx_ep0(bool_t is_setup)
{
    bool_t ready = FALSE;
    uint8_t ep = 0;

    if (is_setup) {

        /* Control Transfer: Setup Stage. */
        ep0.data_len = 0;
        ep0.tx.todo = -1;
        usb_read(ep, &ep0.req, sizeof(ep0.req));
        ready = ep0_data_in() || (ep0.req.wLength == 0);

    } else if (ep0.data_len < 0) {

        /* Unexpected Transaction */
        usb_stall(0);
        usb_read(ep, NULL, 0);

    } else if (ep0_data_out()) {

        /* OUT Control Transfer: Data from Host. */
        uint32_t len = ep_rx_ready(ep);
        int l = 0;
        if (ep0.data_len < sizeof(ep0.data))
            l = min_t(int, sizeof(ep0.data)-ep0.data_len, len);
        usb_read(ep, &ep0.data[ep0.data_len], l);
        ep0.data_len += len;
        if (ep0.data_len >= ep0.req.wLength) {
            ep0.data_len = ep0.req.wLength; /* clip */
            ready = TRUE;
        }

    } else {

        /* IN Control Transfer: Status from Host. */
        usb_read(ep, NULL, 0);
        ep0.tx.todo = -1;
        ep0.data_len = -1; /* Complete */

    }

    /* Are we ready to handle the Control Request? */
    if (!ready)
        return;

    /* Attempt to handle the Control Request: */
    if (!handle_control_request()) {

        /* Unhandled Control Transfer: STALL */
        usb_stall(0);
        ep0.data_len = -1; /* Complete */

    } else if (ep0_data_in()) {

        /* IN Control Transfer: Send Data to Host. */
        ep0.tx.p = ep0.data;
        ep0.tx.todo = ep0.data_len;
        ep0.tx.trunc = (ep0.data_len < ep0.req.wLength);
        usb_write_ep0();

    } else {

        /* OUT Control Transfer: Send Status to Host. */
        ep0.tx.p = NULL;
        ep0.tx.todo = 0;
        ep0.tx.trunc = FALSE;
        usb_write_ep0();
        ep0.data_len = -1; /* Complete */

    }
}

void handle_tx_ep0(void)
{
    usb_write_ep0();
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
