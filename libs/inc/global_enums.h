#pragma once

#define KHZ(x) ((x) * 1000U)
#define MHZ(x) ((x) * 1000000U)
#define GHZ(x) ((x) * 1000000000U)

typedef enum {
  STATUS_CODE_OK = 0,
  STATUS_CODE_INVALID_ARGS = -1,
  STATUS_CODE_NOT_INITIALIZED = -2,
  STATUS_CODE_ALREADY_INITIALIZED = -3,
  STATUS_CODE_THREAD_FAILURE = -4,
  STATUS_CODE_MEM_ACCESS_FAILURE = -5,
  STATUS_CODE_FAILED = -6,
} StatusCode;

#define TRY(expr)                                                              \
  do {                                                                         \
    StatusCode _s = (expr);                                                    \
    if (_s != STATUS_CODE_OK) {                                                \
      printf("%s failed with status code: %d\n", #expr, _s);                   \
      return _s;                                                               \
    }                                                                          \
  } while (0)

#define PERIPH_BASE 0xFE000000UL
#define CORE_FREQ_HZ GHZ(1.5)