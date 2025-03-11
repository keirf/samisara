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
    uint16_t subreport;
    uint16_t cmd_result;
} vdr_state, default_vdr_state = { 0 };

#define SAMISARA_VINTF_REPORT_ID 0x01
#define SAMISARA_VINTF_REPORT_SZ 48

static void vdr_initialise(void)
{
    vdr_state = default_vdr_state;
    usb_configure_ep(0x82, EPT_INTERRUPT, USB_FS_MPS);
}

static bool_t vdr_cmd(uint8_t *data)
{
    uint8_t cmd = data[0];
    uint8_t len = data[1];
    void *p = &data[2];
    int i;

    if ((len < 2) || (len > SAMISARA_VINTF_REPORT_SZ))
        goto bad_cmd;
    for (i = len; i < SAMISARA_VINTF_REPORT_SZ; i++)
        if (data[i]) goto bad_cmd;
    len -= 2;

    switch (cmd) {

    case SAMISARA_CMD_SUBREPORT: {
        struct samisara_cmd_subreport cmd_subreport;
        if (len != sizeof(cmd_subreport))
            goto bad_cmd;
        memcpy(&cmd_subreport, p, len);
        if (cmd_subreport.idx > SAMISARA_SUBREPORT_MAX)
            goto bad_cmd;
        vdr_state.subreport = cmd_subreport.idx;
        break;
    }

    case SAMISARA_CMD_DFU: {
        struct samisara_cmd_dfu cmd_dfu;
        if (len != sizeof(cmd_dfu))
            goto bad_cmd;
        memcpy(&cmd_dfu, p, len);
        if (cmd_dfu.deadbeef != 0xdeadbeef)
            goto bad_cmd;
        reset_to_bootloader();
        break;
    }

    default:
    bad_cmd:
        vdr_state.cmd_result = SAMISARA_RESULT_BAD_CMD;
        return TRUE;

    }

    vdr_state.cmd_result = SAMISARA_RESULT_OKAY;
    return TRUE;
}

static bool_t vdr_subreport(uint8_t *data)
{
    uint8_t len = 0;
    void *p = &data[2];

    switch (vdr_state.subreport) {

    case SAMISARA_SUBREPORT_INFO: {
        struct samisara_subreport_info info = {
            .max_cmd = SAMISARA_CMD_MAX,
            .max_subreport = SAMISARA_SUBREPORT_MAX,
            .cmd_result = vdr_state.cmd_result
        };
        len = sizeof(info);
        memcpy(p, &info, len);
        break;
    }

    case SAMISARA_SUBREPORT_BUILD_VER: {
        len = strlen(build_ver);
        memcpy(p, build_ver, len);
        break;
    }

    case SAMISARA_SUBREPORT_BUILD_DATE: {
        len = strlen(build_date);
        memcpy(p, build_date, len);
        break;
    }

    default:
        return FALSE;

    }

    data[0] = vdr_state.subreport;
    data[1] = len;
    memset(&data[2+len], 0, SAMISARA_VINTF_REPORT_SZ - len - 2);

    return TRUE;
}

static bool_t vdr_report_feature(struct usb_device_request *req)
{
    unsigned int idx = req->wValue & 255;
    bool_t result;
    int i;

    if (idx != SAMISARA_VINTF_REPORT_ID) {
        TRC("bad report id %02x\n", idx);
        return FALSE;
    }

    if (req->wLength != (SAMISARA_VINTF_REPORT_SZ+1)) {
        TRC("bad report size %u\n", req->wLength);
        return FALSE;
    }

    if (req->bRequest & HID_REQ_SET) {
        result = vdr_cmd(&ep0.data[1]);
    } else {
        ep0.data_len = SAMISARA_VINTF_REPORT_SZ+1;
        ep0.data[0] = SAMISARA_VINTF_REPORT_ID;
        result = vdr_subreport(&ep0.data[1]);
    }
    
    for (i = 0; i < req->wLength; i++)
        TRC("%02x ", ep0.data[i]);
    TRC(" -- %d\n", result);
    return result;
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
    0x85, SAMISARA_VINTF_REPORT_ID, /* Report ID */
    0x15, 0x00, /* Logical Minimum (0) */
    0x26, 0xff,0x00, /* Logical Maximum (255) */
    0x75, 0x08, /* Report Size (8) */
    0x95, SAMISARA_VINTF_REPORT_SZ, /* Report Count */
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
