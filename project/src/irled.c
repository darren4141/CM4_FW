#include "irled.h"

#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#include "cm4_gpio.h"
#include "cm4_i2c.h"

static StatusCode irled_read_reg(uint8_t reg, uint8_t *val);

#define IRLED_WRITE_REG(reg, val)                                              \
        i2c_write(I2C_BUS_2, MX_I2C_ADDR, (uint8_t[]) {reg, val}, 2);

#define IRLED_READ_REG(reg, val) irled_read_reg(reg, val);

static pthread_t int_edge_thread;
static atomic_bool is_thread_running = false;
static struct timespec ts = {
  .tv_sec = IRLED_THREAD_PERIOD_NS / NSEC_PER_SEC,
  .tv_nsec = IRLED_THREAD_PERIOD_NS % NSEC_PER_SEC,
};

static void *int_edge_thread_func(void *arg);
static StatusCode max30102_read_fifo_to_buffer();

static Max30102Sample s_buffer[MAX30102_BUFFER_SIZE];
static volatile uint16_t s_head = 0;
static volatile uint16_t s_tail = 0;
static pthread_mutex_t s_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t s_buffer_cv = PTHREAD_COND_INITIALIZER;

static pthread_t hr_thread;
static atomic_bool is_hr_thread_running = false;

static atomic_int s_bpm = 0;
static atomic_int s_confidence_pct = 0;

static uint16_t irled_pop_multiple(Max30102Sample *out, uint16_t max_n);

static bool new_hb = false;

static VerbosityLevel verbosity = VERBOSITY_NONE;

static StatusCode irled_read_reg(uint8_t reg, uint8_t *val)
{
  uint8_t read_buf;
  StatusCode ret =
    i2c_write_then_read(I2C_BUS_2, MX_I2C_ADDR, &reg, 1, &read_buf, 1);
  if (ret != STATUS_CODE_OK) {
    printf("Could not read from register: %d\n", reg);
    return ret;
  }

  *val = read_buf;
  return STATUS_CODE_OK;
}

static void *int_edge_thread_func(void *arg)
{
  (void)arg;
  int event;
  // int state;
  uint32_t loops = 0;

  while (atomic_load(&is_thread_running)) {
    gpio_get_edge_event(INT_PIN_1, &event);

    if (event == 1) {
      VERB1_PRINTF("interrupt triggered\n");

      gpio_clear_edge(INT_PIN_1);
      uint8_t status = 0;
      IRLED_READ_REG(MX_IS1, &status);

      if (status & IS1_A_FULL) {
        StatusCode ret = max30102_read_fifo_to_buffer();
      }
      else {
        printf("Warning: interrupt fired with invalid status: %d\n", status);
      }
    }
    else {
      VERB1_PRINTF("no edge detected\n");
    }
    nanosleep(&ts, NULL);
  }
  VERB1_PRINTF("exiting thread\n");
  return NULL;
}

static StatusCode max30102_read_fifo_to_buffer(void)
{
  StatusCode ret;

  uint8_t wr = 0, rd = 0;

  ret = IRLED_READ_REG(MX_FIFO_WR_PTR, &wr);
  if (ret != STATUS_CODE_OK) {
    return ret;
  }

  ret = IRLED_READ_REG(MX_FIFO_RD_PTR, &rd);
  if (ret != STATUS_CODE_OK) {
    return ret;
  }

  uint8_t count = (wr - rd) & 0x1F;
  if (count == 0) {
    return STATUS_CODE_FAILED;
  }

  uint8_t to_read = (count > 16) ? 16 : count;

  pthread_mutex_lock(&s_buffer_mutex);

  do {
    for (uint8_t i = 0; i < to_read; i++) {
      uint8_t buf[6];
      ret = i2c_write_then_read(I2C_BUS_2, MX_I2C_ADDR,
                                (uint8_t[]) { MX_FIFO_DATA }, 1, buf, 6);
      if (ret != STATUS_CODE_OK) {
        pthread_mutex_unlock(&s_buffer_mutex);
        return STATUS_CODE_FAILED;
      }

      Max30102Sample sample;
      sample.ir = ((uint32_t)(buf[0] & 0x03) << 16)
                  | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
      sample.red = ((uint32_t)(buf[3] & 0x03) << 16)
                   | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];

      s_buffer[s_head] = sample;
      s_head = (s_head + 1) % MAX30102_BUFFER_SIZE;
      if (s_head == s_tail) {
        s_tail = (s_tail + 1) % MAX30102_BUFFER_SIZE;
      }
    }

    count -= to_read;
    to_read = (count > 16) ? 16 : count;
  } while (count > 0);

  pthread_cond_signal(&s_buffer_cv);
  pthread_mutex_unlock(&s_buffer_mutex);

  return STATUS_CODE_OK;
}

static uint16_t irled_buffer_count_unsafe(void)
{
  if (s_head >= s_tail) {
    return (uint16_t)(s_head - s_tail);
  }
  return (uint16_t)(MAX30102_BUFFER_SIZE - (s_tail - s_head));
}

