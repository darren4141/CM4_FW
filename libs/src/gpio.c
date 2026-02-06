#include "cm4_gpio.h"

static volatile uint32_t *s_gpio_regs = NULL;

// static int request_edge_events(struct gpiod_line *line, GpioEdge edge,
// const char *consumer)
// {
// switch (edge) {
// case GPIO_EDGE_RISING:
// return gpiod_line_request_rising_edge_events(line, consumer);

// case GPIO_EDGE_FALLING:
// return gpiod_line_request_falling_edge_events(line, consumer);

// case GPIO_EDGE_BOTH:
// default:
// return gpiod_line_request_both_edges_events(line, consumer);
// }
// }

StatusCode gpio_get_regs_initialized()
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  return STATUS_CODE_OK;
}

StatusCode gpio_regs_init(void)
{
  if (s_gpio_regs != NULL) {
    return STATUS_CODE_ALREADY_INITIALIZED;
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

StatusCode gpio_set_mode(int pin, GpioMode mode)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  int reg_index = GPFSEL0_INDEX + (pin / 10);
  int shift = (pin % 10) * 3;

  uint32_t val = s_gpio_regs[reg_index];
  val &= ~(0b111 << shift);
  val |= (mode << shift);
  s_gpio_regs[reg_index] = val;

  return STATUS_CODE_OK;
}

StatusCode gpio_write(int pin, int value)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (value) {
    // high
    int reg_index = GPSET0_INDEX + (pin / 32);
    s_gpio_regs[reg_index] = (1U << (pin % 32));
  }
  else {
    // low
    int reg_index = GPCLR0_INDEX + (pin / 32);
    s_gpio_regs[reg_index] = (1U << (pin % 32));
  }

  return STATUS_CODE_OK;
}

StatusCode gpio_read(int pin, int *state)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  int reg_index = GPLEV0_INDEX + (pin / 32);
  uint32_t val = s_gpio_regs[reg_index];

  *state = (val & (1U << (pin % 32))) ? 1 : 0;

  return STATUS_CODE_OK;
}

StatusCode gpio_toggle(int pin)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  int state;
  StatusCode ret = gpio_read(pin, &state);
  if (ret != STATUS_CODE_OK) {
    printf("gpio_read failed with exit code: %u\n", ret);
    return ret;
  }

  if (state == 1) {
    ret = gpio_write(pin, 0);
  }
  else if (state == 0) {
    ret = gpio_write(pin, 1);
  }

  return ret;
}

// StatusCode gpio_event_init(GpioEvent *ge, uint8_t line_num, GpioEdge edge,
// const char *consumer)
// {

// if (!ge || !consumer) {
// return STATUS_CODE_INVALID_ARGS;
// }

// memset(ge, 0, sizeof(*ge));

// ge->line_num = line_num;
// ge->chip = gpiod_chip_open("/dev/gpiochip0");

// if (!ge->chip) {
// printf("gpiod failed to open chip\n");
// return STATUS_CODE_FAILED;
// }

// ge->line = gpiod_chip_get_line(ge->line, ge->line_num);

// if (!ge->line) {
// gpiod_chip_close(ge->chip);
// ge->chip = NULL;
// printf("gpiod failed to obtain line num\n");
// return STATUS_CODE_FAILED;
// }

// StatusCode ret = request_edge_events(ge->line, edge, consumer);

// if (ret < 0) {
// gpiod_chip_close(ge->chip);
// ge->chip = NULL;
// ge->line = NULL;
// printf("gpiod failed to request edge\n");
// return STATUS_CODE_FAILED;
// }

// ge->event_fd = gpiod_line_event_get_fd(ge->line);
// if (ge->event_fd < 0) {
// gpio_event_close(ge);
// printf("gpiod failed to get fd\n");
// return STATUS_CODE_FAILED;
// }

// return STATUS_CODE_OK;
// }

// StatusCode gpio_event_wait(GpioEvent *ge, int timeout_ms)
// {
// if (!ge || !ge->line || (ge->event_fd < 0)) {
// return STATUS_CODE_INVALID_ARGS;
// }
// struct pollfd pfd = {
// .fd = ge->event_fd,
// .events = POLLIN,
// };

