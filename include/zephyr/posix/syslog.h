/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for system error logging.
 * @ingroup posix
 *
 * Provides the system logging functions along with the standard option,
 * facility, and priority constants used to submit prioritized messages to
 * the system log.
 *
 * @posix_header{syslog.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYSLOG_H_
#define ZEPHYR_INCLUDE_POSIX_SYSLOG_H_

#include <stdarg.h>

/* option */

/**
 * @brief Include the process ID in each log message.
 */
#define LOG_PID    1

/**
 * @brief Log to the system console if the logger is unavailable.
 */
#define LOG_CONS   2

/**
 * @brief Open the connection to the logger immediately.
 */
#define LOG_NDELAY 4

/**
 * @brief Delay the connection until the first message is sent.
 */
#define LOG_ODELAY 8

/**
 * @brief Do not wait for child processes created by logging.
 */
#define LOG_NOWAIT 16

/**
 * @brief Also write messages to stderr.
 */
#define LOG_PERROR 32

/* facility */

/**
 * @brief Kernel messages.
 */
#define LOG_KERN   0

/**
 * @brief Generic user-level messages.
 */
#define LOG_USER   1

/**
 * @brief Mail system messages.
 */
#define LOG_MAIL   2

/**
 * @brief News subsystem messages.
 */
#define LOG_NEWS   3

/**
 * @brief UUCP subsystem messages.
 */
#define LOG_UUCP   4

/**
 * @brief System daemon messages.
 */
#define LOG_DAEMON 5

/**
 * @brief Security/authorization messages.
 */
#define LOG_AUTH   6

/**
 * @brief Clock daemon messages.
 */
#define LOG_CRON   7

/**
 * @brief Printer subsystem messages.
 */
#define LOG_LPR    8

/**
 * @brief Reserved for local use (facility 0).
 */
#define LOG_LOCAL0 9

/**
 * @brief Reserved for local use (facility 1).
 */
#define LOG_LOCAL1 10

/**
 * @brief Reserved for local use (facility 2).
 */
#define LOG_LOCAL2 11

/**
 * @brief Reserved for local use (facility 3).
 */
#define LOG_LOCAL3 12

/**
 * @brief Reserved for local use (facility 4).
 */
#define LOG_LOCAL4 13

/**
 * @brief Reserved for local use (facility 5).
 */
#define LOG_LOCAL5 14

/**
 * @brief Reserved for local use (facility 6).
 */
#define LOG_LOCAL6 15

/**
 * @brief Reserved for local use (facility 7).
 */
#define LOG_LOCAL7 16

/* priority */

/**
 * @brief A panic condition was reported to all processes.
 */
#define LOG_EMERG   0

/**
 * @brief A condition that should be corrected immediately.
 */
#define LOG_ALERT   1

/**
 * @brief A critical condition.
 */
#define LOG_CRIT    2

/**
 * @brief An error message.
 */
#define LOG_ERR     3

/**
 * @brief A warning message.
 */
#define LOG_WARNING 4

/**
 * @brief A condition requiring special handling.
 */
#define LOG_NOTICE  5

/**
 * @brief A general information message.
 */
#define LOG_INFO    6

/**
 * @brief A message useful for debugging programs.
 */
#define LOG_DEBUG   7

/* generate a valid log mask */

/**
 * @brief Generate a log mask for the given priority.
 */
#define LOG_MASK(mask) ((mask) & BIT_MASK(LOG_DEBUG + 1))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Close the connection to the system logger.
 *
 * @posix_func{closelog}
 */
void closelog(void);

/**
 * @brief Open a connection to the system logger.
 *
 * @param ident    String prepended to each log message; typically the program name.
 * @param logopt   Bitwise OR of logging options (@c LOG_PID, @c LOG_CONS, etc.).
 * @param facility Default facility code (@c LOG_USER, @c LOG_DAEMON, etc.).
 *
 * @posix_func{openlog}
 */
void openlog(const char *ident, int logopt, int facility);

/**
 * @brief Set the log priority mask.
 *
 * @param maskpri New priority mask (generated with LOG_MASK()).
 *
 * @return The previous log priority mask.
 *
 * @posix_func{setlogmask}
 */
int setlogmask(int maskpri);

/**
 * @brief Write a message to the system logger.
 *
 * @param priority Priority code (@c LOG_EMERG through @c LOG_DEBUG).
 * @param message  printf()-style format string.
 * @param ...      Format arguments.
 *
 * @posix_func{syslog}
 */
void syslog(int priority, const char *message, ...);

/**
 * @brief Generate a system log message from a variable argument list.
 *
 * @param priority Priority code (@c LOG_EMERG through @c LOG_DEBUG).
 * @param format   printf()-style format string.
 * @param ap       Variable argument list.
 */
void vsyslog(int priority, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYSLOG_H_ */
