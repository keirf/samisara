/*
 * defs.h
 * 
 * USB HID-specific definitions and private interfaces.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* HID Class requests */
#define HID_REQ_REPORT        0x01
#define HID_REQ_IDLE          0x02
#define HID_REQ_PROTOCOL      0x03
#define HID_REQ_SET           0x08

/* HID Descriptors */
#define HID_DT                0x21
#define HID_DT_REPORT         0x22
#define HID_DT_PHYSICAL       0x23

/* HID Report Types */
#define HID_REPORT_TYPE_IN       1
#define HID_REPORT_TYPE_OUT      2
#define HID_REPORT_TYPE_FEATURE  3

#define TRACE 1

#if TRACE
#define TRC printk
#else
static inline void TRC(const char *format, ...) { }
#endif

struct interface {
    const char *name;
    const uint8_t *report_descriptor;
    unsigned int report_descriptor_length;
    unsigned int (*build_configuration_descriptor)(uint8_t *);
    void (*initialise)(void);
    bool_t (*handle_report)(struct usb_device_request *);
    bool_t (*handle_idle)(struct usb_device_request *);
    bool_t (*handle_protocol)(struct usb_device_request *);
};

extern const struct interface interface_keyboard;
extern const struct interface interface_vendor;

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