static void *hr_calc_thread_func(void *arg)
{

  VERB1_PRINTF("HR calc thread starting...\n");
  (void)arg;
  Max30102Sample block[256];

  #define IBI_BUF 8
  uint32_t ibi_samples[IBI_BUF] = {0};
  uint8_t ibi_head = 0;
  uint8_t ibi_count = 0;

  atomic_store(&is_hr_thread_running, true);

  float prev = 0.0f;
  float prev2 = 0.0f;
  float noise_est = 1.0f;
  float threshold = 0.0f;
  uint32_t last_peak_idx = 0;
  uint32_t sample_idx = 0;

  /**
   * Alpha EMA calculation:
   * Time constant = 1/alpha = 150
   * Sample rate is 100Hz
   * 150 / 100Hz = 1.5 which is well over the min heart rate ~= 50bpm = 1.25s
   * Therefore HR = AC component will be ignored by the EMA
   * If sample rate changes we must readjust alpha_ema
   */
  float alpha_ema = 0.05f;
  float dc = 0.0f;
  float fast_bpf = 0.0f;
  float slow_bpf = 0.0f;

  /**
   * alpha BPF calculation:
   * TC = 1 / alpha * sample_rate
   * TCfast = 1 / (0.04 * 100) =  -> 0.25s -> 240 BPM
   * TCslow = 1 / (0.006 * 100) = 1.666s -> 36 BPM
   */
  float alpha_fast = 0.04f;
  float alpha_slow = 0.00001f;

  float alpha_smoothing = 0.1f;

  float alpha_threshold = 0.05;

  uint32_t ibi_min = (uint32_t)(HR_SMPL_HZ * 60.0f / 200.0f);
  uint32_t ibi_max = (uint32_t)(HR_SMPL_HZ * 60.0f / 40.0f);

  VERB2_PRINTF("ibi_min: %u, ibi_max: %u\n", ibi_min, ibi_max);

  // uint32_t count = 0;

  while (atomic_load(&is_thread_running)) {
    uint16_t n = irled_pop_multiple(block, (uint16_t)(sizeof(block) / sizeof(block[0])));
    // count += n;

    // printf("nc: %u\n", count);
    // time_t current_time;

    // current_time = time(NULL);

    // printf("Current time is %s", ctime(&current_time));

    if (n == 0) {
      struct timespec ts = {0, 1000000};
      nanosleep(&ts, NULL);
      continue;
    }

    for (uint16_t i = 0; i < n; i++) {
      float val = (float)block[i].ir;
      VERB3_PRINTF("%f \n", val);

      // Step 1: EMA
      dc = dc + alpha_ema * (val - dc);

      float ac = val - dc;
      VERB3_PRINTF("AC:%f ", ac);

      // Step 2: BPF
      fast_bpf = fast_bpf + alpha_fast * (ac - fast_bpf);
      slow_bpf = slow_bpf + alpha_slow * (ac - slow_bpf);

      float bp = fast_bpf - slow_bpf;
      VERB3_PRINTF("BPF:%f ", bp);

      // Step 3: smoothing
      float y = prev + alpha_smoothing * (bp - prev);
      VERB3_PRINTF("Smooth:%f ", y);

      // Step 4: update thresholding
      noise_est = noise_est + alpha_threshold * (y - noise_est);
      threshold = 1.0f * noise_est;

      VERB3_PRINTF("Threshold:%f\n", threshold);

      // Step 5: process if local max
      bool local_max = ((prev > prev2) && (prev > y));
      bool above_thresh = (fabsf(prev) > threshold);
      if (local_max && above_thresh) {
        VERB1_PRINTF("detected peak\n");
        if (last_peak_idx == 0) {
          VERB2_PRINTF("First peak detected, index: %u\n", sample_idx);
          last_peak_idx = sample_idx;
        }
        else {
          uint32_t ibi = sample_idx - last_peak_idx;
          VERB2_PRINTF("%u", ibi);

          // Store up to IBI_BUF vals in a ring buffer
          if ((ibi >= ibi_min) && (ibi <= ibi_max)) {
            last_peak_idx = sample_idx;

            ibi_samples[ibi_head] = ibi;
            ibi_head = (ibi_head + 1) % IBI_BUF;
            if (ibi_count < IBI_BUF) {
              ibi_count++;
            }

            uint32_t tmp[IBI_BUF];
            for (uint8_t k = 0; k < ibi_count; k++) {
              tmp[k] = ibi_samples[k];
            }

            // Take the median of the ring buffer
            for (uint8_t a = 1; a < ibi_count; a++) {
              uint32_t key = tmp[a];
              int b = a - 1;
              while (b >= 0 && tmp[b] > key) {
                tmp[b + 1] = tmp[b];
                b--;
              }
              tmp[b + 1] = key;
            }
            uint32_t median = tmp[ibi_count / 2];

            float bpm = 60.0f * HR_SMPL_HZ / (float)median;

            atomic_store(&s_bpm, (int)(bpm));
            new_hb = true;
            VERB1_PRINTF("-------------------------HB----------------------------\n");
          }
          else if (ibi >= ibi_min) {
            last_peak_idx = sample_idx;
          }
        }
      }
      prev2 = prev;
      prev = y;
      sample_idx++;
    }
  }

  atomic_store(&is_hr_thread_running, false);
  return NULL;
}

