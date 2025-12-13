#include "servo.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

static uint8_t initialized = 0;
static Servo servo[NUM_SERVO_CHANNELS];
static volatile bool servo_thread_running = false;
static pthread_t servo_thread;

static struct timespec ts = {
  .tv_sec = 0, .tv_nsec = SERVO_THREAD_PERIOD_S * 1000 * 1000 * 1000
};

static void servo_update_all();
static void servo_update(Servo *servo);
static void *servo_thread_func(void *args);

static void servo_update_all()
{
  bool none_running = true;
  for (uint8_t i = 0; i < NUM_SERVO_CHANNELS; i++) {
    if (servo[i].isRunning == true) {
      none_running = false;
      servo_update(&servo[i]);
    }
  }

  if (none_running) {
    servo_thread_running = false;
  }
}

static void servo_update(Servo *servo)
{

  float diff = servo->target_angle - servo->current_angle;

  if ((diff < 0.1f) && (diff > -0.1f)) {
    servo->current_angle = servo->target_angle;
    servo->isRunning = false;
  }
  else {
    servo_set_angle(servo->channel, servo->current_angle + servo->step);
  }
}

static void *servo_thread_func(void *args)
{
  (void)args;

  while (servo_thread_running) {
    servo_update_all();
    nanosleep(&ts, NULL);
  }

  return NULL;
}

StatusCode servo_init()
{

  if (initialized == 1) {
    return STATUS_CODE_ALREADY_INITIALIZED;
  }

  StatusCode ret = pwm_controller_get_initialized();

  if (ret != STATUS_CODE_OK) {
    printf("pwm controller not initialized\n");
    return ret;
  }

  ret = pwm_controller_init(SERVO_PWM_FREQ_HZ);
  if ((ret != STATUS_CODE_ALREADY_INITIALIZED) && (ret != STATUS_CODE_OK)) {
    return ret;
  }

  initialized = 1;
  return ret;
}

StatusCode servo_deinit()
{
  if (initialized == 0) {
    return STATUS_CODE_OK;
  }

  if (servo_thread_running) {
    servo_thread_running = false;
    pthread_join(servo_thread, NULL);
  }

  initialized = 0;

  return STATUS_CODE_OK;
}

StatusCode servo_set_angle(ServoChannel channel, float angle)
{
  if (initialized == 0) {
    return STATUS_CODE_NOT_INITIALIZED;
  }

  if ((angle < MIN_ANGLE_DEGREES) || (angle > MAX_ANGLE_DEGREES)
      || (channel >= NUM_SERVO_CHANNELS)) {
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
    servo[channel].current_angle = angle;
  }

  return ret;
}

StatusCode servo_get(ServoChannel channel, float *angle)
{
  if (channel >= NUM_SERVO_CHANNELS) {
    return STATUS_CODE_INVALID_ARGS;
  }

  *angle = servo[channel].current_angle;
  return STATUS_CODE_OK;
}

StatusCode servo_move_smooth(ServoChannel channel, float angle,
                             float angular_velocity)
{

  if ((angular_velocity <= 0) || (channel < 0) || (channel >= NUM_SERVO_CHANNELS)) {
    return STATUS_CODE_INVALID_ARGS;
  }

  if (angle == servo[channel].current_angle) {
    printf("Already at target");
    return STATUS_CODE_OK;
  }

  servo[channel].channel = channel;
  servo[channel].isRunning = true;
  servo[channel].target_angle = angle;
  servo[channel].step = angular_velocity / SERVO_THREAD_FREQ_HZ;

  if (servo[channel].target_angle < servo[channel].current_angle) {
    servo[channel].step *= -1;
  }

  if (!servo_thread_running) {
    servo_thread_running = true;
    int ret = pthread_create(&servo_thread, NULL, servo_thread_func, NULL);
    if (ret != 0) {
      servo[channel].isRunning = false;
      servo_thread_running = false;
      return STATUS_CODE_THREAD_FAILURE;
    }
  }

  return STATUS_CODE_OK;
}