/*
 * main.c
 * 
 * System initialisation and navigation main loop.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

int EXC_reset(void) __attribute__((alias("main")));

volatile uint32_t reset_flag;
#define BOOTLOADER_START 0x1fffac00 /* AT32F415 */

static void canary_init(void)
{
    _irq_stackbottom[0] = _thread_stackbottom[0] = 0xdeadbeef;
}

static void canary_check(void)
{
    ASSERT(_irq_stackbottom[0] == 0xdeadbeef);
    ASSERT(_thread_stackbottom[0] == 0xdeadbeef);
}

static bool_t check_bootloader_requested(void)
{
    bool_t match = (reset_flag == 0xdeadbeef);
    reset_flag = 0;
    return match;
}

int main(void)
{
    if (check_bootloader_requested()) {
        /* Nope, so jump straight at the main firmware. */
        uint32_t sp = *(uint32_t *)BOOTLOADER_START;
        uint32_t pc = *(uint32_t *)(BOOTLOADER_START + 4);
        asm volatile (
            "mov sp,%0 ; blx %1"
            :: "r" (sp), "r" (pc));
    }

    /* Relocate DATA. Initialise BSS. */
    if (&_sdat[0] != &_ldat[0])
        memcpy(_sdat, _ldat, _edat-_sdat);
    memset(_sbss, 0, _ebss-_sbss);

    canary_init();
    stm32_init();
    time_init();
    console_init();
    console_crash_on_input();
    board_init();

    printk("\n** Samisara v%u.%u\n", fw_major, fw_minor);
    printk("** Keir Fraser <keir.xen@gmail.com>\n");
    printk("** https://github.com/keirf/samisara\n\n");

    keyboard_init();
    usb_init();

    for (;;) {
        canary_check();
        usb_process();
        keyboard_process();

#if 0
        /* XXX */
        usb_deinit();
        delay_ms(500);
        reset_flag = 0xdeadbeef;
        system_reset();
#endif
    }

    return 0;
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
