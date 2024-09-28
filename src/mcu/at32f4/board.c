/*
 * at32f4/board.c
 * 
 * Board-specific setup and management.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#define gpio_led gpiob
#define pin_led 2 /* Keyboard LED */

const static struct board_config _board_config[] = {
    [0] = {
        .hse_mhz   = 8 },
};

const struct core_floppy_pins *core_floppy_pins;

/* Blink the activity LED to indicate fatal error. */
void early_fatal(int blinks)
{
    int i;
    rcc->apb2enr |= RCC_APB2ENR_IOPBEN;
    delay_ticks(10);
    gpio_configure_pin(gpio_led, pin_led, GPO_pushpull(IOSPD_LOW, HIGH));
    for (;;) {
        for (i = 0; i < blinks; i++) {
            gpio_write_pin(gpio_led, pin_led, LOW);
            early_delay_ms(150);
            gpio_write_pin(gpio_led, pin_led, HIGH);
            early_delay_ms(150);
        }
        early_delay_ms(2000);
    }
}

void identify_board_config(void)
{
    board_config = &_board_config[0];
}

static void mcu_board_init(void)
{
    /* N/A */
    gpio_pull_up_pins(gpioa, 0);
    gpio_pull_up_pins(gpiob, 0);
    gpio_pull_up_pins(gpioc, 0);
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
