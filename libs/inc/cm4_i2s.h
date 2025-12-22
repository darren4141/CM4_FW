#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "global_enums.h"

#define I2_WAITING_PERIOD_S 1 / 10


StatusCode pcm_init();
StatusCode pcm_deinit();

/**
 * Note: records in S32_LE but converts and saves audio file as S16_LE
 */
StatusCode pcm_record(const char *path, double seconds);
StatusCode pcm_play(const char *path);