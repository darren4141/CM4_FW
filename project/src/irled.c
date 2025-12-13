#include "irled.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "cm4_gpio.h"
#include "cm4_i2c.h"

static StatusCode irled_read_reg(uint8_t reg, uint8_t *val);

#define IRLED_WRITE_REG(reg, val)                                              \
        i2c_write(I2C_BUS_1, MX_I2C_ADDR, (uint8_t[]) {reg, val}, 2);

#define IRLED_READ_REG(reg, val) irled_read_reg(reg, val);

static pthread_t edge_thread;
static volatile bool is_thread_running = false;
static struct timespec ts = {
  .tv_sec = 0, .tv_nsec = IRLED_THREAD_PERIOD_S * 1000 * 1000 * 1000
};

static void *edge_thread_func(void *arg);
static StatusCode max30102_read_fifo_to_buffer();

static Max30102Sample s_buffer[MAX30102_BUFFER_SIZE];
static volatile uint16_t s_head = 0;
static volatile uint16_t s_tail = 0;
static pthread_mutex_t s_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static StatusCode irled_read_reg(uint8_t reg, uint8_t *val)
{
  uint8_t read_buf;
  StatusCode ret =
    i2c_write_then_read(I2C_BUS_1, MX_I2C_ADDR, &reg, 1, &read_buf, 1);
  if (ret != STATUS_CODE_OK) {
    printf("Could not read from register: %d\n", reg);
    return ret;
  }

  *val = read_buf;
  return STATUS_CODE_OK;
}

static void *edge_thread_func(void *arg)
{
  (void)arg;
  int event;

  while (is_thread_running) {
    gpio_get_edge_event(INT_PIN_1, &event);
    if (event == 1) {
      gpio_clear_edge(INT_PIN_1);
      uint8_t status = 0;
      IRLED_READ_REG(MX_IS1, &status);

      if (status & IS1_A_FULL) {
        max30102_read_fifo_to_buffer();
      }

      if (status & IS1_ALC_OVF) {
        printf("Warning: Ambient light cancellation overflow is causing signal "
               "to be unreliable");
      }

      if (!(status & (IS1_A_FULL | IS1_ALC_OVF))) {
        printf("Warning: interrupt fired with invalid status: %d\n", status);
      }
    }
    nanosleep(&ts, NULL);
  }

  return NULL;
}

static StatusCode max30102_read_fifo_to_buffer()
{

  uint8_t wr = 0;
  uint8_t rd = 0;

  IRLED_READ_REG(MX_FIFO_WR_PTR, &wr);
  IRLED_READ_REG(MX_FIFO_RD_PTR, &rd);

  uint8_t count = (wr - rd) & 0x1F;

  for (uint8_t i = 0; i < count; i++) {
    Max30102Sample sample;

    uint8_t buf[6];
    StatusCode ret = i2c_write_then_read(I2C_BUS_1, MX_I2C_ADDR,
                                         (uint8_t[]) {MX_FIFO_DATA}, 1, buf, 6);
    if (ret != STATUS_CODE_OK) {
      printf("i2c read from fifo data register failed\n");
      return STATUS_CODE_FAILED;
    }

    sample.ir = ((uint32_t)(buf[0] & 0x03) << 16 | (uint32_t)(buf[1] << 8)
                 | (uint32_t)(buf[2]));

    sample.red = ((uint32_t)(buf[3] & 0x03) << 16 | (uint32_t)(buf[4] << 8)
                  | (uint32_t)(buf[5]));

    pthread_mutex_lock(&s_buffer_mutex);
    s_buffer[s_head] = sample;
    s_head = (s_head + 1) % MAX30102_BUFFER_SIZE;
    if (s_head == s_tail) {
      s_tail = (s_tail + 1) % MAX30102_BUFFER_SIZE;
    }
    pthread_mutex_unlock(&s_buffer_mutex);
  }

  return STATUS_CODE_OK;
}

