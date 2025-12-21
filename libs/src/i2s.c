#include "cm4_i2s.h"

#include <time.h>
#include <alsa/asoundlib.h>

#define MIC_DEV     "plughw:2,0"
#define SPEAKER_DEV "plughw:2,1"

static const unsigned kRate = 48000;   // sample rate
static const unsigned pb_kCh = 1;
static const unsigned rec_kCh = 2;
static const snd_pcm_format_t pb_kFmt = SND_PCM_FORMAT_S16_LE;
static const snd_pcm_format_t rec_kFmt = SND_PCM_FORMAT_S32_LE;
static const snd_pcm_uframes_t kPeriodFrames = 1024;

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
      out[i] = (int16_t)(left >> 16);
      out[i] *= gain;
      out[i] = clamp16(out[i]);
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

  snd_pcm_t *playback = NULL;
  int ret = pcm_open_config(&playback, SPEAKER_DEV, SND_PCM_STREAM_PLAYBACK, kRate, pb_kCh, pb_kFmt, kPeriodFrames);
  if (ret < 0) {
    return STATUS_CODE_FAILED;
  }

  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("File open error\n");
    snd_pcm_close(playback);
    return STATUS_CODE_FAILED;
  }

  const size_t bytes_per_sample = snd_pcm_format_physical_width(pb_kFmt) / 8;
  const size_t bytes_per_frame = pb_kCh * bytes_per_sample;
  const size_t buf_bytes = bytes_per_frame * kPeriodFrames;


  uint8_t *buf = (uint8_t *)malloc(buf_bytes);
  if (buf == NULL) {
    printf("malloc failed");
    fclose(f);
    snd_pcm_close(playback);
    return STATUS_CODE_OUT_OF_MEMORY;
  }

  printf("Playing raw PCM: %s (rate=%u ch=%u fmt=%s)\n",
         path, kRate, pb_kCh, snd_pcm_format_name(pb_kFmt));

  while (1) {
    size_t retrieved = fread(buf, 1, buf_bytes, f);
    if (retrieved == 0) {
      break;
    }

    snd_pcm_sframes_t frames = (snd_pcm_sframes_t)(retrieved / bytes_per_frame);
    if (frames == 0) {
      break;
    }

    snd_pcm_sframes_t written = 0;

    while (written < (snd_pcm_sframes_t)frames) {
      snd_pcm_sframes_t n = snd_pcm_writei(playback, buf + written * bytes_per_frame, frames - written);
      if (n < 0) {
        ret = pcm_recover(playback, (int)n, "playback");
        if (ret < 0) {
          printf("playback failed: %s\n", snd_strerror(ret));
          free(buf);
          fclose(f);
          snd_pcm_close(playback);
          return STATUS_CODE_FAILED;
        }
        continue;
      }
      written += n;
    }
  }

  snd_pcm_drain(playback);

  free(buf);
  fclose(f);
  snd_pcm_close(playback);
  return STATUS_CODE_OK;
}