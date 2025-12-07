#include "i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpio.h"

static volatile uint32_t *bsc1 = NULL;
static volatile uint32_t *bsc2 = NULL;

StatusCode i2c_init(I2cBus i2c_bus, uint32_t i2c_hz) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    return STATUS_CODE_MEM_ACCESS_FAILURE;
  }

  uint32_t *bsc = NULL;
  int phys_base;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
    phys_base = BSC1_BASE;
  } else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
    phys_base = BSC2_BASE;
  } else {
    perror("phys_base");
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc = (uint32_t *)mmap(NULL, BSC_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                         phys_base);
  close(fd);
  if (bsc == MAP_FAILED) {
    return -1;
  }

  if (i2c_bus == I2C_BUS_1) {
    gpio_set_mode(2, GPIO_ALT0);
    gpio_set_mode(3, GPIO_ALT0);
  } else if (i2c_bus == I2C_BUS_2) {
    gpio_set_mode(4, GPIO_ALT5);
    gpio_set_mode(5, GPIO_ALT5);
  } else {
    return STATUS_CODE_INVALID_ARGS;
  }

  uint32_t cdiv = CORE_FREQ_HZ / i2c_hz;

  if (cdiv < 2) {
    cdiv = 2;
  }

  bsc[BSC_DIV] = cdiv;

  return STATUS_CODE_OK;
}

StatusCode i2c_deinit(I2cBus i2c_bus) {

  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  } else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  } else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (bsc && bsc != MAP_FAILED) {
    munmap((void *)bsc, BSC_LEN);
    bsc = NULL;
  }

  return STATUS_CODE_OK;
}

int i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf, uint32_t len) {
  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  } else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  } else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc || bsc == MAP_FAILED || !addr || !buf || !len) {
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc[BSC_C] = C_I2CEN | C_CLEAR;
  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE; // clear sticky flags

  bsc[BSC_A] = addr;
  bsc[BSC_DLEN] = len;

  uint32_t idx = 0;
  while (idx < len && bsc[BSC_S] & S_TXD) {
    bsc[BSC_FIFO] = buf[idx];
    idx++;
  }

  bsc[BSC_C] = C_I2CEN | C_ST;

  while (!(bsc[BSC_S] & (S_DONE | S_ERR | S_CLKT))) {
    while (idx < len && bsc[BSC_S] & S_TXD) {
      bsc[BSC_FIFO] = buf[idx];
      idx++;
    }
  }

  int rc = 0;

  if (bsc[BSC_S] & S_ERR) {
    rc = -2; // NACK
    perror("i2c failure code -2")
  }
  if (bsc[BSC_S] & S_CLKT) {
    rc = -3; // clock stretch timeout
    perror("i2c failure code -3")
  }

  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE; // clear flags
  return rc == 0 ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}

StatusCode i2c_read(I2cBus i2c_bus, uint8_t addr, uint8_t *buf, uint32_t len) {
  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  } else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  } else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc || bsc == MAP_FAILED || !addr || !buf || !len) {
    return STATUS_CODE_INVALID_ARGS;
  }

  bsc[BSC_C] = C_I2CEN | C_CLEAR;
  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE;

  bsc[BSC_A] = addr;
  bsc[BSC_DLEN] = len;

  bsc[BSC_C] = C_I2CEN | C_ST | C_READ;

  uint32_t idx = 0;

  while (!(bsc[BSC_S] & (S_DONE | S_ERR | S_CLKT))) {
    while (idx < len && (bsc[BSC_S] & S_RXD)) {
      buf[idx] = bsc[BSC_FIFO] & 0xFF;
      idx++;
    }
  }

  while (idx < len && (bsc[BSC_S] & S_RXD)) {
    buf[idx] = bsc[BSC_FIFO] & 0xFF;
    idx++;
  }

  int rc = 0;

  if (bsc[BSC_S] & S_ERR) {
    rc = -2; // NACK
    perror("i2c failure code -2")
  }
  if (bsc[BSC_S] & S_CLKT) {
    rc = -3; // clock stretch timeout
    perror("i2c failure code -3")
  }

  bsc[BSC_S] = S_CLKT | S_ERR | S_DONE; // clear flags
  return rc == 0 ? STATUS_CODE_OK : STATUS_CODE_FAILED;
}
