#include "cm4_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cm4_gpio.h"

static volatile uint32_t *bsc1 = NULL;
static volatile uint32_t *bsc2 = NULL;

static int i2c_fd_1 = -1;
static int i2c_fd_2 = -1;

StatusCode i2c_get_initialized(I2cBus i2c_bus)
{

  int *fdp = NULL;
  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!fdp || (*fdp < 0)) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_init(I2cBus i2c_bus)
{

  StatusCode ret = gpio_get_regs_initialized();
  printf("Attempting to initialize i2c\n");

  if (ret != STATUS_CODE_OK) {
    printf("gpio regs are not initialized");
    return ret;
  }

  const char *dev = NULL;
  int *fdp = NULL;

  if (i2c_bus == I2C_BUS_1) {
    TRY(gpio_set_mode(2, GPIO_MODE_ALT0));
    TRY(gpio_set_mode(3, GPIO_MODE_ALT0));
    dev = "/dev/i2c-20";
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    TRY(gpio_set_mode(4, GPIO_MODE_ALT5));
    TRY(gpio_set_mode(5, GPIO_MODE_ALT5));
    dev = "/dev/i2c-21";
    fdp = &i2c_fd_2;
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (dev == NULL) {
    return STATUS_CODE_INVALID_ARGS;
  }

  int fd = open(dev, O_RDWR);
  if (fd < 0) {
    printf("Failed to access memory\n");
    fprintf(stderr, "open(%s) failed: %s (errno=%d)\n", dev, strerror(errno), errno);
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  *fdp = fd;

  TRY(i2c_scan(i2c_bus));

  return STATUS_CODE_OK;
}

StatusCode i2c_deinit(I2cBus i2c_bus)
{
  int *fdp = NULL;

  if (i2c_bus == I2C_BUS_1) {
    fdp = i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = i2c_fd_2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  fdp = NULL;

  return STATUS_CODE_OK;
}

StatusCode i2c_scan(I2cBus bus)
{
  int *fdp = NULL;
  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!fdp || (*fdp < 0)) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  printf("Scanning I2C bus %d\n", bus);

  for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
    // 0-byte write probe (address-only)
    struct i2c_msg msg = {
      .addr = addr,
      .flags = 0,        // write
      .len = 0,
      .buf = NULL,
    };

    struct i2c_rdwr_ioctl_data data = {
      .msgs = &msg,
      .nmsgs = 1,
    };

    int rc = ioctl(*fdp, I2C_RDWR, &data);
    if (rc == 1) {
      printf("  Found I2C device at 0x%02X\n", addr);
      continue;
    }
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf,
                     uint32_t len)
{
  int *fdp = NULL;
  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!fdp || (*fdp < 0)) {
    return STATUS_CODE_NOT_INITIALIZED;
  }
  if (!buf || (len == 0)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (ioctl(*fdp, I2C_SLAVE, addr) < 0) {
    printf("ioctl failed\n");
    return STATUS_CODE_FAILED;
  }

  ssize_t w = write(*fdp, buf, len);

  if (w != (ssize_t)len) {
    printf("i2c write length mismatch\n");
    return STATUS_CODE_FAILED;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_write_byte(I2cBus i2c_bus, uint8_t addr, uint8_t data)
{
  uint8_t data_buf[1];
  data_buf[0] = data;
  StatusCode ret = i2c_write(i2c_bus, addr, data_buf, 1);
  return ret;
}

StatusCode i2c_write_then_read(I2cBus bus, uint8_t addr,
                               const uint8_t *wbuf, uint32_t wlen,
                               uint8_t *rbuf, uint32_t rlen)
{
  int *fdp = NULL;

  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!fdp || (*fdp < 0)) {
    return STATUS_CODE_NOT_INITIALIZED;
  }
  if (!wbuf || !rbuf) {
    return STATUS_CODE_INVALID_ARGS;
  }

  struct i2c_msg msgs[2] = {
    {.addr = addr, .flags = 0, .len = (uint16_t)wlen, .buf = (uint8_t *)wbuf},
    {.addr = addr, .flags = I2C_M_RD, .len = (uint16_t)rlen, .buf = rbuf},
  };

  struct i2c_rdwr_ioctl_data data = {
    .msgs = msgs,
    .nmsgs = 2,
  };

  return (ioctl(*fdp, I2C_RDWR, &data) == 2) ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}