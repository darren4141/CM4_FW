#pragma once

#include "global_enums.h"
#include "pwm_controller.h"

#define SERVO_PWM_FREQ_HZ 50

typedef enum {
  SERVO_CHANNEL_0 = 0,
  SERVO_CHANNEL_1 = 1,
  SERVO_CHANNEL_2 = 2,
  SERVO_CHANNEL_3 = 3,
  NUM_SERVO_CHANNELS = 4,
} ServoChannel;

#define SERVO_CHANNEL_TO_PWM_CHANNEL(x)                                        \
  ((x) == SERVO_CHANNEL_0   ? PCA_LED0_ON_L                                    \
   : (x) == SERVO_CHANNEL_1 ? PCA_LED1_ON_L                                    \
   : (x) == SERVO_CHANNEL_2 ? PCA_LED2_ON_L                                    \
   : (x) == SERVO_CHANNEL_3 ? PCA_LED3_ON_L                                    \
                            : INVALID_PCA_CHANNEL)

#define MINIMUM_PULSE_WIDTH_MS 1
#define MAXIMUM_PULSE_WIDTH_MS 2

#define AVERAGE_PULSE_WIDTH_MS                                                 \
  (MINIMUM_PULSE_WIDTH_MS + MAXIMUM_PULSE_WIDTH_MS) / 2

#define MIN_ANGLE_DEGREES -90
#define MAX_ANGLE_DEGREES 90

#define D_ANGLE MAX_ANGLE_DEGREES - MIN_ANGLE_DEGREES

StatusCode servo_init();

StatusCode servo_get(ServoChannel channel, float *angle);

StatusCode servo_set_angle(ServoChannel channel, float angle);

StatusCode servo_move_smooth(ServoChannel channel, float angle,
                             int step_delay_ms);
