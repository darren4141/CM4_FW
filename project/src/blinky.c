#include "blinky.h"

#include <stdint.h>
#include <stdio.h>

#include "pwm_controller.h"

static LedState led_state[2];

StatusCode blinky_init(void)
{

  StatusCode ret = pwm_controller_get_initialized();

  if (ret != STATUS_CODE_OK) {
    printf("pwm controller not initialized\n");
    return ret;
  }

  led_state[0] = LED_STATE_OFF;
  led_state[1] = LED_STATE_OFF;

  return STATUS_CODE_OK;
}

StatusCode blinky_set(LedChannel channel, LedState state)
{
  StatusCode ret = STATUS_CODE_OK;
  if ((channel != LED_CHANNEL_4) && (channel != LED_CHANNEL_5)) {
    printf("Led channel %d is invalid\n", channel);
    return STATUS_CODE_INVALID_ARGS;
  }

  if (state == LED_STATE_ON) {
    ret =
      pwm_controller_digital_set_channel(LED_CHANNEL_TO_PCA_CHANNEL(channel));

    led_state[LED_CHANNEL_TO_INDEX(channel)] = LED_STATE_ON;
  }
  else if (state == LED_STATE_OFF) {
    ret = pwm_controller_stop_channel(LED_CHANNEL_TO_PCA_CHANNEL(channel));
    led_state[LED_CHANNEL_TO_INDEX(channel)] = LED_STATE_OFF;
  }
  else {
    printf("Led state %d is invalid\n", state);
    return STATUS_CODE_INVALID_ARGS;
  }

  if (ret != STATUS_CODE_OK) {
    return ret;
  }

  return ret;
}

StatusCode blinky_toggle(LedChannel channel)
{
  StatusCode ret = STATUS_CODE_OK;
  if ((channel != LED_CHANNEL_4) && (channel != LED_CHANNEL_5)) {
    printf("Led channel %d is invalid\n", channel);
    return STATUS_CODE_INVALID_ARGS;
  }

  LedState curr_state = led_state[LED_CHANNEL_TO_INDEX(channel)];

  if ((curr_state == LED_STATE_ON) || (curr_state == LED_STATE_PWM)) {
    ret = blinky_set(channel, LED_STATE_OFF);
  }
  else if (curr_state == LED_STATE_OFF) {
    ret = blinky_set(channel, LED_STATE_ON);
  }
  else {
    return STATUS_CODE_INVALID_ARGS;
  }

  return ret;
}

StatusCode blinky_set_pwm(LedChannel channel, float pwm_percentage)
{
  StatusCode ret = STATUS_CODE_OK;
  if ((channel != LED_CHANNEL_4) && (channel != LED_CHANNEL_5)) {
    printf("Led channel %d is invalid\n", channel);
    return STATUS_CODE_INVALID_ARGS;
  }

  if ((pwm_percentage < 0.0f) || (pwm_percentage > 1.0f)) {
    return STATUS_CODE_INVALID_ARGS;
  }
  else if (pwm_percentage == 0) {
    ret = blinky_set(channel, LED_STATE_OFF);
  }
  else if (pwm_percentage == 1) {
    ret = blinky_set(channel, LED_STATE_ON);
  }
  else {
    ret = pwm_controller_set_channel(LED_CHANNEL_TO_PCA_CHANNEL(channel), 0.0f,
                                     pwm_percentage);
    if (ret != STATUS_CODE_OK) {
      return ret;
    }
    led_state[LED_CHANNEL_TO_INDEX(channel)] = LED_STATE_PWM;
  }

  return ret;
}