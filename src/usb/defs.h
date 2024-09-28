/*
 * defs.h
 * 
 * USB standard definitions and private interfaces.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* bRequest: Standard Request Codes */
#define USB_REQ_GET_STATUS          0
#define USB_REQ_CLEAR_FEATURE       1
#define USB_REQ_SET_FEATURE         3
#define USB_REQ_SET_ADDRESS         5
#define USB_REQ_GET_DESCRIPTOR      6
#define USB_REQ_SET_DESCRIPTOR      7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE      10
#define USB_REQ_SET_INTERFACE      11
#define USB_REQ_SYNCH_FRAME        12

/* Descriptor Types */
#define USB_DT_DEVICE               1
#define USB_DT_CONFIGURATION        2
#define USB_DT_STRING               3
#define USB_DT_INTERFACE            4
#define USB_DT_ENDPOINT             5
#define USB_DT_DEVICE_QUALIFIER     6
#define USB_DT_OTHER_SPEED_CONFIGURATION 7
#define USB_DT_INTERFACE_POWER      8
#define USB_DT_OTG                  9
#define USB_DT_DEBUG               10
#define USB_DT_INTERFACE_ASSOCIATION 11

struct packed usb_device_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerial;
    uint8_t bNumConfigurations;
};

struct packed usb_configuration_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
};

struct packed usb_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
};

struct packed usb_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};

struct packed usb_hid_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bReportDescriptorType;
    uint16_t wReportDescriptorLength;
};

struct packed usb_device_request {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

#define EP0_MPS 64

extern struct ep0 {
    struct usb_device_request req;
    uint8_t data[128];
    int data_len;
    struct {
        const uint8_t *p;
        int todo;
        bool_t trunc;
    } tx;
} ep0;
#define ep0_data_out() (!(ep0.req.bmRequestType & 0x80))
#define ep0_data_in()  (!ep0_data_out())

/* USB HID */
bool_t hid_handle_class_request(void);
bool_t hid_get_descriptor(void);
bool_t hid_set_configuration(void);
unsigned int hid_build_configuration_descriptor(uint8_t *dat);

/* USB Core */
void handle_rx_ep0(bool_t is_setup);
void handle_tx_ep0(void);

/* USB Hardware */
enum { EPT_CONTROL=0, EPT_ISO, EPT_BULK, EPT_INTERRUPT, EPT_DBLBUF };
void usb_configure_ep(uint8_t ep, uint8_t type, uint32_t size);
void usb_stall(uint8_t ep);
void usb_setaddr(uint8_t addr);
void hw_usb_init(void);
void hw_usb_deinit(void);
bool_t hw_has_highspeed(void);

struct usb_driver {
    void (*init)(void);
    void (*deinit)(void);
    void (*process)(void);

    bool_t (*has_highspeed)(void);
    bool_t (*is_highspeed)(void);

    void (*setaddr)(uint8_t addr);

    void (*configure_ep)(uint8_t epnr, uint8_t type, uint32_t size);
    int (*ep_rx_ready)(uint8_t epnr);
    bool_t (*ep_tx_ready)(uint8_t epnr);
    void (*read)(uint8_t epnr, void *buf, uint32_t len);
    void (*write)(uint8_t epnr, const void *buf, uint32_t len);
    void (*stall)(uint8_t epnr);
};

extern const struct usb_driver dwc_otg;
extern const struct usb_driver usbd;

#define WARN printk

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