// int rc = poll(&pfd, 1, timeout_ms);
// if (rc < 0) {
// printf("gpiod poll failed\n");
// return STATUS_CODE_FAILED;
// }
// if (rc == 0) {
// return STATUS_CODE_TIMEOUT;
// }
// if (pfd.revents & POLLIN) {
// return STATUS_CODE_OK;
// }

// return STATUS_CODE_OK;
// }

// StatusCode gpio_event_read(GpioEvent *ge, struct gpiod_line_event *out)
// {
// if (!ge || !ge->line || !out) {
// return STATUS_CODE_INVALID_ARGS;
// }
// if (gpiod_line_event_read(ge->line, out) < 0) {
// printf("gpiod line event read failed\n");
// return STATUS_CODE_FAILED;
// }

// return STATUS_CODE_OK;
// }

// StatusCode gpio_event_close(GpioEvent *ge)
// {
// if (!ge) {
// return STATUS_CODE_OK;
// }
// if (ge->chip) {
// gpiod_chip_close(ge->chip);
// }
// ge->chip = NULL;
// ge->line = NULL;
// ge->event_fd = -1;
// ge->line_num = 0;

// return STATUS_CODE_OK;
// }

StatusCode gpio_set_edge(int pin, GpioEdge edge)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  uint8_t bank;

  if (pin >= 32) {
    bank = 1;
  }
  else {
    bank = 0;
  }

  uint8_t bit = pin % 32;
  uint32_t mask = (1U << bit);

  uint8_t ren_index = GPREN0_INDEX + bank;
  uint8_t fen_index = GPFEN0_INDEX + bank;
  uint8_t eds_index = GPEDS0_INDEX + bank;

// clear pending event
  s_gpio_regs[eds_index] = mask;

  uint32_t ren = s_gpio_regs[ren_index];
  uint32_t fen = s_gpio_regs[fen_index];

// clear existing config

  if (edge == GPIO_EDGE_RISING) {
    ren |= mask;
    fen &= ~mask;
  }
  else if (edge == GPIO_EDGE_FALLING) {
    ren &= ~mask;
    fen |= mask;
  }
  else if (edge == GPIO_EDGE_BOTH) {
    ren |= mask;
    fen |= mask;
  }
  else {
// nothing to do
  }

  s_gpio_regs[ren_index] = ren;
  s_gpio_regs[fen_index] = fen;

  return STATUS_CODE_OK;
}

StatusCode gpio_get_edge_event(int pin, int *event)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (event == NULL) {
    return STATUS_CODE_INVALID_ARGS;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  uint8_t bank;

  if (pin >= 32) {
    bank = 1;
  }
  else {
    bank = 0;
  }

  uint8_t bit = pin % 32;
  uint32_t mask = (1U << bit);

  uint8_t eds_index = GPEDS0_INDEX + bank;

  uint32_t eds = s_gpio_regs[eds_index];

  if (eds & mask) {
    *event = 1;
  }
  else {
    *event = 0;
  }

  return STATUS_CODE_OK;
}

StatusCode gpio_clear_edge(int pin)
{
  if (!s_gpio_regs) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((pin < 0) || (pin > 53)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  uint8_t bank;

  if (pin >= 32) {
    bank = 1;
  }
  else {
    bank = 0;
  }

  uint8_t bit = pin % 32;
  uint32_t mask = (1U << bit);
  uint8_t eds_index = GPEDS0_INDEX + bank;

  s_gpio_regs[eds_index] = mask;

  return STATUS_CODE_OK;
}

/**
 * New interrupt architecture:
 *
 * use /dev/gpiochip0 for blocking interrupts/IRQs
 * still use /dev/gpiomem for normal GPIO
 *
 * Some function to "register" pins as INT, can probably turn gpio_set_mode into
 * a switch case If a pin is registered as interrupt, it may still allow reading
 * through gpiomem OR we could use gpiod_line_get_value() in the gpiochip
 * library instead
 *
 */