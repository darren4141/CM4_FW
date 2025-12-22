#include "cm4_i2s.h"

#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>

#define MIC_DEV     "plughw:2,0"
#define SPEAKER_DEV "plughw:2,1"

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
static void *playback_thread_func(void *arg);
static pthread_mutex_t s_playback_mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool is_playback_thread_running = false;
static atomic_bool is_playback_active = false;
static atomic_bool is_playback_idle = true;

typedef struct {
  snd_pcm_t *playback;
  FILE *file;
  uint8_t *buf;
  size_t buf_size_bytes;
  size_t bytes_per_frame;
} PlaybackThreadInfo_s;

static PlaybackThreadInfo_s playback_thread_info;

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

static int pcm_recover(snd_pcm_t *h, int ret, const char *tag)
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

static int pcm_open_config(snd_pcm_t **h, const char *dev, snd_pcm_stream_t stream,
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

static void playback_thread_deactivate()
{
  printf("deactivating playback thread\n");
  pthread_mutex_lock(&s_playback_mutex);

  if (playback_thread_info.playback) {
    snd_pcm_drop(playback_thread_info.playback);
    snd_pcm_close(playback_thread_info.playback);
    playback_thread_info.playback = NULL;
  }
  if (playback_thread_info.file) {
    fclose(playback_thread_info.file);
    playback_thread_info.file = NULL;
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

  while (atomic_load(&is_playback_thread_running)) {
    while (atomic_load(&is_playback_active)) {
      atomic_store(&is_playback_idle, false);
      snd_pcm_t *pb;
      FILE *f;
      uint8_t *buf;
      size_t buf_bytes, bpf;

      pthread_mutex_lock(&s_playback_mutex);
      pb = playback_thread_info.playback;
      f = playback_thread_info.file;
      buf = playback_thread_info.buf;
      buf_bytes = playback_thread_info.buf_size_bytes;
      bpf = playback_thread_info.bytes_per_frame;
      pthread_mutex_unlock(&s_playback_mutex);


      if (!pb || !f || !buf || (bpf == 0) || (buf_bytes == 0)) {
        playback_thread_deactivate();
        atomic_store(&is_playback_idle, true);
        atomic_store(&is_playback_active, false);
        break;
      }


      size_t retrieved = fread(buf, 1, buf_bytes, f);
      if (retrieved == 0) {
        playback_thread_deactivate();
        atomic_store(&is_playback_idle, true);
        atomic_store(&is_playback_active, false);
        break;
      }

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
          int ret = pcm_recover(pb, (int)n, "playback");
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

StatusCode pcm_init()
{
  atomic_store(&is_playback_thread_running, true);
  int threadRet = pthread_create(&playback_thread, NULL, playback_thread_func, NULL);
  if (threadRet != 0) {
    atomic_store(&is_playback_thread_running, false);
    return STATUS_CODE_THREAD_FAILURE;
  }
  return STATUS_CODE_OK;
}

StatusCode pcm_deinit()
{
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

  printf("Deinitializing\n");
  return STATUS_CODE_OK;
}

StatusCode pcm_record(const char *dir_path, double seconds)
{
  snd_pcm_t *capture = NULL;
  int ret = pcm_open_config(&capture, MIC_DEV, SND_PCM_STREAM_CAPTURE, kRate, rec_kCh, rec_kFmt, kPeriodFrames);

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
      ret = pcm_recover(capture, (int)n, "capture");
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

StatusCode pcm_play(const char *path)
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
  int ret = pcm_open_config(&playback_thread_info.playback, SPEAKER_DEV, SND_PCM_STREAM_PLAYBACK, kRate, pb_kCh, pb_kFmt, kPeriodFrames);
  if (ret < 0) {
    pthread_mutex_unlock(&s_playback_mutex);
    return STATUS_CODE_FAILED;
  }

  playback_thread_info.file = fopen(path, "rb");
  if (!playback_thread_info.file) {
    printf("File open error\n");
    snd_pcm_close(playback_thread_info.playback);
    pthread_mutex_unlock(&s_playback_mutex);
    return STATUS_CODE_FAILED;
  }

  const size_t bytes_per_sample = snd_pcm_format_physical_width(pb_kFmt) / 8;
  const size_t bytes_per_frame = pb_kCh * bytes_per_sample;
  const size_t buf_bytes = bytes_per_frame * kPeriodFrames;

  playback_thread_info.buf = (uint8_t *)malloc(buf_bytes);
  if (playback_thread_info.buf == NULL) {
    printf("malloc failed");
    fclose(playback_thread_info.file);
    snd_pcm_close(playback_thread_info.playback);
    pthread_mutex_unlock(&s_playback_mutex);
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  playback_thread_info.buf_size_bytes = buf_bytes;
  playback_thread_info.bytes_per_frame = bytes_per_frame;
  pthread_mutex_unlock(&s_playback_mutex);

  printf("Playing raw PCM: %s (rate=%u ch=%u fmt=%s)\n",
         path, kRate, pb_kCh, snd_pcm_format_name(pb_kFmt));

  // start thread
  atomic_store(&is_playback_active, true);
  return STATUS_CODE_OK;
}