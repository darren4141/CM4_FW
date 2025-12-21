#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "global_enums.h"

StatusCode pcm_record(const char *path, double seconds);
StatusCode pcm_play(const char *path);