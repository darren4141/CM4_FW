#include "gpio.h"
#include <stdio.h>

static int pin_modes[40];
static int pin_values[40];

int gpio_init(void)
{
    printf("[SIM] gpio_init()\n");
    return 0;
}

void gpio_set_mode(int pin, GpioMode mode)
{
    pin_modes[pin] = mode;
    printf("[SIM] Set pin %d mode = %s\n", pin,
           mode == GPIO_MODE_OUTPUT ? "OUTPUT" : "INPUT");
}

void gpio_write(int pin, int value)
{
    pin_values[pin] = value;
    printf("[SIM] Pin %d -> %d\n", pin, value);
}

int gpio_read(int pin)
{
    printf("[SIM] Read pin %d = %d\n", pin, pin_values[pin]);
    return pin_values[pin];
}