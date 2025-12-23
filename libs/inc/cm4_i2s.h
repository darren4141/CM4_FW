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

#include "global_enums.h"

#define I2_WAITING_PERIOD_S 1 / 10

/**
 * Initialize i2s bus
 */
StatusCode i2s_init();

/**
 * Deinitialize i2s, join all threads - recommended on program termination
 */
StatusCode i2s_deinit();

/**
 * Start the recording thread (non-blocking), records in S32_LE but converts and saves audio file as S16_LE
 */
StatusCode i2s_start_recording();

/**
 * Record mic input to a file for a specified amount of time - BLOCKING
 */
StatusCode i2s_record_to_file(const char *path, double seconds);

/**
 * Play audio from a file using the playback thread
 */
StatusCode i2s_play_file(const char *path);

/**
 * Play audio from a raw data stream using the playback thread
 */
StatusCode i2s_play_raw(const uint8_t *data, const size_t data_size);

/**
 * Pop a microphone reading from the ring buffer
 */
int i2s_rb_pop(uint8_t *out, uint32_t len);