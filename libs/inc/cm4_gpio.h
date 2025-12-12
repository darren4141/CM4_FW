#pragma once

#include <stdint.h>

#include "global_enums.h"

#define GPIO_BLOCK_SIZE 4096

#define GPFSEL0_INDEX 0 // 0x00 / 4
#define GPSET0_INDEX 7  // 0x1C / 4
#define GPCLR0_INDEX 10 // 0x28 / 4
#define GPLEV0_INDEX 13 // 0x34 / 4

#define GPEDS0_INDEX 16 // 0x40 / 4
#define GPREN0_INDEX 19 // 0x4C / 4
#define GPFEN0_INDEX 22 // 0x58 / 4

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

typedef enum {
  GPIO_EDGE_NONE = 0,
  GPIO_EDGE_RISING = 1 << 0,
  GPIO_EDGE_FALLING = 1 << 1,
  GPIO_EDGE_BOTH = GPIO_EDGE_RISING | GPIO_EDGE_FALLING,
} GpioEdge;

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

#define EDGE_TO_STR(x)                                                         \
  ((x) == GPIO_EDGE_NONE      ? "NONE"                                         \
   : (x) == GPIO_EDGE_RISING  ? "RISING"                                       \
   : (x) == GPIO_EDGE_FALLING ? "FALLING"                                      \
   : (x) == GPIO_EDGE_BOTH    ? "BOTH"                                         \
                              : "UNKNOWN")

StatusCode gpio_get_regs_initialized();

StatusCode gpio_regs_init(void);

StatusCode gpio_set_mode(int pin, GpioMode mode);

StatusCode gpio_write(int pin, int value);

StatusCode gpio_read(int pin, int *state);

StatusCode gpio_toggle(int pin);

StatusCode gpio_set_edge(int pin, GpioEdge edge);

StatusCode gpio_get_edge_event(int pin, int *event);

StatusCode gpio_clear_edge(int pin);