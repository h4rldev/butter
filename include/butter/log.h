#ifndef BUTTER_LOG_H
#define BUTTER_LOG_H

#include <htils/basictypes.h>

#include <butter/types.h>

void butter_log(butter_log_level_t level, const cstr *fmt, ...);

#define butter_log_debug(fmt, ...)                                             \
  butter_log(BUTTER_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define butter_log_info(fmt, ...)                                              \
  butter_log(BUTTER_LOG_INFO, fmt, ##__VA_ARGS__)
#define butter_log_warning(fmt, ...)                                           \
  butter_log(BUTTER_LOG_WARNING, fmt, ##__VA_ARGS__)
#define butter_log_error(fmt, ...)                                             \
  butter_log(BUTTER_LOG_ERROR, fmt, ##__VA_ARGS__)
#define butter_log_fatal(fmt, ...)                                             \
  butter_log(BUTTER_LOG_FATAL, fmt, ##__VA_ARGS__)

#endif // !BUTTER_LOG_H
