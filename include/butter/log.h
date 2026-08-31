#ifndef BUTTER_LOG_H
#define BUTTER_LOG_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Log a message.
 * @details Logs a message with the given level and format string, and keeps
 * track of the last log message to prevent duplicate logs.
 *
 * @param level The log level.
 * @param fmt The format string.
 * @param ... The format arguments.
 *
 * @pre
 * - @c level must be a valid log level, see @ref butter_log_level_t for more
 * info.
 * - @c fmt must be a valid C-string.
 * - @c ... must be a valid format arguments.
 */
void butter_log(butter_log_level_t level, const cstr *fmt, ...);

//
//
//

/************************************************************************/
/** Convenience macros for logging messages with preapplied log level. **/
/************************************************************************/

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
