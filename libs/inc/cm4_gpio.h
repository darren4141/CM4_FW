#pragma once

#include <stdint.h>

#include "global_enums.h"

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

#define MODE_TO_STR(x)                                                         \
  ((x) == GPIO_MODE_INPUT    ? "INPUT"                                         \
   : (x) == GPIO_MODE_OUTPUT ? "OUTPUT"                                        \
   : (x) == GPIO_MODE_ALT0   ? "ALT0"                                          \
   : (x) == GPIO_MODE_ALT1   ? "ALT1"                                          \
   : (x) == GPIO_MODE_ALT2   ? "ALT2"                                          \
   : (x) == GPIO_MODE_ALT3   ? "ALT3"                                          \
   : (x) == GPIO_MODE_ALT4   ? "ALT4"                                          \
   : (x) == GPIO_MODE_ALT5   ? "ALT5"                                          \
                             : "UNKNOWN")

StatusCode gpio_init(void);

StatusCode gpio_set_mode(int pin, GpioMode mode);

StatusCode gpio_write(int pin, int value);

StatusCode gpio_read(int pin, int *state);

StatusCode gpio_toggle(int pin);
