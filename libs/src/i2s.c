#include "cm4_i2s.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const unsigned kRate = 48000;   // sample rate
static const unsigned pb_kCh = 1;
static const unsigned rec_kCh = 2;
static const snd_pcm_format_t pb_kFmt = SND_PCM_FORMAT_S16_LE;
static const snd_pcm_format_t rec_kFmt = SND_PCM_FORMAT_S32_LE;
static const snd_pcm_uframes_t kPeriodFrames = 1024;

static struct timespec ts = {
  .tv_sec = 0, .tv_nsec = I2_WAITING_PERIOD_S * 1000 * 1000 * 1000
};

static pthread_t playback_thread;
static void playback_thread_deactivate();
static void *playback_thread_func(void *arg);
static pthread_mutex_t s_playback_mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool is_playback_thread_running = false;
static atomic_bool is_playback_active = false;
static atomic_bool is_playback_idle = true;

typedef struct {
  snd_pcm_t *playback;
  uint8_t *data;
  size_t data_len;
  uint8_t *buf;
  size_t buf_size_bytes;
  size_t bytes_per_frame;
} PlaybackThreadInfo_s;


static pthread_t record_thread;
static void record_thread_deactivate();
static void *record_thread_func(void *arg);
static pthread_mutex_t s_record_mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool is_record_thread_running = false;
static atomic_bool is_record_active = false;
static atomic_bool is_record_idle = true;

typedef struct {
  snd_pcm_t *capture;
} RecordThreadInfo_s;


static PlaybackThreadInfo_s playback_thread_info;
static RecordThreadInfo_s record_thread_info;

static uint8_t g_rec_rb[RING_BUF_SIZE];
static _Atomic uint32_t g_rb_w = 0;   // write index
static _Atomic uint32_t g_rb_r = 0;   // read index

static int i2s_recover(snd_pcm_t *h, int ret, const char *tag);
static int i2s_open_config(snd_pcm_t **h, const char *dev, snd_pcm_stream_t stream,
                           unsigned rate, unsigned ch, snd_pcm_format_t fmt,
                           snd_pcm_uframes_t period_frames);
static inline uint32_t rb_used(uint32_t r, uint32_t w);
static inline uint32_t rb_free(uint32_t r, uint32_t w);
static void rb_push(const uint8_t *data, uint32_t len);

static inline uint32_t rb_used(uint32_t r, uint32_t w)
{
  return (w - r);
}

static inline uint32_t rb_free(uint32_t r, uint32_t w)
{
  return RING_BUF_SIZE - rb_used(r, w);
}

static void rb_push(const uint8_t *data, uint32_t len)
{
  uint32_t r = atomic_load_explicit(&g_rb_r, memory_order_acquire);
  uint32_t w = atomic_load_explicit(&g_rb_w, memory_order_relaxed);

  if (len > rb_free(r, w)) {
    // Drop newest chunk if buffer would overflow (or drop oldest by advancing r)
    return;
  }

  uint32_t wpos = w % RING_BUF_SIZE;
  uint32_t first = RING_BUF_SIZE - wpos;

  if (first > len) {
    first = len;
  }

  memcpy(&g_rec_rb[wpos], data, first);
  memcpy(&g_rec_rb[0], data + first, len - first);

  atomic_store_explicit(&g_rb_w, w + len, memory_order_release);
}

int i2s_rb_pop(uint8_t *out, uint32_t len)
{
  uint32_t r = atomic_load_explicit(&g_rb_r, memory_order_relaxed);
  uint32_t w = atomic_load_explicit(&g_rb_w, memory_order_acquire);

  uint32_t used = rb_used(r, w);
  if (used == 0) {
    return 0;
  }

  uint32_t n = (used < len) ? used : len;

  uint32_t rpos = r % RING_BUF_SIZE;
  uint32_t first = RING_BUF_SIZE - rpos;

  if (first > n) {
    first = n;
  }

  memcpy(out, &g_rec_rb[rpos], first);
  memcpy(out + first, &g_rec_rb[0], n - first);

  atomic_store_explicit(&g_rb_r, r + n, memory_order_release);
  return n;
}

