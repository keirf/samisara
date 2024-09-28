/*
 * keyboard.c
 * 
 * A600/A1200 keyboard matrix scanning.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

static bool_t initialised;
static int count;

/* Rows 1-6: PA0-5 */
#define gpio_row gpioa

struct a1200_column {
    uint8_t code[6];
    uint8_t gpio;
    uint8_t pin;
};

#define NA 0xff

const static struct a1200_column a1200_matrix[15] = {
    { { 0x45, 0x00, 0x42, 0x62, 0x30, 0x5d }, _B, 15 },
    { { 0x5a, 0x01, 0x10, 0x20, 0x31, 0x5e }, _B, 14 },
    { { 0x50, 0x02, 0x11, 0x21, 0x32, 0x3f }, _B, 13 },
    { { 0x51, 0x03, 0x12, 0x22, 0x33, 0x2f }, _B, 12 },
    { { 0x52, 0x04, 0x13, 0x23, 0x34, 0x1f }, _B, 11 },
    { { 0x53, 0x05, 0x14, 0x24, 0x35, 0x3c }, _B, 10 },
    { { 0x54, 0x06, 0x15, 0x25, 0x36, 0x3e }, _B,  9 },
    { { 0x5b, 0x07, 0x16, 0x26, 0x37, 0x2e }, _B,  8 },
    { { 0x55, 0x08, 0x17, 0x27, 0x38, 0x1e }, _B,  7 },
    { { 0x5c, 0x09, 0x18, 0x28, 0x39, 0x43 }, _B,  6 },
    { { 0x56, 0x0a, 0x19, 0x29, 0x3a, 0x3d }, _B,  5 },
    { { 0x57, 0x0b, 0x1a, 0x2a, NA,   0x2d }, _B,  4 },
    { { 0x58, 0x0c, 0x1b, 0x2b, 0x40, 0x1d }, _B,  3 },
    { { 0x59, 0x0d, 0x44, 0x46, 0x41, 0x0f }, _B,  1 },
    { { 0x5f, 0x4c, 0x4f, 0x4e, 0x4d, 0x4a }, _B,  0 }
};

struct special_key {
    uint8_t code;
    uint8_t gpio;
    uint8_t pin;
};

const static struct special_key special_key[7] = {
    { 0x61, _A,  6 }, /* R.Shift */
    { 0x65, _A,  7 }, /* R.Alt */
    { 0x67, _A,  8 }, /* R.Amiga */
    { 0x63, _C, 13 }, /* Ctrl */
    { 0x60, _C, 15 }, /* L.Shift */
    { 0x64, _C, 14 }, /* L.Alt */
    { 0x66, _A, 15 }  /* L.Amiga */
};

const static uint8_t a1200_usb_map[] = {
    0x35, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, /* 00-07 */
    0x25, 0x26, 0x27, 0x2d, 0x2e, 0x31, NA,   0x62, /* 08-0f */
    0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, /* 10-17 */
    0x12, 0x13, 0x2f, 0x30, NA,   0x59, 0x5a, 0x5b, /* 18-1f */
    0x04, 0x16, 0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, /* 20-27 */
    0x0f, 0x33, 0x34, 0x31, NA,   0x5c, 0x5d, 0x5e, /* 28-2f */
    0x31, 0x1d, 0x1b, 0x06, 0x19, 0x05, 0x11, 0x10, /* 30-37 */
    0x36, 0x37, 0x38, NA,   0x63, 0x5f, 0x60, 0x61, /* 38-3f */
    0x2c, 0x2a, 0x2b, 0x58, 0x28, 0x29, 0x4c, NA,   /* 40-47 */
    NA,   NA,   0x56, NA,   0x52, 0x51, 0x4f, 0x50, /* 48-4f */
    0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, /* 50-57 */
    0x42, 0x43, 0x53, 0x47, 0x54, 0x55, 0x57, 0x49, /* 58-5f */
    0xe1, 0xe5, 0x39, 0xe0, 0xe2, 0xe6, 0xe3, 0xe7, /* 60-67 */
};

struct usb_report {
    uint8_t buf[8];
    unsigned int nr_codes;
};

static struct usb_report last_report;

void keyboard_init(void)
{
    int i;

    /* Rows */
    for (i = 0; i < 6; i++)
        gpio_configure_pin(gpio_row, i, GPI_floating);

    /* Special keys */
    for (i = 0; i < ARRAY_SIZE(special_key); i++) {
        const struct special_key *sk = &special_key[i];
        gpio_configure_pin(gpio_from_id(sk->gpio), sk->pin, GPI_floating);
    }

    /* Columns */
    for (i = 0; i < ARRAY_SIZE(a1200_matrix); i++) {
        const struct a1200_column *col = &a1200_matrix[i];
        gpio_configure_pin(gpio_from_id(col->gpio), col->pin,
                           GPO_opendrain(IOSPD_LOW, HIGH));
    }

    /* Caps Lock */
    gpio_configure_pin(gpiob, 2, GPO_pushpull(IOSPD_LOW, LOW));
}

static void report_init(struct usb_report *report)
{
    memset(report, 0, sizeof(*report));
}

static void report_add(struct usb_report *report, uint8_t code)
{
    if ((code >= 0xe0) && (code <= 0xe7)) {

        report->buf[0] |= 1u << (code & 7);

    } else if (report->nr_codes == 6) {

        /* Overflow: Phantom state */
        memset(&report->buf[2], 0x01, 6);

    } else {

        report->buf[2 + report->nr_codes++] = code;

    }
}

static void keyboard_scan(struct usb_report *report)
{
    int i, j;

    report_init(report);

    for (i = 0; i < ARRAY_SIZE(a1200_matrix); i++) {
        const struct a1200_column *col = &a1200_matrix[i];
        uint32_t idr;
        gpio_write_pin(gpio_from_id(col->gpio), col->pin, LOW);
        delay_us(20);
        idr = gpio_row->idr;
        for (j = 0; j < 6; j++) {
            if (!(idr & 1))
                report_add(report, a1200_usb_map[col->code[j]]);
            idr >>= 1;
        }
        gpio_write_pin(gpio_from_id(col->gpio), col->pin, HIGH);
    }

    for (i = 0; i < ARRAY_SIZE(special_key); i++) {
        const struct special_key *sk = &special_key[i];
        bool_t pressed = !gpio_read_pin(gpio_from_id(sk->gpio), sk->pin);
        if (pressed)
            report_add(report, a1200_usb_map[sk->code]);
    }
}

void keyboard_process(void)
{
    struct usb_report report;

    if (!initialised)
        return;

    gpio_write_pin(gpiob, 2, (kbd_led() & 2) ? HIGH : LOW);

    keyboard_scan(&report);

    if (initialised && ep_tx_ready(EP_TX)
        && memcmp(report.buf, last_report.buf, 8)) {
            usb_write(EP_TX, report.buf, 8);
            last_report = report;
    }
}

static void usb_hid_reset(void)
{
    initialised = FALSE;
    gpio_write_pin(gpiob, 2, LOW);
}

static void usb_hid_configure(void)
{
    memset(&last_report, 0xff, sizeof(last_report));
    initialised = TRUE;
    count = 0;
}

const struct usb_class_ops usb_class_ops = {
    .reset = usb_hid_reset,
    .configure = usb_hid_configure
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
