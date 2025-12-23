#pragma once

#include "global_enums.h"

#define LED_CHANNEL_TO_PCA_CHANNEL(x)                                          \
        ((x) == LED_CHANNEL_4 ? PCA_LED4_ON_L                                      \
         : (x) == LED_CHANNEL_5 ? PCA_LED5_ON_L                                      \
         : INVALID_PCA_CHANNEL)

#define LED_CHANNEL_TO_INDEX(x)                                                \
        ((x) == LED_CHANNEL_4 ? 0 : (x) == LED_CHANNEL_5 ? 1 : -1)

typedef enum {
  LED_CHANNEL_4 = 4,
  LED_CHANNEL_5 = 5,
} LedChannel;

typedef enum {
  LED_STATE_OFF = 0,
  LED_STATE_ON  = 1,
  LED_STATE_PWM = 2
} LedState;

/**
 * Initializie blinky
 */
StatusCode blinky_init(void);

/**
 * Set the state of a given led, either on or off
 */
StatusCode blinky_set(LedChannel channel, LedState state);

/**
 * Toggle the state of a given led
 */
StatusCode blinky_toggle(LedChannel channel);

/**
 * Set the pwm of an led, pwm_percentage ranges from 0-1
 */
StatusCode blinky_set_pwm(LedChannel channel, float pwm_percentage);
