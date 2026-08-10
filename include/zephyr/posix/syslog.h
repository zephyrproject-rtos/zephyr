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

#define LOG_PID    1  /**< Include the process ID in each log message. */

#define LOG_CONS   2  /**< Log to the system console if the logger is unavailable. */

#define LOG_NDELAY 4  /**< Open the connection to the logger immediately. */

#define LOG_ODELAY 8  /**< Delay the connection until the first message is sent. */

#define LOG_NOWAIT 16 /**< Do not wait for child processes created by logging. */

#define LOG_PERROR 32 /**< Also write messages to stderr. */

/* facility */

#define LOG_KERN   0  /**< Kernel messages. */

#define LOG_USER   1  /**< Generic user-level messages. */

#define LOG_MAIL   2  /**< Mail system messages. */

#define LOG_NEWS   3  /**< News subsystem messages. */

#define LOG_UUCP   4  /**< UUCP subsystem messages. */

#define LOG_DAEMON 5  /**< System daemon messages. */

#define LOG_AUTH   6  /**< Security/authorization messages. */

#define LOG_CRON   7  /**< Clock daemon messages. */

#define LOG_LPR    8  /**< Printer subsystem messages. */

#define LOG_LOCAL0 9  /**< Reserved for local use (facility 0). */

#define LOG_LOCAL1 10 /**< Reserved for local use (facility 1). */

#define LOG_LOCAL2 11 /**< Reserved for local use (facility 2). */

#define LOG_LOCAL3 12 /**< Reserved for local use (facility 3). */

#define LOG_LOCAL4 13 /**< Reserved for local use (facility 4). */

#define LOG_LOCAL5 14 /**< Reserved for local use (facility 5). */

#define LOG_LOCAL6 15 /**< Reserved for local use (facility 6). */

#define LOG_LOCAL7 16 /**< Reserved for local use (facility 7). */

/* priority */

#define LOG_EMERG   0 /**< A panic condition was reported to all processes. */

#define LOG_ALERT   1 /**< A condition that should be corrected immediately. */

#define LOG_CRIT    2 /**< A critical condition. */

#define LOG_ERR     3 /**< An error message. */

#define LOG_WARNING 4 /**< A warning message. */

#define LOG_NOTICE  5 /**< A condition requiring special handling. */

#define LOG_INFO    6 /**< A general information message. */

#define LOG_DEBUG   7 /**< A message useful for debugging programs. */

/* generate a valid log mask */

/**
 * @brief Generate a log mask covering priorities up to and including @p mask.
 *
 * @param mask Priority value (@c LOG_EMERG .. @c LOG_DEBUG).
 *
 * @return Bit mask with the bits for all priorities up to and including @p mask set.
 */
#define LOG_MASK(mask) ((mask) & BIT_MASK(LOG_DEBUG + 1))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Close the connection to the system logger.
 *
 * @posix_api{XSI_SYSTEM_LOGGING,closelog}
 */
void closelog(void);

/**
 * @brief Open a connection to the system logger.
 *
 * @param ident    String prepended to each log message; typically the program name.
 * @param logopt   Bitwise OR of logging options (@c LOG_PID, @c LOG_CONS, etc.).
 * @param facility Default facility code (@c LOG_USER, @c LOG_DAEMON, etc.).
 *
 * @posix_api{XSI_SYSTEM_LOGGING,openlog}
 */
void openlog(const char *ident, int logopt, int facility);

/**
 * @brief Set the log priority mask.
 *
 * @param maskpri New priority mask (generated with LOG_MASK()).
 *
 * @return The previous log priority mask.
 *
 * @posix_api{XSI_SYSTEM_LOGGING,setlogmask}
 */
int setlogmask(int maskpri);

/**
 * @brief Write a message to the system logger.
 *
 * @param priority Priority code (@c LOG_EMERG through @c LOG_DEBUG).
 * @param message  printf()-style format string.
 * @param ...      Format arguments.
 *
 * @posix_api{XSI_SYSTEM_LOGGING,syslog}
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
