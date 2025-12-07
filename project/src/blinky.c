#include "blinky.h"

#include <stdint.h>
#include <stdio.h>

#include "gpio.h"

static int state = 0;
static int led_pin = 17;

int blinky_init(void) {
  if (gpio_init() != 0) {
    fprintf(stderr, "Failed to init GPIO\n");
    return -1;
  }

  gpio_set_mode(led_pin, GPIO_MODE_OUTPUT);
  return 0;
}

int blinky_step(void) {
  gpio_write(led_pin, state);
  state = !state;
  return state;
}