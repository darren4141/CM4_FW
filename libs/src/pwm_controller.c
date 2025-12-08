#include "pwm_controller.h"
#include "cm4_i2c.h"

#define PCA_WRITE_REG(reg, val)                                                \
  i2c_write(I2C_BUS_2, PCA_I2C_ADDR, (uint8_t[]){reg, val}, 2);

StatusCode pwm_controller_init(uint32_t pwm_freq) {
  if (pwm_freq < 24 || pwm_freq > 1526) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
  }

  StatusCode ret = i2c_init(I2C_BUS_2, PCA_I2C_HZ);
  if (ret != STATUS_CODE_OK && ret != STATUS_CODE_ALREADY_INITIALIZED) {
    perror("i2c_init() failed with exit code: %u\n", ret);
    return ret;
  }

  PCA_WRITE_REG(PCA_MODE1, MODE1_AI | MODE1_SLEEP);

  uint32_t prescale_val = (PCA_DEFAULT_FREQ / (4096 * pwm_freq)) - 1;

  PCA_WRITE_REG(PCA_PRE_SCALE, prescale_val);
  PCA_WRITE_REG(PCA_MODE1, MODE1_AI & ~MODE1_SLEEP);
  PCA_WRITE_REG(PCA_MODE2, MODE2_OUTDRV);
}

StatusCode pwm_controller_start_channel(PCAChannel channel,
                                        float delay_percentage,
                                        float duty_cycle) {
  if (delay_percentage + duty_cycle > 1.0f) {
    return STATUS_CODE_ARGS_OUT_OF_RANGE;
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
  StatusCode ret = PCA_WRITE_REG(channel + 3, LEDX_FULL_OFF);
  return ret;
}