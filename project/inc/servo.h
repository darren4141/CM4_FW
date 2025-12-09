#pragma once

#include <stdbool.h>

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

#define SERVO_THREAD_FREQ_HZ 1000
#define SERVO_THREAD_PERIOD_S 1 / SERVO_THREAD_FREQ_HZ
#define SERVO_MAX_SPEED_DEG_S 360
#define SERV_MAX_STEP SERVO_MAX_SPEED_DEG_S / SERVO_THREAD_FREQ_HZ

typedef struct {
  ServoChannel channel;
  bool isRunning;
  float current_angle;
  float target_angle;
  float step;
} Servo;

StatusCode servo_init();

StatusCode servo_deinit();

StatusCode servo_get(ServoChannel channel, float *angle);

StatusCode servo_set_angle(ServoChannel channel, float angle);

/* angular_velocity is measured in turns per second*/
StatusCode servo_move_smooth(ServoChannel channel, float angle,
                             float angular_velocity);
