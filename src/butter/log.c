#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

#define COLOR_RESET "\x1b[0m"
#define COLOR_DARK_RED "\x1b[31m"
#define COLOR_RED "\x1b[91m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"

static thread_local cstr last_level[20] = {0};
static thread_local cstr last_msg[4096] = {0};
static thread_local u64 repeat_count = 0;
static thread_local b32 has_printed = false;

void butter_log(butter_log_level_t level, const cstr *fmt, ...) {
#ifndef BUTTER_DEBUG
  if (level == BUTTER_LOG_DEBUG)
    return;
#endif

  static cstr level_str[20] = {0};
  static cstr fmt_str[4096] = {0};

  switch (level) {
  case BUTTER_LOG_DEBUG:
    snprintf(level_str, 20, "%s[DEBUG]%s", COLOR_CYAN, COLOR_RESET);
    break;
  case BUTTER_LOG_INFO:
    snprintf(level_str, 20, "%s[INFO]%s", COLOR_CYAN, COLOR_RESET);
    break;
  case BUTTER_LOG_WARNING:
    snprintf(level_str, 20, "%s[WARN]%s", COLOR_YELLOW, COLOR_RESET);
    break;
  case BUTTER_LOG_ERROR:
    snprintf(level_str, 20, "%s[ERROR]%s", COLOR_RED, COLOR_RESET);
    break;
  case BUTTER_LOG_FATAL:
    snprintf(level_str, 20, "%s[FATAL]%s", COLOR_DARK_RED, COLOR_RESET);
    break;
  }

  va_list args;
  va_start(args, fmt);
  vsnprintf(fmt_str, 4096, fmt, args);
  va_end(args);

  if (memcmp(last_level, level_str, 20) == 0 &&
      memcmp(last_msg, fmt_str, 4096) == 0) {
    repeat_count++;
    fprintf(stderr, "\r[BUTTER] %s: %s [x%lu]", level_str, fmt_str,
            repeat_count + 1);
    fflush(stderr);
    has_printed = true;
    return;
  }

  if (has_printed) {
    fprintf(stderr, "\n");
    fflush(stderr);
    has_printed = false;
  }

  fprintf(stderr, "[BUTTER] %s: %s\n", level_str, fmt_str);
  memcpy(last_level, level_str, 20);
  memcpy(last_msg, fmt_str, 4096);
}
