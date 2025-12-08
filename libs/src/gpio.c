#include "cm4_gpio.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define GPIO_BLOCK_SIZE 4096

#define GPFSEL0_INDEX 0 // 0x00 / 4
#define GPSET0_INDEX 7  // 0x1C / 4
#define GPCLR0_INDEX 10 // 0x28 / 4
#define GPLEV0_INDEX 13 // 0x34 / 4

static volatile uint32_t *s_gpio_regs = NULL;

StatusCode gpio_init(void) {
  if (s_gpio_regs != NULL) {
    return STATUS_CODE_ALREADY_INITIALIZED; // already initialized
  }

  int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);

  if (fd < 0) {
    perror("open /dev/gpiomem");
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  s_gpio_regs = (uint32_t *)mmap(NULL, GPIO_BLOCK_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, 0);

  close(fd);

  if (s_gpio_regs == MAP_FAILED) {
    perror("mmap");
    s_gpio_regs = NULL;
    return STATUS_CODE_INVALID_ARGS;
  }

  return STATUS_CODE_OK;
}

StatusCode gpio_set_mode(int pin, GpioMode mode) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
  }

  int reg_index = GPFSEL0_INDEX + (pin / 10);
  int shift = (pin % 10) * 3;

  uint32_t val = s_gpio_regs[reg_index];
  val &= ~(0b111 << shift);
  val |= (mode << shift);
  s_gpio_regs[reg_index] = val;

  return STATUS_CODE_OK;
}

StatusCode gpio_write(int pin, int value) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
  }

  if (value) {
    // high
    int reg_index = GPSET0_INDEX + (pin / 32);
    s_gpio_regs[reg_index] = (1U << (pin % 32));
  } else {
    // low
    int reg_index = GPCLR0_INDEX + (pin / 32);
    s_gpio_regs[reg_index] = (1U << (pin % 32));
  }

  return STATUS_CODE_OK;
}

StatusCode gpio_read(int pin, int *state) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
  }

  int reg_index = GPLEV0_INDEX + (pin / 32);
  uint32_t val = s_gpio_regs[reg_index];

  *state = (val & (1U << (pin % 32))) ? 1 : 0;

  return STATUS_CODE_OK
}

StatusCode gpio_toggle(int pin) {
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (pin < 0 || pin > 53) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
  }

  int state;
  StatusCode ret = gpio_read(pin, &state);
  if (ret != STATUS_CODE_OK) {
    perror("gpio_read failed with exit code: %u\n", ret);
    return ret;
  }

  if (state == 1) {
    ret = gpio_write(pin, 0);
  } else if (state == 0) {
    ret = gpio_write(pin, 1);
  }

  return ret;
}