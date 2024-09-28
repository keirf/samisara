/*
 * hid_vendor.c
 * 
 * Vendor-specific HID class handling.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

static struct vdr_state {
    uint8_t idle;
} vdr_state, default_vdr_state = { 0 };

#define VDR_FEAT_REPORT_ID 0x01
#define VDR_FEAT_REPORT_SZ 48

static void vdr_initialise(void)
{
    vdr_state = default_vdr_state;
    usb_configure_ep(0x82, EPT_INTERRUPT, USB_FS_MPS);
}

static bool_t vdr_report_feature(struct usb_device_request *req)
{
    unsigned int idx = req->wValue & 255;
    int i;

    if (idx != VDR_FEAT_REPORT_ID) {
        TRC("bad report id %02x\n", idx);
        return FALSE;
    }

    if (req->wLength != (VDR_FEAT_REPORT_SZ+1)) {
        TRC("bad report size %u\n", req->wLength);
        return FALSE;
    }

    if (req->bRequest & HID_REQ_SET) {
        for (i = 0; i < req->wLength; i++)
            TRC("%02x ", ep0.data[i]);
    } else {
        ep0.data_len = VDR_FEAT_REPORT_SZ+1;
        ep0.data[0] = VDR_FEAT_REPORT_ID;
        for (i = 1; i < ep0.data_len; i++)
            ep0.data[i] = i;
    }
    
    TRC("\n");
    return TRUE;
}

static bool_t vdr_handle_report(struct usb_device_request *req)
{
    switch (req->wValue >> 8) {
    case HID_REPORT_TYPE_FEATURE:
        return vdr_report_feature(req);
    default: /* Unknown */
        TRC("bad report type %02x\n", req->wValue >> 8);
        break;
    }

    return FALSE;
}

static bool_t vdr_handle_idle(struct usb_device_request *req)
{
    if (req->bRequest & HID_REQ_SET) {
        vdr_state.idle = req->wValue >> 8;
    } else {
        ep0.data[0] = vdr_state.idle;
        ep0.data_len = 1;
    }

    TRC("%02x\n", vdr_state.idle);
    return TRUE;
}

const static struct usb_interface_descriptor vdr_interface_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_interface_descriptor),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bNumEndpoints = 1,
    .bInterfaceClass = 3, /* HID */
};

const static struct usb_endpoint_descriptor vdr_endpoint_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = 0x03, /* Interrupt */
    .wMaxPacketSize = 8,
    .bInterval = 255 /* max */
};

const static struct usb_hid_descriptor vdr_hid_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_hid_descriptor),
    .bDescriptorType = HID_DT,
    .bcdHID = 0x0110, /* 1.10 */
    .bNumDescriptors = 1,
    .bReportDescriptorType = HID_DT_REPORT,
    .wReportDescriptorLength = 23
};

const static uint8_t vdr_hid_report[] aligned(2) = {
    0x06, 0xc1, 0xff, /* Usage Page (Vendor FFC1) */
    0x09, 0x01, /* Usage (Vendor 0001) */
    0xa1, 0x01, /* Collection (Application) */
    0x09, 0xf0, /* Usage (Vendor) */
    0x85, VDR_FEAT_REPORT_ID, /* Report ID */
    0x15, 0x00, /* Logical Minimum (0) */
    0x26, 0xff,0x00, /* Logical Maximum (255) */
    0x75, 0x08, /* Report Size (8) */
    0x95, VDR_FEAT_REPORT_SZ, /* Report Count */
    0xb1, 0x02, /* Feature (Data, Array) */
    0xc0 /* End Collection */
};

unsigned int vdr_build_configuration_descriptor(uint8_t *dat)
{
    uint8_t *p = dat;
    memcpy(p, &vdr_interface_descriptor, sizeof(vdr_interface_descriptor));
    p += sizeof(vdr_interface_descriptor);
    memcpy(p, &vdr_hid_descriptor, sizeof(vdr_hid_descriptor));
    p += sizeof(vdr_hid_descriptor);
    memcpy(p, &vdr_endpoint_descriptor, sizeof(vdr_endpoint_descriptor));
    p += sizeof(vdr_endpoint_descriptor);
    return p - dat;
}

const struct interface interface_vendor = {
    .name = "Vendor",
    .report_descriptor = vdr_hid_report,
    .report_descriptor_length = sizeof(vdr_hid_report),
    .build_configuration_descriptor = vdr_build_configuration_descriptor,
    .initialise = vdr_initialise,
    .handle_report = vdr_handle_report,
    .handle_idle = vdr_handle_idle
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
