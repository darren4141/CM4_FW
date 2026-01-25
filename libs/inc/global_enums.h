#pragma once

#define KHZ(x)            ((x) * 1000U)
#define MHZ(x)            ((x) * 1000000U)
#define GHZ(x)            ((x) * 1000000000U)

typedef enum {
  STATUS_CODE_OK                  = 0,
  STATUS_CODE_INVALID_ARGS        = -1,
  STATUS_CODE_NOT_INITIALIZED     = -2,
  STATUS_CODE_ALREADY_INITIALIZED = -3,
  STATUS_CODE_TIMEOUT             = -4,
  STATUS_CODE_UNIMPLEMENTED       = -5,
  STATUS_CODE_THREAD_FAILURE      = -6,
  STATUS_CODE_MEM_ACCESS_FAILURE  = -7,
  STATUS_CODE_OUT_OF_MEMORY       = -8,
  STATUS_CODE_FAILED              = -9,
} StatusCode;

typedef enum {
  VERBOSITY_NONE    = 0,
  VERBOSITY_LEVEL_1 = 1,
  VERBOSITY_LEVEL_2 = 2,
  VERBOSITY_LEVEL_3 = 3,
} VerbosityLevel;

#define VERB_PRINTF(level, ...)                                                \
        do {                                                                         \
          if (verbosity == (level)) {                                                \
            printf(__VA_ARGS__);                                                     \
          }                                                                          \
        } while (0)

#define VERB1_PRINTF(...) VERB_PRINTF(VERBOSITY_LEVEL_1, __VA_ARGS__)
#define VERB2_PRINTF(...) VERB_PRINTF(VERBOSITY_LEVEL_2, __VA_ARGS__)
#define VERB3_PRINTF(...) VERB_PRINTF(VERBOSITY_LEVEL_3, __VA_ARGS__)

#define TRY(expr)                                                              \
        do {                                                                         \
          StatusCode _s = (expr);                                                    \
          if (_s != STATUS_CODE_OK) {                                                \
            printf("%s failed with status code: %d\n", #expr, _s);                   \
            return _s;                                                               \
          }                                                                          \
        } while (0)

#define PERIPH_BASE  0xFE000000UL
#define CORE_FREQ_HZ GHZ(1.5)