#ifndef GPIO_H
#define GPIO_H

#include "types.h"

void gpio_init(void);
void gpio_set_pin(int pin);
void gpio_clear_pin(int pin);
int  gpio_get_pin(int pin);

#endif /* GPIO_H */