StatusCode irled_init()
{
  StatusCode ret = STATUS_CODE_OK;

  ret = i2c_get_initialized(I2C_BUS_2);
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
    printf("irled init, part id: %02X, expected 0x15\n", partId);
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

  IRLED_WRITE_REG(MX_FIFO_CONFIG, FIFO_CONFIG_SAMPLE_AVERAGE_4
                  | FIFO_CONFIG_ROLLOVER_EN
                  | FIFO_CONFIG_A_FULL_4_SAMPLES);

  IRLED_WRITE_REG(MX_FIFO_WR_PTR, 0x00);
  IRLED_WRITE_REG(MX_OVF_COUNTER, 0x00);
  IRLED_WRITE_REG(MX_FIFO_RD_PTR, 0x00);

  IRLED_WRITE_REG(MX_SPO2_CONFIG, SPO2_CONFIG_ADC_RGE_4096
                  | SPO2_CONFIG_SAMPLE_RT_400
                  | SPO2_CONFIG_LED_PW_18);

  IRLED_WRITE_REG(MX_LED1_PULSE_AMP, 0x3C);
  IRLED_WRITE_REG(MX_LED2_PULSE_AMP, 0x3C);

  IRLED_WRITE_REG(MX_IE1, IE1_A_FULL_EN);
  IRLED_WRITE_REG(MX_MODE_CONFIG, MODE_CONFIG_SPO2_MODE);

  TRY(gpio_set_mode(INT_PIN_1, GPIO_MODE_INPUT));
  TRY(gpio_set_edge(INT_PIN_1, GPIO_EDGE_FALLING));

  return STATUS_CODE_OK;
}

StatusCode irled_deinit()
{
  if (atomic_load(&is_thread_running)) {
    atomic_store(&is_thread_running, false);
    pthread_join(int_edge_thread, NULL);
  }

  printf("Deinitializing\n");

  IRLED_WRITE_REG(MX_MODE_CONFIG, MODE_CONFIG_RESET);
  IRLED_WRITE_REG(MX_IE1, 0x00);

  return STATUS_CODE_OK;
}

StatusCode irled_start_reading()
{
  atomic_store(&is_thread_running, true);
  int threadRet = pthread_create(&int_edge_thread, NULL, int_edge_thread_func, NULL);
  if (threadRet != 0) {
    atomic_store(&is_thread_running, false);
    return STATUS_CODE_THREAD_FAILURE;
  }

  threadRet = pthread_create(&hr_thread, NULL, hr_calc_thread_func, NULL);
  if (threadRet != 0) {
    atomic_store(&is_thread_running, false);
    return STATUS_CODE_THREAD_FAILURE;
  }


  return STATUS_CODE_OK;
}

StatusCode irled_stop_reading(void)
{
  if (!atomic_load(&is_thread_running)) {
    return STATUS_CODE_OK;
  }

  printf("stopping thread\n");
  atomic_store(&is_thread_running, false);

  // Wake up hr thread if it's blocked in cond_wait
  pthread_mutex_lock(&s_buffer_mutex);
  pthread_cond_broadcast(&s_buffer_cv);
  pthread_mutex_unlock(&s_buffer_mutex);

  pthread_join(int_edge_thread, NULL);

  if (atomic_load(&is_hr_thread_running)) {
    pthread_join(hr_thread, NULL);
  }

  printf("threads stopped\n");
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

static uint16_t irled_pop_multiple(Max30102Sample *out, uint16_t max_n)
{
  pthread_mutex_lock(&s_buffer_mutex);

  while (atomic_load(&is_thread_running) && irled_buffer_count_unsafe() == 0) {
    pthread_cond_wait(&s_buffer_cv, &s_buffer_mutex);
  }

  uint16_t n = 0;
  while (n < max_n && s_head != s_tail) {
    out[n] = s_buffer[s_tail];
    s_tail = (s_tail + 1) % MAX30102_BUFFER_SIZE;
    n++;
  }

  pthread_mutex_unlock(&s_buffer_mutex);
  return n;
}

int irled_get_bpm(void)
{
  return atomic_load(&s_bpm);
}

int irled_get_confidence(void)
{
  return atomic_load(&s_confidence_pct);
}

int irled_get_hb_state(void)
{
  return new_hb ? 1 : 0;
}

int irled_clear_hb_state(void)
{
  new_hb = false;
  return 1;
}

void irled_set_verbosity_level(VerbosityLevel new_verbosity)
{
  verbosity = new_verbosity;
  printf("Set verbosity level to %u\n", verbosity);
}