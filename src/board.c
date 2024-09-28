/*
 * board.c
 * 
 * Board-specific setup and management.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

const struct board_config *board_config;

GPIO gpio_from_id(uint8_t id)
{
    switch (id) {
    case _A: return gpioa;
    case _B: return gpiob;
    case _C: return gpioc;
    case _D: return gpiod;
    case _E: return gpioe;
    case _F: return gpiof;
    case _G: return gpiog;
    }
    ASSERT(0);
    return NULL;
}

/* Pull up currently unused and possibly-floating pins. */
static void gpio_pull_up_pins(GPIO gpio, uint16_t mask)
{
    unsigned int i;
    for (i = 0; i < 16; i++) {
        if (mask & 1)
            gpio_configure_pin(gpio, i, GPI_pull_up);
        mask >>= 1;
    }
}

#if MCU == STM32F1
#include "mcu/stm32f1/board.c"
#elif MCU == AT32F4
#include "mcu/at32f4/board.c"
#endif

void board_init(void)
{
    mcu_board_init();

#ifdef NDEBUG
    /* Pull up unused debug pins (A9,A10 = serial console). */
    gpio_pull_up_pins(gpioa, (1u<<9) | (1u<<10));
#endif

    /* Activity LED is active low. */
    gpio_configure_pin(gpio_led, pin_led, GPO_pushpull(IOSPD_LOW, HIGH));
}

/* Set the activity LED status. */
void act_led(bool_t on)
{
    gpio_write_pin(gpio_led, pin_led, on ? LOW : HIGH);
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
