#include "servo.h"

static uint8_t initialized = 0;
static float angle_storage[NUM_SERVO_CHANNELS];

StatusCode servo_init() {

  if (initialized == 1) {
    return STATUS_CODE_ALREADY_INITIALIZED;
  }

  StatusCode ret = pwm_controller_init(SERVO_PWM_FREQ_HZ);
  if (ret != STATUS_CODE_ALREADY_INITIALIZED && ret != STATUS_CODE_OK) {
    return ret;
  }

  initialized = 1;
  return ret;
}

StatusCode servo_set_angle(ServoChannel channel, float angle) {
  if (initialized == 0) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if (angle < MIN_ANGLE_DEGREES || angle > MAX_ANGLE_DEGREES ||
      channel >= NUM_SERVO_CHANNELS) {
    return STATUS_CODE_INVALID_ARGS;
  }

  int pcaChannel = SERVO_CHANNEL_TO_PWM_CHANNEL(channel);
  if (pcaChannel == INVALID_PCA_CHANNEL) {
    return STATUS_CODE_INVALID_ARGS;
  }

  float duty_cycle =
      ((angle / D_ANGLE) + AVERAGE_PULSE_WIDTH_MS) * SERVO_PWM_FREQ_HZ / 1000;

  StatusCode ret = pwm_controller_set_channel(pcaChannel, 0.0f, duty_cycle);

  if (ret == STATUS_CODE_OK) {
    angle_storage[channel] = angle;
  }

  return ret;
}

StatusCode servo_get(ServoChannel channel, float *angle) {
  if (channel >= NUM_SERVO_CHANNELS) {
    return STATUS_CODE_INVALID_ARGS;
  }

  *angle = angle_storage[channel];
  return STATUS_CODE_OK;
}
