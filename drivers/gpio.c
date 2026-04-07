#include "gpio.h"
#include "esp32_regs.h"

void gpio_init(void)
{
    /* GPIO is already initialized by ROM bootloader for UART pins.
     * Additional GPIO initialization can be added here. */
}

void gpio_set_pin(int pin)
{
    GPIO_OUT_W1TS_REG = (1U << pin);
}

void gpio_clear_pin(int pin)
{
    GPIO_OUT_W1TC_REG = (1U << pin);
}

int gpio_get_pin(int pin)
{
    return (GPIO_IN_REG >> pin) & 1;
}
