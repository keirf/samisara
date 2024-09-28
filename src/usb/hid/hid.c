/*
 * hid.c
 * 
 * HID class handling.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

const static struct interface *interfaces[2] = {
    [0] = &interface_keyboard,
    [1] = &interface_vendor
};

bool_t hid_handle_class_request(void)
{
    const struct interface *intf = NULL;
    struct usb_device_request *req = &ep0.req;
    bool_t handled = FALSE;

    if (req->wIndex < ARRAY_SIZE(interfaces))
        intf = interfaces[req->wIndex];

    TRC("ClassReq if=%u/%s ", req->wIndex, intf ? intf->name : "Unknown");

    if (intf == NULL) {
        TRC("Bad interface\n");
        return FALSE;
    }

    TRC("req=%02x/%s_", req->bRequest,
        (req->bRequest & HID_REQ_SET) ? "Set" : "Get");

    switch (req->bRequest & ~HID_REQ_SET) {

    case HID_REQ_REPORT: {
        TRC("Report val=%04x len=%04x: ", req->wValue, req->wLength);
        if (intf->handle_report == NULL)
            goto no_handler;
        handled = intf->handle_report(req);
        break;
    }

    case HID_REQ_IDLE: {
        TRC("Idle val=%04x: ", req->wValue);
        if (intf->handle_idle == NULL)
            goto no_handler;
        handled = intf->handle_idle(req);
        break;
    }

    case HID_REQ_PROTOCOL: {
        TRC("Protocol val=%04x: ", req->wValue);
        if (intf->handle_protocol == NULL)
            goto no_handler;
        handled = intf->handle_idle(req);
        break;
    }

    default:
        TRC("???: Unknown req\n", req->bRequest);
        handled = FALSE;
        break;

    }

    return handled;

no_handler:
    TRC("No handler\n");
    return FALSE;
}

bool_t hid_get_descriptor(void)
{
    struct usb_device_request *req = &ep0.req;
    const struct interface *intf = NULL;
    uint8_t type = req->wValue >> 8;
    uint8_t idx = req->wValue;

    if (req->wIndex < ARRAY_SIZE(interfaces))
        intf = interfaces[req->wIndex];

    TRC("GetDescriptor if=%u/%s ", req->wIndex, intf ? intf->name : "Unknown");

    if (intf == NULL) {
        TRC("Bad interface\n");
        return FALSE;
    }

    TRC("type=%02x/", type);

    switch (type) {
    case HID_DT_REPORT:
        TRC("Report ");
        if (idx != 0) {
            TRC("Bad index %u\n", idx);
            return FALSE;
        }
        ep0.data_len = intf->report_descriptor_length;
        memcpy(ep0.data, intf->report_descriptor, ep0.data_len);
        TRC("%d bytes\n", ep0.data_len);
        break;
    default:
        TRC("Unknown type %02x\n", type);
        return FALSE;
    }

    return TRUE;
}

bool_t hid_set_configuration(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(interfaces); i++)
        interfaces[i]->initialise();

    usb_class_ops.configure();

    return TRUE;
}

const static struct usb_configuration_descriptor config_descriptor aligned(2) = {
    .bLength = sizeof(struct usb_configuration_descriptor),
    .bDescriptorType = USB_DT_CONFIGURATION,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .bmAttributes = 0xa0, /* Bus powered, Remote Wakeup */
    .bMaxPower = 50 /* 100mA */
};

unsigned int hid_build_configuration_descriptor(uint8_t *dat)
{
    uint8_t *p = dat;
    int i;

    memcpy(p, &config_descriptor, sizeof(config_descriptor));
    p += sizeof(config_descriptor);

    for (i = 0; i < ARRAY_SIZE(interfaces); i++)
        p += interfaces[i]->build_configuration_descriptor(p);

    return p - dat;
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