StatusCode irled_init()
{
  StatusCode ret = STATUS_CODE_OK;

  ret = i2c_get_initialized(I2C_BUS_1);
  if (ret != STATUS_CODE_OK) {
    printf("i2c bus: %u is not initialized\n", ret);
    return ret;
  }

  IRLED_WRITE_REG(MX_MODE_CONFIG, MODE_CONFIG_RESET);

  uint8_t mode_cfg = 0;
  const int max_retries = 50;

  for (int i = 0; i < max_retries; i++) {
    IRLED_READ_REG(MX_MODE_CONFIG, &mode_cfg);
    if ((mode_cfg & MODE_CONFIG_RESET) == 0) {
      break;
    }
    nanosleep(&ts, NULL);
  }

  uint8_t partId;
  ret = IRLED_READ_REG(MX_PART_ID, &partId);
  if (ret != STATUS_CODE_OK) {
    printf("IRLED_READ_REG() failed with exit code: %u\n", ret);
    return STATUS_CODE_FAILED;
  }
  else {
    printf("irled init, part id: %d, expected 0x15\n", partId);
  }

  uint8_t int_status;
  ret = IRLED_READ_REG(MX_IS1, &int_status);
  if (ret != STATUS_CODE_OK) {
    printf("IRLED_READ_REG() failed with exit code: %u\n", ret);
    return STATUS_CODE_FAILED;
  }
  else {
    printf("Cleared interrupt status 1 with value: %d\n", int_status);
  }
  ret = IRLED_READ_REG(MX_IS2, &int_status);
  if (ret != STATUS_CODE_OK) {
    printf("IRLED_READ_REG() failed with exit code: %u\n", ret);
    return STATUS_CODE_FAILED;
  }
  else {
    printf("Cleared interrupt status 2 with value: %d\n", int_status);
  }

  IRLED_WRITE_REG(MX_FIFO_CONFIG, FIFO_CONFIG_SAMPLE_AVERAGE_8
                  | FIFO_CONFIG_ROLLOVER_EN
                  | FIFO_CONFIG_A_FULL_16_SAMPLES);

  IRLED_WRITE_REG(MX_FIFO_WR_PTR, 0x00);
  IRLED_WRITE_REG(MX_OVF_COUNTER, 0x00);
  IRLED_WRITE_REG(MX_FIFO_RD_PTR, 0x00);

  IRLED_WRITE_REG(MX_SPO2_CONFIG, SPO2_CONFIG_ADC_RGE_4096
                  | SPO2_CONFIG_SAMPLE_RT_50
                  | SPO2_CONFIG_LED_PW_18);

  IRLED_WRITE_REG(MX_LED1_PULSE_AMP, 0x0F);   /* 3.0mA*/
  IRLED_WRITE_REG(MX_LED2_PULSE_AMP, 0x0F);   /* 3.0mA*/

  IRLED_WRITE_REG(MX_IE1, IE1_A_FULL_EN | IE1_ALC_OVF_EN);
  IRLED_WRITE_REG(MX_MODE_CONFIG, MODE_CONFIG_SPO2_MODE);

  TRY(gpio_set_mode(INT_PIN_1, GPIO_MODE_INPUT));
  TRY(gpio_set_edge(INT_PIN_1, GPIO_EDGE_FALLING));

  return STATUS_CODE_OK;
}

StatusCode irled_deinit()
{
  if (is_thread_running) {
    is_thread_running = false;
    pthread_join(edge_thread, NULL);
  }

  return STATUS_CODE_OK;
}

StatusCode irled_start_reading()
{
  is_thread_running = true;
  int threadRet = pthread_create(&edge_thread, NULL, edge_thread_func, NULL);
  if (threadRet != 0) {
    is_thread_running = false;
    return STATUS_CODE_THREAD_FAILURE;
  }
  return STATUS_CODE_OK;
}

StatusCode irled_stop_reading()
{
  if (is_thread_running) {
    is_thread_running = false;
    int ret = pthread_join(edge_thread, NULL);
    if (ret != 0) {
      return STATUS_CODE_THREAD_FAILURE;
    }
  }
  return STATUS_CODE_OK;
}

StatusCode irled_pop_sample(Max30102Sample *sample)
{
  pthread_mutex_lock(&s_buffer_mutex);

  if (s_head != s_tail) {
    *sample = s_buffer[s_tail];
    s_tail = (s_tail + 1) % MAX30102_BUFFER_SIZE;
  }
  else {
    pthread_mutex_unlock(&s_buffer_mutex);
    return STATUS_CODE_FAILED;
  }

  pthread_mutex_unlock(&s_buffer_mutex);
  return STATUS_CODE_OK;
}
