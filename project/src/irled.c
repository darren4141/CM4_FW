#include "irled.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "cm4_gpio.h"

static pthread_t edge_thread;
static volatile bool is_int_triggered = false;
static volatile bool is_thread_running = false;
static struct timespec ts = {
    .tv_sec = 0, .tv_nsec = IRLED_THREAD_PERIOD_S * 1000 * 1000 * 1000};

static void *edge_thread_func(void *arg);
static bool irled_interrupt_fired(void);

static void *edge_thread_func(void *arg) {
  (void)arg;
  int event;

  while (is_thread_running) {
    gpio_get_edge_event(INT_PIN_1, &event);
    if (event == 1) {
      is_int_triggered = true;
    }
    nanosleep(&ts, NULL);
  }

  return NULL;
}

static bool irled_interrupt_fired(void) {
  if (is_int_triggered) {
    is_int_triggered = false;
    return true;
  }
  return false;
}

StatusCode irled_init() {
  StatusCode ret = gpio_set_mode(INT_PIN_1, GPIO_MODE_INPUT);
  if (ret != STATUS_CODE_OK) {
    printf("gpio_set_mode() failed with exit code: %u\n", ret);
  }

  ret = gpio_set_edge(INT_PIN_1, GPIO_EDGE_FALLING);
  if (ret != STATUS_CODE_OK) {
    printf("gpio_set_mode() failed with exit code: %u\n", ret);
  }

  is_thread_running = true;
  int threadRet = pthread_create(&edge_thread, NULL, edge_thread_func, NULL);
  if (threadRet != 0) {
    is_thread_running = false;
    return STATUS_CODE_THREAD_FAILURE;
  }

  return STATUS_CODE_OK;
}

StatusCode irled_deinit() {
  if (is_thread_running) {
    is_thread_running = false;
    pthread_join(edge_thread, NULL);
  }

  return STATUS_CODE_OK;
}