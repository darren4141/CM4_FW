#include "cm4_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include  <pthread.h>
#include <unistd.h>

#include "cm4_gpio.h"

static int i2c_fd_1 = -1;
static int i2c_fd_2 = -1;

static pthread_mutex_t s_i2c_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    dev = "/dev/i2c-20";
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    dev = "/dev/i2c-3";
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

  int timeout = 2;
  ioctl(*fdp, I2C_TIMEOUT, timeout);

  int retries = 1;
  ioctl(*fdp, I2C_RETRIES, retries);

  TRY(i2c_scan(i2c_bus));

  return STATUS_CODE_OK;
}

StatusCode i2c_deinit(I2cBus i2c_bus)
{
  int *fdp = NULL;

  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!fdp) {
    return STATUS_CODE_OK;
  }

  fdp = NULL;

  return STATUS_CODE_OK;
}

static int smbus_quick(int fd, uint8_t addr)
{
  struct i2c_smbus_ioctl_data args = {
    .read_write = I2C_SMBUS_WRITE,
    .command = 0,
    .size = I2C_SMBUS_QUICK,
    .data = NULL,
  };

  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    return -1;
  }
  return ioctl(fd, I2C_SMBUS, &args);
}

StatusCode i2c_scan(I2cBus i2c_bus)
{
  int *fdp = NULL;

  if (i2c_bus == I2C_BUS_1) {
    fdp = &i2c_fd_1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    fdp = &i2c_fd_2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (*fdp < 0) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
    if (smbus_quick(*fdp, addr) == 0) {
      printf("Found device at 0x%02X\n", addr);
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

  pthread_mutex_lock(&s_i2c_mutex);
  if (ioctl(*fdp, I2C_SLAVE, addr) < 0) {
    printf("ioctl failed\n");
    pthread_mutex_unlock(&s_i2c_mutex);
    return STATUS_CODE_FAILED;
  }
  pthread_mutex_unlock(&s_i2c_mutex);

  ssize_t w = write(*fdp, buf, len);
  if (w < 0) {
    fprintf(stderr, "I2C write to 0x%02X failed: %s (errno=%d)\n",
            addr, strerror(errno), errno);
    return STATUS_CODE_FAILED;
  }
  if ((uint32_t)w != len) {
    fprintf(stderr, "I2C write short: wrote %zd/%u to 0x%02X\n", w, len, addr);
    return STATUS_CODE_FAILED;
  }

  printf("I2C wrote to 0x%02X\n", addr);

  return STATUS_CODE_OK;
}

StatusCode i2c_write_byte(I2cBus i2c_bus, uint8_t addr, uint8_t data)
{
  uint8_t data_buf[1];
  data_buf[0] = data;
  StatusCode ret = i2c_write(i2c_bus, addr, data_buf, 1);
  return ret;
}

StatusCode i2c_write_then_read(I2cBus i2c_bus, uint8_t addr,
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
    printf("i2c not initialized, fdp: %d\n", *fdp);
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

  pthread_mutex_lock(&s_i2c_mutex);
  if (ioctl(*fdp, I2C_RDWR, &data) < 0) {
    printf("ioctl failed\n");
    pthread_mutex_unlock(&s_i2c_mutex);
    return STATUS_CODE_FAILED;
  }
  pthread_mutex_unlock(&s_i2c_mutex);

  return STATUS_CODE_OK;
}