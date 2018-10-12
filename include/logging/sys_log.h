/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file sys_log.h
 *  @brief Logging macros.
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_SYS_LOG_H_
#define ZEPHYR_INCLUDE_LOGGING_SYS_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_LOG_LEVEL_OFF 0
#define SYS_LOG_LEVEL_ERROR 1
#define SYS_LOG_LEVEL_WARNING 2
#define SYS_LOG_LEVEL_INFO 3
#define SYS_LOG_LEVEL_DEBUG 4

/* Determine this compile unit log level */
#if !defined(SYS_LOG_LEVEL)
/* Use default */
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#elif (SYS_LOG_LEVEL < CONFIG_SYS_LOG_OVERRIDE_LEVEL)
/* Use override */
#undef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_OVERRIDE_LEVEL
#endif

/**
 * @brief System Log
 * @defgroup system_log System Log
 * @{
 */
#if defined(CONFIG_SYS_LOG) && (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF)

extern void (*syslog_hook)(const char *fmt, ...);
void syslog_hook_install(void (*hook)(const char *, ...));

/* decide print func */
#if defined(CONFIG_SYS_LOG_EXT_HOOK)
#define SYS_LOG_BACKEND_FN syslog_hook
#else
#include <misc/printk.h>
#define SYS_LOG_BACKEND_FN printk
#endif

/* Should use color? */
#if defined(CONFIG_SYS_LOG_SHOW_COLOR)
#define SYS_LOG_COLOR_OFF     "\x1B[0m"
#define SYS_LOG_COLOR_RED     "\x1B[0;31m"
#define SYS_LOG_COLOR_YELLOW  "\x1B[0;33m"
#else
#define SYS_LOG_COLOR_OFF     ""
#define SYS_LOG_COLOR_RED     ""
#define SYS_LOG_COLOR_YELLOW  ""
#endif /* CONFIG_SYS_LOG_SHOW_COLOR */

/* Should use log lv tags? */
#if defined(CONFIG_SYS_LOG_SHOW_TAGS)
#define SYS_LOG_TAG_ERR " [ERR]"
#define SYS_LOG_TAG_WRN " [WRN]"
#define SYS_LOG_TAG_INF " [INF]"
#define SYS_LOG_TAG_DBG " [DBG]"
#else
#define SYS_LOG_TAG_ERR ""
#define SYS_LOG_TAG_WRN ""
#define SYS_LOG_TAG_INF ""
#define SYS_LOG_TAG_DBG ""
#endif /* CONFIG_SYS_LOG_SHOW_TAGS */

/* Log domain name */
#if !defined(SYS_LOG_DOMAIN)
#define SYS_LOG_DOMAIN "general"
#endif /* SYS_LOG_DOMAIN */

/**
 * @def SYS_LOG_NO_NEWLINE
 *
 * @brief Specifies whether SYS_LOG should add newline at the end of line
 * or not.
 *
 * @details User can define SYS_LOG_NO_NEWLINE no prevent the header file
 * from adding newline if the debug print already has a newline character.
 */
#if !defined(SYS_LOG_NO_NEWLINE)
#define SYS_LOG_NL "\n"
#else
#define SYS_LOG_NL ""
#endif

/* [domain] [level] function: */
#define LOG_LAYOUT "[%s]%s %s: %s"
#define LOG_BACKEND_CALL(log_lv, log_color, log_format, color_off, ...)	\
	SYS_LOG_BACKEND_FN(LOG_LAYOUT log_format "%s" SYS_LOG_NL,	\
	SYS_LOG_DOMAIN, log_lv, __func__, log_color, ##__VA_ARGS__, color_off)

#define LOG_NO_COLOR(log_lv, log_format, ...)				\
	LOG_BACKEND_CALL(log_lv, "", log_format, "", ##__VA_ARGS__)
#define LOG_COLOR(log_lv, log_color, log_format, ...)			\
	LOG_BACKEND_CALL(log_lv, log_color, log_format,			\
	SYS_LOG_COLOR_OFF, ##__VA_ARGS__)

#define SYS_LOG_ERR(...) LOG_COLOR(SYS_LOG_TAG_ERR, SYS_LOG_COLOR_RED,	\
	##__VA_ARGS__)

#if (SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING)
#define SYS_LOG_WRN(...) LOG_COLOR(SYS_LOG_TAG_WRN,			\
	SYS_LOG_COLOR_YELLOW, ##__VA_ARGS__)
#endif

#if (SYS_LOG_LEVEL >= SYS_LOG_LEVEL_INFO)
#define SYS_LOG_INF(...) LOG_NO_COLOR(SYS_LOG_TAG_INF, ##__VA_ARGS__)
#endif

#if (SYS_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG)
#define SYS_LOG_DBG(...) LOG_NO_COLOR(SYS_LOG_TAG_DBG, ##__VA_ARGS__)
#endif

#else
/**
 * @def SYS_LOG_ERR
 *
 * @brief Writes an ERROR level message to the log.
 *
 * @details Lowest logging level, these messages are logged whenever sys log is
 * active. it's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define SYS_LOG_ERR(...) { ; }
#endif /* CONFIG_SYS_LOG */

/* create dummy macros */
#if !defined(SYS_LOG_WRN)
/**
 * @def SYS_LOG_WRN
 *
 * @brief Writes a WARNING level message to the log.
 *
 * @details available if SYS_LOG_LEVEL is SYS_LOG_LEVEL_WARNING or higher.
 * It's meant to register messages related to unusual situations that are
 * not necessarily errors.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define SYS_LOG_WRN(...) { ; }
#endif

#if !defined(SYS_LOG_INF)
/**
 * @def SYS_LOG_INF
 *
 * @brief Writes an INFO level message to the log.
 *
 * @details available if SYS_LOG_LEVEL is SYS_LOG_LEVEL_INFO or higher.
 * It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define SYS_LOG_INF(...) { ; }
#endif

#if !defined(SYS_LOG_DBG)
/**
 * @def SYS_LOG_DBG
 *
 * @brief Writes a DEBUG level message to the log.
 *
 * @details highest logging level, available if SYS_LOG_LEVEL is
 * SYS_LOG_LEVEL_DEBUG. It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define SYS_LOG_DBG(...) { ; }
#endif
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_SYS_LOG_H_ */
