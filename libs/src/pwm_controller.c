#include "pwm_controller.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "cm4_i2c.h"

#define PCA_WRITE_REG(reg, val)                                                \
  i2c_write(I2C_BUS_2, PCA_I2C_ADDR, (uint8_t[]){reg, val}, 2);

static bool isInitialized = false;

StatusCode pwm_controller_get_initialized() {
  if (isInitialized) {
    return STATUS_CODE_OK;
  } else {
    return STATUS_CODE_NOT_INITIALIZED;
  }
}

StatusCode pwm_controller_init(uint32_t pwm_freq) {
  if (pwm_freq < 24 || pwm_freq > 1526) {
    return STATUS_CODE_INVALID_ARGS;
  }

  StatusCode ret = i2c_get_initialized(I2C_BUS_2);
  if (ret != STATUS_CODE_OK) {
    printf("i2c bus: %u is not initialized\n", ret);
    return ret;
  }

  uint8_t mode1 = 0;

  ret = i2c_write_then_read(I2C_BUS_2, PCA_I2C_ADDR, (uint8_t[]){PCA_MODE1}, 1,
                            &mode1, 1);

  if (ret != STATUS_CODE_OK) {
    printf("i2c_write_then_read() failed with exit code %d\n", ret);
    return ret;
  }

  uint8_t mode1_sleep = (mode1 & ~MODE1_RESTART) | MODE1_SLEEP;

  PCA_WRITE_REG(PCA_MODE1, mode1_sleep);

  float prescale_f = (float)(PCA_DEFAULT_FREQ / (4096 * (float)pwm_freq)) - 1;

  uint8_t prescale_val = (uint8_t)(prescale_f + 0.5f); // rounding

  PCA_WRITE_REG(PCA_PRE_SCALE, prescale_val);

  PCA_WRITE_REG(PCA_MODE1, mode1_sleep & ~MODE1_SLEEP);

  usleep(1000);
  PCA_WRITE_REG(PCA_MODE1,
                (mode1_sleep & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
  PCA_WRITE_REG(PCA_MODE2, MODE2_OUTDRV);

  isInitialized = true;
  return STATUS_CODE_OK;
}

StatusCode pwm_controller_set_channel(PCAChannel channel,
                                      float delay_percentage,
                                      float duty_cycle) {
  if (delay_percentage + duty_cycle > 1.0f) {
    return STATUS_CODE_INVALID_ARGS;
  }

  uint16_t high_start = delay_percentage * PWM_RESOLUTION;
  uint16_t low_start = high_start + (duty_cycle * PWM_RESOLUTION);

  uint8_t buf[5];
  buf[0] = channel;
  buf[1] = high_start & 0xFF;
  buf[2] = (high_start >> 8) & 0xF;
  buf[3] = low_start & 0xFF;
  buf[4] = (low_start >> 8) & 0xF;

  StatusCode ret = i2c_write(I2C_BUS_2, PCA_I2C_ADDR, buf, 5);

  return ret;
}

StatusCode pwm_controller_stop_channel(PCAChannel channel) {
  // write to channel LEDX_OFF_H
  StatusCode ret = PCA_WRITE_REG(channel + 3, LEDX_FULL_OFF);
  return ret;
}

StatusCode pwm_controller_digital_set_channel(PCAChannel channel) {
  // write to channel LEDX_ON_H
  StatusCode ret = PCA_WRITE_REG(channel + 1, LEDX_FULL_ON);
  return ret;
}