static inline int16_t clamp16(int32_t x)
{
  if (x > 32767) {
    return 32767;
  }
  if (x < -32768) {
    return -32768;
  }
  return (int16_t)x;
}

static int i2s_recover(snd_pcm_t *h, int ret, const char *tag)
{
  if (ret == -EPIPE) {
    printf("Error: %s: XRUN\n", tag);
    return snd_pcm_prepare(h);
  }
  else if (ret == -ESTRPIPE) {
    printf("Error: %s: SUSPEND\n", tag);
    while ((ret = snd_pcm_resume(h)) == -EAGAIN) {
      usleep(1000);
    }
    if (ret < 0) {
      return snd_pcm_prepare(h);
    }
    return 0;
  }
  return ret;
}

static int i2s_open_config(snd_pcm_t **h, const char *dev, snd_pcm_stream_t stream,
                           unsigned rate, unsigned ch, snd_pcm_format_t fmt,
                           snd_pcm_uframes_t period_frames)
{
  int ret = snd_pcm_open(h, dev, stream, 0);
  if (ret < 0) {
    printf("snd_pcm_open(%s) failed: %s\n", dev, snd_strerror(ret));
    return ret;
  }

  snd_pcm_hw_params_t *hw;
  snd_pcm_hw_params_alloca(&hw);
  snd_pcm_hw_params_any(*h, hw);

  if ((ret = snd_pcm_hw_params_set_access(*h, hw, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    return ret;
  }

  if ((ret = snd_pcm_hw_params_set_format(*h, hw, fmt)) < 0) {
    return ret;
  }

  if ((ret = snd_pcm_hw_params_set_channels(*h, hw, ch)) < 0) {
    return ret;
  }

  unsigned r = rate;
  if ((ret = snd_pcm_hw_params_set_rate_near(*h, hw, &r, 0)) < 0) {
    return ret;
  }

  snd_pcm_uframes_t p = period_frames;
  if ((ret = snd_pcm_hw_params_set_period_size_near(*h, hw, &p, 0)) < 0) {
    return ret;
  }

  if ((ret = snd_pcm_hw_params(*h, hw)) < 0) {
    return ret;
  }

  if ((ret = snd_pcm_prepare(*h)) < 0) {
    return ret;
  }

  printf("%s configured: rate=%u ch=%u fmt=%s period=%lu\n", dev, r, ch, snd_pcm_format_name(fmt), (unsigned long)p);
  return 0;
}

static void record_thread_deactivate()
{

  pthread_mutex_lock(&s_record_mutex);

  if (record_thread_info.capture) {
    snd_pcm_drop(record_thread_info.capture);
    snd_pcm_close(record_thread_info.capture);
    record_thread_info.capture = NULL;
  }
  pthread_mutex_unlock(&s_record_mutex);
}

static void *record_thread_func(void *arg)
{
  (void)arg;
  const size_t bytes_per_sample = snd_pcm_format_physical_width(rec_kFmt) / 8;
  const size_t bytes_per_frame = rec_kCh * bytes_per_sample;
  const size_t buf_bytes = bytes_per_frame * kPeriodFrames;

  uint8_t *buf = (uint8_t *)malloc(buf_bytes);
  if (buf == NULL) {
    printf("record thread - malloc failed\n");
    record_thread_deactivate();
    atomic_store(&is_record_idle, true);
    atomic_store(&is_record_thread_running, false);
  }

  while (atomic_load(&is_record_thread_running)) {
    snd_pcm_t *cap;
    pthread_mutex_lock(&s_record_mutex);
    cap = record_thread_info.capture;
    pthread_mutex_unlock(&s_record_mutex);

    if (!cap) {
      record_thread_deactivate();
      atomic_store(&is_record_idle, true);
      atomic_store(&is_record_active, false);
      break;
    }


    while (atomic_load(&is_record_active)) {
      atomic_store(&is_record_idle, false);

      snd_pcm_sframes_t n = snd_pcm_readi(cap, buf, (snd_pcm_uframes_t)kPeriodFrames);

      if (n < 0) {
        int ret = i2s_recover(cap, (int)n, "capture");
        if (ret < 0) {
          printf("Capture failed: %s\n", snd_strerror(ret));
        }
        continue;
      }

      int32_t *in = (int32_t *)buf;
      int16_t out[kPeriodFrames];
      const int32_t gain = 4;

      for (snd_pcm_sframes_t i = 0; i < n; i++) {
        int32_t left = in[2 * i];
        int32_t s = (left >> 16);
        s *= gain;
        out[i] = clamp16(s);
      }

      rb_push((const uint8_t *)out, (uint32_t)(kPeriodFrames * sizeof(int16_t)));
      memset(buf, 0, buf_bytes);
    }
    atomic_store(&is_record_idle, true);
    nanosleep(&ts, NULL);
  }

  printf("Record thread ending...\n");
  return NULL;
}

static void playback_thread_deactivate()
{
  printf("deactivating playback thread\n");
  pthread_mutex_lock(&s_playback_mutex);

  if (playback_thread_info.playback) {
    snd_pcm_drop(playback_thread_info.playback);
    snd_pcm_close(playback_thread_info.playback);
    playback_thread_info.playback = NULL;
  }
  if (playback_thread_info.data) {
    free(playback_thread_info.data);
    playback_thread_info.data = NULL;
  }
  free(playback_thread_info.buf);
  playback_thread_info.buf = NULL;

  playback_thread_info.buf_size_bytes = 0;
  playback_thread_info.bytes_per_frame = 0;
  pthread_mutex_unlock(&s_playback_mutex);
}

static void *playback_thread_func(void *arg)
{
  (void)arg;

  size_t cursor = 0;

  while (atomic_load(&is_playback_thread_running)) {
    while (atomic_load(&is_playback_active)) {
      atomic_store(&is_playback_idle, false);
      snd_pcm_t *pb;
      uint8_t *data;
      size_t data_len;
      uint8_t *buf;
      size_t buf_bytes, bpf;

      pthread_mutex_lock(&s_playback_mutex);
      pb = playback_thread_info.playback;
      data = playback_thread_info.data;
      data_len = playback_thread_info.data_len;
      buf = playback_thread_info.buf;
      buf_bytes = playback_thread_info.buf_size_bytes;
      bpf = playback_thread_info.bytes_per_frame;
      pthread_mutex_unlock(&s_playback_mutex);

      if (!pb || !data || !buf || (bpf == 0) || (buf_bytes == 0) || (data_len == 0)) {
        playback_thread_deactivate();
        atomic_store(&is_playback_idle, true);
        atomic_store(&is_playback_active, false);
        break;
      }

      size_t remaining = (cursor < data_len) ? (data_len - cursor) : 0;
      if (remaining == 0) {
        // finished
        playback_thread_deactivate();
        atomic_store(&is_playback_idle, true);
        atomic_store(&is_playback_active, false);
        break;
      }

      size_t retrieved = (remaining < buf_bytes) ? remaining : buf_bytes;
      memcpy(buf, data + cursor, retrieved);
      cursor += retrieved;

      snd_pcm_sframes_t frames = (snd_pcm_sframes_t)(retrieved / bpf);
      if (frames == 0) {
        playback_thread_deactivate();
        atomic_store(&is_playback_idle, true);
        atomic_store(&is_playback_active, false);
        break;
      }

      snd_pcm_sframes_t written = 0;

      while (written < (snd_pcm_sframes_t)frames) {
        snd_pcm_sframes_t n = snd_pcm_writei(pb, buf + written * bpf, frames - written);
        if (n < 0) {
          int ret = i2s_recover(pb, (int)n, "playback");
          if (ret < 0) {
            printf("playback failed: %s\n", snd_strerror(ret));
            playback_thread_deactivate();
            atomic_store(&is_playback_idle, true);
            atomic_store(&is_playback_active, false);
          }
          break;
        }
        written += n;
      }
    }
    atomic_store(&is_playback_idle, true);
    nanosleep(&ts, NULL);
  }

  printf("exiting playback thread\n");
  return NULL;
}

StatusCode i2s_init()
{
  atomic_store(&is_playback_thread_running, true);
  int threadRet = pthread_create(&playback_thread, NULL, playback_thread_func, NULL);
  if (threadRet != 0) {
    atomic_store(&is_playback_thread_running, false);
    return STATUS_CODE_THREAD_FAILURE;
  }

  atomic_store(&is_record_thread_running, true);
  threadRet = pthread_create(&record_thread, NULL, record_thread_func, NULL);
  if (threadRet != 0) {
    atomic_store(&is_record_thread_running, false);
    return STATUS_CODE_THREAD_FAILURE;
  }

  return STATUS_CODE_OK;
}

static inline playback_deinit() {
  pthread_mutex_lock(&s_playback_mutex);
  if (atomic_load(&is_playback_active)) {
    printf("stopping playback active\n");
    atomic_store(&is_playback_active, false);
    if (playback_thread_info.playback) {
      snd_pcm_drop(playback_thread_info.playback);
    }
  }
  pthread_mutex_unlock(&s_playback_mutex);

  while (!atomic_load(&is_playback_idle)) {
    sched_yield();
  }

  playback_thread_deactivate();

  atomic_store(&is_playback_thread_running, false);
  pthread_join(playback_thread, NULL);
}

static inline record_deinit() {
  pthread_mutex_lock(&s_record_mutex);
  if (atomic_load(&is_record_active)) {
    printf("stopping record active\n");
    atomic_store(&is_record_active, false);
    if (record_thread_info.capture) {
      snd_pcm_drop(record_thread_info.capture);
    }
  }

  pthread_mutex_unlock(&s_record_mutex);

  while (!atomic_load(&is_record_idle)) {
    sched_yield();
  }

  record_thread_deactivate();

  atomic_store(&is_record_thread_running, false);
  pthread_join(record_thread, NULL);
}

StatusCode i2s_deinit()
{
  playback_deinit();
  record_deinit();
  printf("Deinitializing\n");
  return STATUS_CODE_OK;
}

StatusCode i2s_start_recording()
{
  printf("i2s record\n");

  pthread_mutex_lock(&s_record_mutex);

  record_thread_info.capture = NULL;
  int ret = i2s_open_config(&record_thread_info.capture, MIC_DEV, SND_PCM_STREAM_CAPTURE, kRate, rec_kCh, rec_kFmt, kPeriodFrames);
  pthread_mutex_unlock(&s_record_mutex);

  if (ret < 0) {
    return STATUS_CODE_FAILED;
  }

  atomic_store(&is_record_active, true);
  return STATUS_CODE_OK;
}

StatusCode i2s_record_to_file(const char *dir_path, double seconds)
{
  snd_pcm_t *capture = NULL;
  int ret = i2s_open_config(&capture, MIC_DEV, SND_PCM_STREAM_CAPTURE, kRate, rec_kCh, rec_kFmt, kPeriodFrames);

  if (ret < 0) {
    return STATUS_CODE_FAILED;
  }

  time_t now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);

  char filepath[512];
  int n = snprintf(filepath, sizeof(filepath),
                   "%s/rec_%04d-%02d-%02d_%02d-%02d-%02d.pcm",
                   dir_path,
                   tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                   tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

  if ((n < 0) || ((size_t)n >= sizeof(filepath))) {
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  FILE *f = fopen(filepath, "wb");
  if (!f) {
    printf("File open error\n");
    snd_pcm_close(capture);
    return STATUS_CODE_FAILED;
  }

  const size_t bytes_per_sample = snd_pcm_format_physical_width(rec_kFmt) / 8;
  const size_t bytes_per_frame = rec_kCh * bytes_per_sample;
  const size_t buf_bytes = bytes_per_frame * kPeriodFrames;

  uint8_t *buf = (uint8_t *)malloc(buf_bytes);
  if (buf == NULL) {
    printf("malloc failed");
    fclose(f);
    snd_pcm_close(capture);
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  const uint64_t total_frames = (uint64_t)(seconds * (double)kRate);
  uint64_t frames_done = 0;

  printf("Recording audio: %.2fs -> %s\n", seconds, filepath);

  while (frames_done < total_frames) {
    snd_pcm_uframes_t wanted = kPeriodFrames;
    uint64_t remaining = total_frames - frames_done;

    if (remaining < wanted) {
      wanted = (snd_pcm_uframes_t)remaining;
    }

    snd_pcm_sframes_t n = snd_pcm_readi(capture, buf, wanted);

    if (n < 0) {
      ret = i2s_recover(capture, (int)n, "capture");
      if (ret < 0) {
        printf("Capture failed: %s\n", snd_strerror(ret));
      }
      continue;
    }

    int32_t *in = (int32_t *)buf;
    int16_t out[kPeriodFrames];
    const int32_t gain = 4;

    for (snd_pcm_sframes_t i = 0; i < n; i++) {
      int32_t left = in[2 * i];
      int32_t s = (left >> 16);
      s *= gain;
      out[i] = clamp16(s);
    }

    size_t out_bytes = (size_t)n * sizeof(out[0]);
    if (fwrite(out, 1, out_bytes, f) != out_bytes) {
      printf("file write error\n");
      break;
    }

    frames_done += (uint64_t)n;
  }

  free(buf);
  fclose(f);
  snd_pcm_close(capture);
  return STATUS_CODE_OK;
}

StatusCode i2s_play_file(const char *path)
{
  if (!path) {
    return STATUS_CODE_INVALID_ARGS;
  }

  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("File open error\n");
    return STATUS_CODE_FAILED;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return STATUS_CODE_FAILED;
  }

  long data_size = ftell(f);
  if (data_size <= 0) {
    fclose(f);
    return STATUS_CODE_FAILED;
  }
  rewind(f);

  uint8_t *data = (uint8_t *)malloc((size_t)data_size);
  if (!data) {
    fclose(f);
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  size_t got = fread(data, 1, (size_t)data_size, f);
  fclose(f);
  if (got != (size_t)data_size) {
    free(data);
    return STATUS_CODE_FAILED;
  }

  return i2s_play_raw(data, data_size);
}

StatusCode i2s_play_raw(const uint8_t *data, const size_t data_size)
{
  pthread_mutex_lock(&s_playback_mutex);
  if (atomic_load(&is_playback_active)) {
    atomic_store(&is_playback_active, false);
    pthread_mutex_unlock(&s_playback_mutex);

    while (!atomic_load(&is_playback_idle)) {
      sched_yield();
    }

    playback_thread_deactivate();
    pthread_mutex_lock(&s_playback_mutex);
  }

  playback_thread_info.playback = NULL;
  int ret = i2s_open_config(&playback_thread_info.playback, SPEAKER_DEV, SND_PCM_STREAM_PLAYBACK, kRate, pb_kCh, pb_kFmt, kPeriodFrames);
  if (ret < 0) {
    pthread_mutex_unlock(&s_playback_mutex);
    return STATUS_CODE_FAILED;
  }

  free(playback_thread_info.data);
  playback_thread_info.data = data;
  playback_thread_info.data_len = (size_t)data_size;

  const size_t bytes_per_sample = snd_pcm_format_physical_width(pb_kFmt) / 8;
  const size_t bytes_per_frame = pb_kCh * bytes_per_sample;
  const size_t buf_bytes = bytes_per_frame * kPeriodFrames;

  playback_thread_info.buf = (uint8_t *)malloc(buf_bytes);
  if (playback_thread_info.buf == NULL) {
    printf("malloc failed");
    snd_pcm_close(playback_thread_info.playback);
    pthread_mutex_unlock(&s_playback_mutex);
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  playback_thread_info.buf_size_bytes = buf_bytes;
  playback_thread_info.bytes_per_frame = bytes_per_frame;
  pthread_mutex_unlock(&s_playback_mutex);

  printf("Playing raw PCM: (rate=%u ch=%u fmt=%s)\n", kRate, pb_kCh, snd_pcm_format_name(pb_kFmt));

  // start thread
  atomic_store(&is_playback_active, true);
  return STATUS_CODE_OK;
}
