#pragma once

#include <stdint.h>

typedef enum {
  GPIO_MODE_INPUT = 0b000,
  GPIO_MODE_OUTPUT = 0b001,
  GPIO_MODE_ALT0 = 0b100,
  GPIO_MODE_ALT1 = 0b101,
  GPIO_MODE_ALT2 = 0b110,
  GPIO_MODE_ALT3 = 0b111,
  GPIO_MODE_ALT4 = 0b011,
  GPIO_MODE_ALT5 = 0b010
} GpioMode;

int gpio_init(void);

void gpio_set_mode(int pin, GpioMode mode);

void gpio_write(int pin, int value);

int gpio_read(int pin);