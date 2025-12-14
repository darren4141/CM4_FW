#include "cm4_i2c.h"

#include <stdio.h>

#include "cm4_gpio.h"

static uint32_t *bsc1 = NULL;
static uint32_t *bsc2 = NULL;

StatusCode i2c_get_initialized(I2cBus i2c_bus)
{
  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (!bsc) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  return STATUS_CODE_OK;
}

StatusCode i2c_init(I2cBus i2c_bus)
{
  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("phys_base");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (bsc != NULL) {
    return STATUS_CODE_ALREADY_INITIALIZED;
  }

  *bsc = 1U;

  if (i2c_bus == I2C_BUS_1) {
    gpio_set_mode(2, GPIO_MODE_ALT0);
    gpio_set_mode(3, GPIO_MODE_ALT0);
  }
  else if (i2c_bus == I2C_BUS_2) {
    gpio_set_mode(4, GPIO_MODE_ALT5);
    gpio_set_mode(5, GPIO_MODE_ALT5);
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] i2c_init() bus: %u\n", i2c_bus);

  TRY(i2c_scan(i2c_bus));

  return STATUS_CODE_OK;
}

StatusCode i2c_deinit(I2cBus i2c_bus)
{
  uint32_t *bsc = NULL;

  if (i2c_bus == I2C_BUS_1) {
    bsc = bsc1;
  }
  else if (i2c_bus == I2C_BUS_2) {
    bsc = bsc2;
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  if (bsc) {
    bsc = NULL;
  }

  printf("[SIM] i2c_deinit() bus: %u\n", i2c_bus);
  return STATUS_CODE_OK;
}

StatusCode i2c_scan(I2cBus i2c_bus)
{
  if (i2c_bus == I2C_BUS_1) {
    if (!bsc1) {
      return STATUS_CODE_NOT_INITIALIZED;
    }
  }
  else if (i2c_bus == I2C_BUS_2) {
    if (!bsc2) {
      return STATUS_CODE_NOT_INITIALIZED;
    }
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] i2c scan on bus %d\n", i2c_bus);

  return STATUS_CODE_OK;
}

StatusCode i2c_write(I2cBus i2c_bus, uint8_t addr, const uint8_t *buf,
                     uint32_t len)
{

  if (i2c_bus == I2C_BUS_1) {
    if (!bsc1) {
      return STATUS_CODE_NOT_INITIALIZED;
    }
  }
  else if (i2c_bus == I2C_BUS_2) {
    if (!bsc2) {
      return STATUS_CODE_NOT_INITIALIZED;
    }
  }
  else {
    perror("i2c_bus");
    return STATUS_CODE_INVALID_ARGS;
  }

  printf("[SIM] i2c_write(): ");

  for (uint32_t i = 0; i < len; i++) {
    printf("%u ", buf[i]);
  }
  printf("at address: %u on bus: %u\n", addr, i2c_bus);
  return STATUS_CODE_OK;
}

StatusCode i2c_write_byte(I2cBus i2c_bus, uint8_t addr, uint8_t data)
{
  uint8_t data_buf[1];
  data_buf[0] = data;
  StatusCode ret = i2c_write(i2c_bus, addr, data_buf, 1);
  return ret;
}
