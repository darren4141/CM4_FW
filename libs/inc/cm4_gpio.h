#pragma once
/* Standard library Headers */
#include <stdint.h>
#include <stdio.h>

/* Hardware library Headers*/
#include <fcntl.h>
#include <gpiod.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Inter-component Headers */
#include "global_enums.h"

#define GPIO_BLOCK_SIZE 4096

#define GPFSEL0_INDEX   0  // 0x00 / 4
#define GPSET0_INDEX    7  // 0x1C / 4
#define GPCLR0_INDEX    10 // 0x28 / 4
#define GPLEV0_INDEX    13 // 0x34 / 4

#define GPEDS0_INDEX    16 // 0x40 / 4
#define GPREN0_INDEX    19 // 0x4C / 4
#define GPFEN0_INDEX    22 // 0x58 / 4

typedef enum {
  GPIO_MODE_INPUT  = 0b000,
  GPIO_MODE_OUTPUT = 0b001,
  GPIO_MODE_ALT0   = 0b100,
  GPIO_MODE_ALT1   = 0b101,
  GPIO_MODE_ALT2   = 0b110,
  GPIO_MODE_ALT3   = 0b111,
  GPIO_MODE_ALT4   = 0b011,
  GPIO_MODE_ALT5   = 0b010
} GpioMode;

typedef enum {
  GPIO_EDGE_NONE = 0,
  GPIO_EDGE_RISING,
  GPIO_EDGE_FALLING,
  GPIO_EDGE_BOTH
} GpioEdge;

typedef struct {
  struct gpiod_chip *chip;
  struct gpiod_line *line;
  int event_fd;
  uint8_t line_num;
} GpioEvent;

#define MODE_TO_STR(x)                                                         \
        ((x) == GPIO_MODE_INPUT ? "INPUT"                                         \
         : (x) == GPIO_MODE_OUTPUT ? "OUTPUT"                                        \
         : (x) == GPIO_MODE_ALT0 ? "ALT0"                                          \
         : (x) == GPIO_MODE_ALT1 ? "ALT1"                                          \
         : (x) == GPIO_MODE_ALT2 ? "ALT2"                                          \
         : (x) == GPIO_MODE_ALT3 ? "ALT3"                                          \
         : (x) == GPIO_MODE_ALT4 ? "ALT4"                                          \
         : (x) == GPIO_MODE_ALT5 ? "ALT5"                                          \
         : "UNKNOWN")

#define EDGE_TO_STR(x)                                                         \
        ((x) == GPIO_EDGE_NONE ? "NONE"                                         \
         : (x) == GPIO_EDGE_RISING ? "RISING"                                       \
         : (x) == GPIO_EDGE_FALLING ? "FALLING"                                      \
         : (x) == GPIO_EDGE_BOTH ? "BOTH"                                         \
         : "UNKNOWN")

/**
 * Check if the gpio registers have been initialized, if not, call
 * gpio_regs_init
 */
StatusCode gpio_get_regs_initialized();

/**
 * Initialize the gpio registers by grabbing them from "/dev/gpiomem"
 */
StatusCode gpio_regs_init(void);

/**
 * Set the mode of a specified gpio pin
 */
StatusCode gpio_set_mode(int pin, GpioMode mode);

/**
 * Write to a specified gpio pin
 */
StatusCode gpio_write(int pin, int value);

/**
 * Read from a specified gpio pin
 */
StatusCode gpio_read(int pin, int *state);

/**
 * Toggle the state of a specified gpio pin
 */
StatusCode gpio_toggle(int pin);

// StatusCode gpio_event_init(GpioEvent *ge, uint8_t line_num, GpioEdge edge,
// const char *consumer);

///**
// * timeout_ms = -1 waits forever
// */
// StatusCode gpio_event_wait(GpioEvent *ge, int timeout_ms);

// StatusCode gpio_event_read(GpioEvent *ge, struct gpiod_line_event *out);

// StatusCode gpio_event_close(GpioEvent *ge);

/**
 * Set the edge action of a specified gpio pin
 */
StatusCode gpio_set_edge(int pin, GpioEdge edge);

/**
 * Get the edge state of a specified gpio pin, only works if the pin has been
 * configured with gpio_set_edge
 */
StatusCode gpio_get_edge_event(int pin, int *event);

/**
 * Clear the edge state of a specified gpio pin, only works if the pin has
   been
 * configured with gpio_set_edge
 */
StatusCode gpio_clear_edge(int pin);