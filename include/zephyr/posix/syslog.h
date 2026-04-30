/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SYSLOG_H_
#define ZEPHYR_INCLUDE_POSIX_SYSLOG_H_

#include <stdarg.h>

/* option */
#define LOG_PID    1
#define LOG_CONS   2
#define LOG_NDELAY 4
#define LOG_ODELAY 8
#define LOG_NOWAIT 16
#define LOG_PERROR 32

/* facility */
#define LOG_KERN   0
#define LOG_USER   1
#define LOG_MAIL   2
#define LOG_NEWS   3
#define LOG_UUCP   4
#define LOG_DAEMON 5
#define LOG_AUTH   6
#define LOG_CRON   7
#define LOG_LPR    8
#define LOG_LOCAL0 9
#define LOG_LOCAL1 10
#define LOG_LOCAL2 11
#define LOG_LOCAL3 12
#define LOG_LOCAL4 13
#define LOG_LOCAL5 14
#define LOG_LOCAL6 15
#define LOG_LOCAL7 16

/* priority */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#ifndef LOG_ERR
#define LOG_ERR     3
#else
#ifndef DONT_WARN_ME_ABOUT_SYSLOG
#warning \
"syslog.h and log.h are not compatibly. "\
"When including both syslog.h and log.h, by default log.h prevails and it is not possible to use "\
"syslog(LOG_ERR,..). If you want to use syslog(LOG_ERR,..) avoid including log.h, "\
"OR #undef LOG_ERR before including syslog.h. "\
"To simply suppress this warning define DONT_WARN_ME_ABOUT_SYSLOG"
#endif
#endif /* ifndef LOG_ERR */
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

/* generate a valid log mask */
#define LOG_MASK(mask) ((mask) & BIT_MASK(LOG_DEBUG + 1))

#ifdef __cplusplus
extern "C" {
#endif

void closelog(void);
void openlog(const char *ident, int logopt, int facility);
int setlogmask(int maskpri);
void syslog(int priority, const char *message, ...);
void vsyslog(int priority, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYSLOG_H_ */
