#include "currentsense.h"

#include <stdint.h>
#include <stdio.h>

#include "cm4_i2c.h"

static StatusCode ina_read_reg(uint8_t reg, uint16_t *val);

#define INA_WRITE_REG(reg, val)                                                \
  i2c_write(I2C_BUS_2, INA_I2C_ADDRESS,                                        \
            (uint8_t[]){reg, (val >> 8) & 0xFF, val & 0xFF}, 3);

#define INA_READ_REG(reg, val) ina_read_reg(reg, val);

static StatusCode ina_read_reg(uint8_t reg, uint16_t *val) {
  uint8_t read_buf[2] = {0, 0};
  StatusCode ret =
      i2c_write_then_read(I2C_BUS_1, INA_I2C_ADDRESS, &reg, 1, read_buf, 2);
  if (ret != STATUS_CODE_OK) {
    printf("Could not read from register: %d\n", reg);
    return ret;
  }

  *val = read_buf[1] << 8 | (read_buf[0] & 0xFF);
  return STATUS_CODE_OK;
}

StatusCode currentsense_init() {
  StatusCode ret = INA_WRITE_REG(INA_CALIBRATION, CAL_VALUE);

  if (ret != STATUS_CODE_OK) {
    printf("write to i2c failed\n");
    return ret;
  }

  INA_WRITE_REG(INA_CONFIGURATION, (CONFIG_BRNG | CONFIG_PG | CONFIG_BADC |
                                    CONFIG_SADC | CONFIG_MODE));
  return STATUS_CODE_OK;
}

StatusCode currentsense_read(float *current_val) {
  uint16_t current_reg;
  StatusCode ret = INA_READ_REG(INA_CURRENT, &current_reg);
  if (ret != STATUS_CODE_OK) {
    printf("read from i2c failed\n");
    return ret;
  }

  *current_val = (float)current_reg * CURRENT_LSB;
  return STATUS_CODE_OK;
}