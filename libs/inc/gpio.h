#pragma once

#include <stdint.h>

typedef enum
{
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
} GpioMode;

int gpio_init(void);

void gpio_set_mode(int pin, GpioMode mode);

void gpio_write(int pin, int value);

int gpio_read(int pin);