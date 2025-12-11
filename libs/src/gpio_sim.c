#include "cm4_gpio.h"
#include <stdio.h>

static int s_gpio_regs = 0;
static int pin_modes[40];
static int pin_values[40];

StatusCode gpio_init(void) {
  if (s_gpio_regs != 0) {
    return STATUS_CODE_ALREADY_INITIALIZED; // already initialized
  }

  s_gpio_regs = 1;
  printf("[SIM] gpio_init()\n");
  return STATUS_CODE_OK;
}

StatusCode gpio_set_mode(int pin, GpioMode mode) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  pin_modes[pin] = mode;
  printf("[SIM] Set pin %d mode = %s\n", pin, MODE_TO_STR(mode));
  return STATUS_CODE_OK;
}

StatusCode gpio_write(int pin, int value) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  pin_values[pin] = value;
  printf("[SIM] Pin %d -> %d\n", pin, value);
  return STATUS_CODE_OK;
}

StatusCode gpio_read(int pin, int *state) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] Read pin %d = %d\n", pin, pin_values[pin]);
  *state = pin_values[pin];
  return STATUS_CODE_OK;
}

StatusCode gpio_toggle(int pin) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  pin_values[pin] = !pin_values[pin];
  printf("[SIM] Pin %d -> %d\n", pin, pin_values[pin]);
  return STATUS_CODE_OK;
}

StatusCode gpio_set_edge(int pin, GpioEdge edge) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] Set pin %d edge %s\n", pin, EDGE_TO_STR(edge));
  return STATUS_CODE_OK;
}

StatusCode gpio_get_edge_event(int pin, int *event) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  *event = 1;

  printf("[SIM] Pin %d = %d\n", pin, *event);
  return STATUS_CODE_OK;
}

StatusCode gpio_clear_edge(int pin) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] Pin  %d edge cleared\n", pin);
  return STATUS_CODE_OK;
}