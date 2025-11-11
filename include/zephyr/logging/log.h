/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_H_

#include <zephyr/logging/log_instance.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

#if CONFIG_USERSPACE && CONFIG_LOG_ALWAYS_RUNTIME
#include <zephyr/app_memory/app_memdomain.h>
#endif

#ifdef CONFIG_LOG_RATELIMIT
#include <zephyr/kernel.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LOG_RATELIMIT
#define LOG_RATELIMIT_INTERVAL_MS CONFIG_LOG_RATELIMIT_INTERVAL_MS

#else
#define LOG_RATELIMIT_INTERVAL_MS 0

#endif

/**
 * @brief Logging
 * @defgroup logging Logging
 * @since 1.13
 * @version 1.0.0
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @brief Logger API
 * @defgroup log_api Logging API
 * @ingroup logger
 * @{
 */

/**
 * @brief Writes an ERROR level message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR(...) Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN(...) Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INF(...) Z_LOG(LOG_LEVEL_INF, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG(...) Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message to the log on the first execution only.
 *
 * @details It's meant for situations that warrant investigation but could clutter
 * the logs if output on every execution.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN_ONCE(...)                                                                          \
	do {                                                                                       \
		static atomic_t __warned;                                                          \
		if (unlikely(atomic_cas(&__warned, 0, 1))) {                                       \
			Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__);                                         \
		}                                                                                  \
	} while (0)

/**
 * @brief Core rate-limited logging macro for regular messages
 *
 * @details Internal macro that provides rate-limited logging functionality
 * for regular log messages. Uses atomic operations to ensure thread safety
 * in multi-threaded environments. Only one thread can successfully log a
 * message within the specified rate limit period.
 *
 * @param _level Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... Arguments to pass to the logging function.
 */
#define _LOG_RATELIMIT_CORE(_level, _rate_ms, ...)                                                 \
	do {                                                                                       \
		static atomic_t __last_log_time;                                                   \
		static atomic_t __skipped_count;                                                   \
		uint32_t __now = k_uptime_get_32();                                                \
		uint32_t __last = atomic_get(&__last_log_time);                                    \
		uint32_t __diff = __now - __last;                                                  \
		if (unlikely(__diff >= (_rate_ms))) {                                              \
			if (atomic_cas(&__last_log_time, __last, __now)) {                         \
				uint32_t __skipped = atomic_clear(&__skipped_count);               \
				if (__skipped > 0) {                                               \
					Z_LOG(_level, "Skipped %d messages", __skipped);           \
				}                                                                  \
				Z_LOG(_level, __VA_ARGS__);                                        \
			} else {                                                                   \
				atomic_inc(&__skipped_count);                                      \
			}                                                                          \
		} else {                                                                           \
			atomic_inc(&__skipped_count);                                              \
		}                                                                                  \
	} while (0)

/**
 * @brief Rate-limited logging macros
 *
 * @details These macros provide rate-limited logging functionality to prevent
 * log flooding when messages are generated frequently. Each macro ensures that
 * log messages are not output more frequently than a specified interval.
 * Rate limiting is per-macro-call-site, meaning each unique call has its own
 * independent rate limit.
 *
 * The macros use atomic operations to ensure thread safety in multi-threaded
 * environments. Only one thread can successfully log a message within the
 * specified rate limit period.
 *
 * @see CONFIG_LOG_RATELIMIT_INTERVAL_MS
 */

/**
 * @brief Core rate-limited logging macro with level parameter
 *
 * @details Internal macro that provides rate-limited logging functionality
 * with configurable log level. Uses atomic operations to ensure thread safety
 * in multi-threaded environments.
 *
 * @param _level Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... Arguments to pass to the logging function.
 */
#define _LOG_RATELIMIT_LVL(_level, _rate_ms, ...)                                                  \
	do {                                                                                       \
		if (IS_ENABLED(CONFIG_LOG_RATELIMIT)) {                                            \
			_LOG_RATELIMIT_CORE(_level, _rate_ms, __VA_ARGS__);                        \
		} else if (IS_ENABLED(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)) {                       \
			(void)0;                                                                   \
		} else {                                                                           \
			Z_LOG(_level, __VA_ARGS__);                                                \
		}                                                                                  \
	} while (0)

/**
 * @brief Writes a WARNING level message to the log with rate limiting.
 *
 * @details It's meant for situations that warrant investigation but could clutter
 * the logs if output too frequently. The message will be logged at most once
 * per default interval (see CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN_RATELIMIT(...)                                                                     \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_WRN, LOG_RATELIMIT_INTERVAL_MS, __VA_ARGS__)

/**
 * @brief Writes an ERROR level message to the log with rate limiting.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per default interval (see
 * CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR_RATELIMIT(...)                                                                     \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_ERR, LOG_RATELIMIT_INTERVAL_MS, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log with rate limiting.
 *
 * @details It's meant to write generic user oriented messages with rate limiting
 * to prevent log flooding. The message will be logged at most once per default
 * interval (see CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INF_RATELIMIT(...)                                                                     \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_INF, LOG_RATELIMIT_INTERVAL_MS, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message to the log with rate limiting.
 *
 * @details It's meant to write developer oriented information with rate limiting
 * to prevent log flooding. The message will be logged at most once per default
 * interval (see CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG_RATELIMIT(...)                                                                     \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_DBG, LOG_RATELIMIT_INTERVAL_MS, __VA_ARGS__)

/**
 * @brief Core rate-limited logging macro for hexdump messages
 *
 * @details Internal macro that provides rate-limited logging functionality
 * for hexdump log messages. Uses atomic operations to ensure thread safety
 * in multi-threaded environments. Only one thread can successfully log a
 * message within the specified rate limit period.
 *
 * @param _level Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define _LOG_HEXDUMP_RATELIMIT_CORE(_level, _rate_ms, _data, _length, _str)                        \
	do {                                                                                       \
		static atomic_t __last_log_time;                                                   \
		static atomic_t __skipped_count;                                                   \
		uint32_t __now = k_uptime_get_32();                                                \
		uint32_t __last = atomic_get(&__last_log_time);                                    \
		uint32_t __diff = __now - __last;                                                  \
		if (unlikely(__diff >= (_rate_ms))) {                                              \
			if (atomic_cas(&__last_log_time, __last, __now)) {                         \
				uint32_t __skipped = atomic_clear(&__skipped_count);               \
				if (__skipped > 0) {                                               \
					Z_LOG(_level, "Skipped %d hexdump messages", __skipped);   \
				}                                                                  \
				Z_LOG_HEXDUMP(_level, _data, _length, _str);                       \
			} else {                                                                   \
				atomic_inc(&__skipped_count);                                      \
			}                                                                          \
		} else {                                                                           \
			atomic_inc(&__skipped_count);                                              \
		}                                                                                  \
	} while (0)

/**
 * @brief Core rate-limited hexdump logging macro with level parameter
 *
 * @details Internal macro that provides rate-limited hexdump logging functionality
 * with configurable log level. Uses atomic operations to ensure thread safety
 * in multi-threaded environments.
 *
 * @param _level Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define _LOG_HEXDUMP_RATELIMIT_LVL(_level, _rate_ms, _data, _length, _str)                         \
	do {                                                                                       \
		if (IS_ENABLED(CONFIG_LOG_RATELIMIT)) {                                            \
			_LOG_HEXDUMP_RATELIMIT_CORE(_level, _rate_ms, _data, _length, _str);       \
		} else if (IS_ENABLED(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)) {                       \
			(void)0;                                                                   \
		} else {                                                                           \
			Z_LOG_HEXDUMP(_level, _data, _length, _str);                               \
		}                                                                                  \
	} while (0)

/**
 * @brief Writes an ERROR level hexdump message to the log with rate limiting.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per default interval (see
 * CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_ERR_RATELIMIT(_data, _length, _str)                                            \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_ERR, LOG_RATELIMIT_INTERVAL_MS, _data, _length, _str)

/**
 * @brief Writes a WARNING level hexdump message to the log with rate limiting.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per default interval (see
 * CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_WRN_RATELIMIT(_data, _length, _str)                                            \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_WRN, LOG_RATELIMIT_INTERVAL_MS, _data, _length, _str)

/**
 * @brief Writes an INFO level hexdump message to the log with rate limiting.
 *
 * @details It's meant to write generic user oriented messages with rate limiting
 * to prevent log flooding. The message will be logged at most once per default
 * interval (see CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_INF_RATELIMIT(_data, _length, _str)                                            \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_INF, LOG_RATELIMIT_INTERVAL_MS, _data, _length, _str)

/**
 * @brief Writes a DEBUG level hexdump message to the log with rate limiting.
 *
 * @details It's meant to write developer oriented information with rate limiting
 * to prevent log flooding. The message will be logged at most once per default
 * interval (see CONFIG_LOG_RATELIMIT_INTERVAL_MS).
 *
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_DBG_RATELIMIT(_data, _length, _str)                                            \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_DBG, LOG_RATELIMIT_INTERVAL_MS, _data, _length, _str)

/**
 * @brief Rate-limited logging macros with custom rate
 *
 * @details These macros provide rate-limited logging functionality with custom
 * rate intervals. They take an explicit rate parameter (in milliseconds) that
 * specifies the minimum interval between log messages.
 */

/**
 * @brief Writes an ERROR level message to the log with custom rate limiting.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per specified interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR_RATELIMIT_RATE(_rate_ms, ...)                                                      \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_ERR, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message to the log with custom rate limiting.
 *
 * @details It's meant for situations that warrant investigation but could clutter
 * the logs if output too frequently. The message will be logged at most once
 * per specified interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN_RATELIMIT_RATE(_rate_ms, ...)                                                      \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_WRN, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log with custom rate limiting.
 *
 * @details It's meant to write generic user oriented messages with rate limiting
 * to prevent log flooding. The message will be logged at most once per specified
 * interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INF_RATELIMIT_RATE(_rate_ms, ...)                                                      \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_INF, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message to the log with custom rate limiting.
 *
 * @details It's meant to write developer oriented information with rate limiting
 * to prevent log flooding. The message will be logged at most once per specified
 * interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG_RATELIMIT_RATE(_rate_ms, ...)                                                      \
	_LOG_RATELIMIT_LVL(LOG_LEVEL_DBG, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes an ERROR level hexdump message to the log with custom rate limiting.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per specified interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_ERR_RATELIMIT_RATE(_rate_ms, _data, _length, _str)                             \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_ERR, _rate_ms, _data, _length, _str)

/**
 * @brief Writes a WARNING level hexdump message to the log with custom rate limiting.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors, but with rate limiting to prevent log flooding.
 * The message will be logged at most once per specified interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_WRN_RATELIMIT_RATE(_rate_ms, _data, _length, _str)                             \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_WRN, _rate_ms, _data, _length, _str)

/**
 * @brief Writes an INFO level hexdump message to the log with custom rate limiting.
 *
 * @details It's meant to write generic user oriented messages with rate limiting
 * to prevent log flooding. The message will be logged at most once per specified
 * interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_INF_RATELIMIT_RATE(_rate_ms, _data, _length, _str)                             \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_INF, _rate_ms, _data, _length, _str)

/**
 * @brief Writes a DEBUG level hexdump message to the log with custom rate limiting.
 *
 * @details It's meant to write developer oriented information with rate limiting
 * to prevent log flooding. The message will be logged at most once per specified
 * interval.
 *
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_DBG_RATELIMIT_RATE(_rate_ms, _data, _length, _str)                             \
	_LOG_HEXDUMP_RATELIMIT_LVL(LOG_LEVEL_DBG, _rate_ms, _data, _length, _str)

/**
 * @brief Unconditionally print raw log message.
 *
 * The result is same as if printk was used but it goes through logging
 * infrastructure thus utilizes logging mode, e.g. deferred mode.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_PRINTK(...) Z_LOG_PRINTK(0, __VA_ARGS__)

/**
 * @brief Unconditionally print raw log message.
 *
 * Provided string is printed as is without appending any characters (e.g., color or newline).
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_RAW(...) Z_LOG_PRINTK(1, __VA_ARGS__)

/**
 * @brief Writes an ERROR level message associated with the instance to the log.
 *
 * Message is associated with specific instance of the module which has
 * independent filtering settings (if runtime filtering is enabled) and
 * message prefix (\<module_name\>.\<instance_name\>). It's meant to report
 * severe errors, such as those from which it's not possible to recover.
 *
 * @param _log_inst Pointer to the log structure associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_ERR(_log_inst, ...) Z_LOG_INSTANCE(LOG_LEVEL_ERR, _log_inst, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message associated with the instance to the
 *        log.
 *
 * Message is associated with specific instance of the module which has
 * independent filtering settings (if runtime filtering is enabled) and
 * message prefix (\<module_name\>.\<instance_name\>). It's meant to register
 * messages related to unusual situations that are not necessarily errors.
 *
 * @param _log_inst Pointer to the log structure associated with the instance.
 * @param ...       A string optionally containing printk valid conversion
 *                  specifier, followed by as many values as specifiers.
 */
#define LOG_INST_WRN(_log_inst, ...) Z_LOG_INSTANCE(LOG_LEVEL_WRN, _log_inst, __VA_ARGS__)

/**
 * @brief Writes an INFO level message associated with the instance to the log.
 *
 * Message is associated with specific instance of the module which has
 * independent filtering settings (if runtime filtering is enabled) and
 * message prefix (\<module_name\>.\<instance_name\>). It's meant to write
 * generic user oriented messages.
 *
 * @param _log_inst Pointer to the log structure associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_INF(_log_inst, ...) Z_LOG_INSTANCE(LOG_LEVEL_INF, _log_inst, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message associated with the instance to the log.
 *
 * Message is associated with specific instance of the module which has
 * independent filtering settings (if runtime filtering is enabled) and
 * message prefix (\<module_name\>.\<instance_name\>). It's meant to write
 * developer oriented information.
 *
 * @param _log_inst Pointer to the log structure associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_DBG(_log_inst, ...) Z_LOG_INSTANCE(LOG_LEVEL_DBG, _log_inst, __VA_ARGS__)

/**
 * @brief Writes an ERROR level hexdump message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param _data   Pointer to the data to be logged.
 * @param _length Length of data (in bytes).
 * @param _str    Persistent, raw string.
 */
#define LOG_HEXDUMP_ERR(_data, _length, _str) Z_LOG_HEXDUMP(LOG_LEVEL_ERR, _data, _length, (_str))

/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param _data   Pointer to the data to be logged.
 * @param _length Length of data (in bytes).
 * @param _str    Persistent, raw string.
 */
#define LOG_HEXDUMP_WRN(_data, _length, _str) Z_LOG_HEXDUMP(LOG_LEVEL_WRN, _data, _length, (_str))

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param _data   Pointer to the data to be logged.
 * @param _length Length of data (in bytes).
 * @param _str    Persistent, raw string.
 */
#define LOG_HEXDUMP_INF(_data, _length, _str) Z_LOG_HEXDUMP(LOG_LEVEL_INF, _data, _length, (_str))

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param _data   Pointer to the data to be logged.
 * @param _length Length of data (in bytes).
 * @param _str    Persistent, raw string.
 */
#define LOG_HEXDUMP_DBG(_data, _length, _str) Z_LOG_HEXDUMP(LOG_LEVEL_DBG, _data, _length, (_str))

/**
 * @brief Writes an ERROR hexdump message associated with the instance to the
 *        log.
 *
 * Message is associated with specific instance of the module which has
 * independent filtering settings (if runtime filtering is enabled) and
 * message prefix (\<module_name\>.\<instance_name\>). It's meant to report
 * severe errors, such as those from which it's not possible to recover.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param _data       Pointer to the data to be logged.
 * @param _length     Length of data (in bytes).
 * @param _str        Persistent, raw string.
 */
#define LOG_INST_HEXDUMP_ERR(_log_inst, _data, _length, _str)                                      \
	Z_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_ERR, _log_inst, _data, _length, _str)

/**
 * @brief Writes a WARNING level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param _data       Pointer to the data to be logged.
 * @param _length     Length of data (in bytes).
 * @param _str        Persistent, raw string.
 */
#define LOG_INST_HEXDUMP_WRN(_log_inst, _data, _length, _str)                                      \
	Z_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_WRN, _log_inst, _data, _length, _str)

/**
 * @brief Writes an INFO level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param _data       Pointer to the data to be logged.
 * @param _length     Length of data (in bytes).
 * @param _str        Persistent, raw string.
 */
#define LOG_INST_HEXDUMP_INF(_log_inst, _data, _length, _str)                                      \
	Z_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_INF, _log_inst, _data, _length, _str)

/**
 * @brief Writes a DEBUG level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param _data       Pointer to the data to be logged.
 * @param _length     Length of data (in bytes).
 * @param _str        Persistent, raw string.
 */
#define LOG_INST_HEXDUMP_DBG(_log_inst, _data, _length, _str)                                      \
	Z_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_DBG, _log_inst, _data, _length, _str)

/**
 * @brief Writes an formatted string to the log.
 *
 * @details Conditionally compiled (see CONFIG_LOG_PRINTK). Function provides
 * printk functionality.
 *
 * It is less efficient compared to standard logging because static packaging
 * cannot be used.
 *
 * @param fmt Formatted string to output.
 * @param ap  Variable parameters.
 */
void z_log_vprintk(const char *fmt, va_list ap);

#ifdef __cplusplus
}
#define LOG_IN_CPLUSPLUS 1
#endif
/* Macro expects that optionally on second argument local log level is provided.
 * If provided it is returned, otherwise default log level is returned or
 * LOG_LEVEL, if it was locally defined.
 */
#if !defined(CONFIG_LOG)
#define _LOG_LEVEL_RESOLVE(...) LOG_LEVEL_NONE
#else
#define _LOG_LEVEL_RESOLVE(...)                                                                    \
	Z_LOG_EVAL(COND_CODE_0(LOG_LEVEL, (1), (LOG_LEVEL)), \
		(GET_ARG_N(2, __VA_ARGS__, LOG_LEVEL)), \
		(GET_ARG_N(2, __VA_ARGS__, CONFIG_LOG_DEFAULT_LEVEL)))
#endif

/* Return first argument */
#define _LOG_ARG1(arg1, ...) arg1

#define _LOG_MODULE_CONST_DATA_CREATE(_name, _level)                                               \
	IF_ENABLED(CONFIG_LOG_FMT_SECTION, (							\
		static const char UTIL_CAT(_name, _str)[]					\
		     __in_section(_log_strings, static, _CONCAT(_name, _)) __used __noasan =	\
		     STRINGIFY(_name);))                                               \
	IF_ENABLED(LOG_IN_CPLUSPLUS, (extern))                                                     \
	const STRUCT_SECTION_ITERABLE_ALTERNATE(log_const, log_source_const_data,                  \
						Z_LOG_ITEM_CONST_DATA(_name)) = {                  \
		.name = COND_CODE_1(CONFIG_LOG_FMT_SECTION,					\
				(UTIL_CAT(_name, _str)), (STRINGIFY(_name))), .level = (_level)}

#define _LOG_MODULE_DYNAMIC_DATA_CREATE(_name)                                                     \
	STRUCT_SECTION_ITERABLE_ALTERNATE(log_dynamic, log_source_dynamic_data,                    \
					  LOG_ITEM_DYNAMIC_DATA(_name))

#define _LOG_MODULE_DYNAMIC_DATA_COND_CREATE(_name)                                                \
	IF_ENABLED(CONFIG_LOG_RUNTIME_FILTERING,		\
		  (_LOG_MODULE_DYNAMIC_DATA_CREATE(_name);))

#define _LOG_MODULE_DATA_CREATE(_name, _level)                                                     \
	_LOG_MODULE_CONST_DATA_CREATE(_name, _level);                                              \
	_LOG_MODULE_DYNAMIC_DATA_COND_CREATE(_name)

/* Determine if data for the module shall be created. It is created if logging
 * is enabled, override level is set or module specific level is set (not off).
 */
#define Z_DO_LOG_MODULE_REGISTER(...)                                                              \
	COND_CODE_1(CONFIG_LOG, \
		(Z_LOG_EVAL(CONFIG_LOG_OVERRIDE_LEVEL, \
		   (1), \
		   (Z_LOG_EVAL(_LOG_LEVEL_RESOLVE(__VA_ARGS__), (1), (0))) \
		  )), (0))

/* Determine if the data of the log module shall be in the partition
 * 'k_log_partition' to allow a user mode thread access to this data.
 */
#if CONFIG_USERSPACE && CONFIG_LOG_ALWAYS_RUNTIME
extern struct k_mem_partition k_log_partition;
#define Z_LOG_MODULE_PARTITION(_k_app_mem) _k_app_mem(k_log_partition)
#else
#define Z_LOG_MODULE_PARTITION(_k_app_mem)
#endif

/**
 * @brief Create module-specific state and register the module with Logger.
 *
 * This macro normally must be used after including <zephyr/logging/log.h> to
 * complete the initialization of the module.
 *
 * Module registration can be skipped in two cases:
 *
 * - The module consists of more than one file, and another file
 *   invokes this macro. (LOG_MODULE_DECLARE() should be used instead
 *   in all of the module's other files.)
 * - Instance logging is used and there is no need to create module entry. In
 *   that case LOG_LEVEL_SET() should be used to set log level used within the
 *   file.
 *
 * Macro accepts one or two parameters:
 * - module name
 * - optional log level. If not provided then default log level is used in
 *  the file.
 *
 * Example usage:
 * - LOG_MODULE_REGISTER(foo, CONFIG_FOO_LOG_LEVEL)
 * - LOG_MODULE_REGISTER(foo)
 *
 *
 * @note The module's state is defined, and the module is registered,
 *       only if LOG_LEVEL for the current source file is non-zero or
 *       it is not defined and CONFIG_LOG_DEFAULT_LEVEL is non-zero.
 *       In other cases, this macro has no effect.
 * @see LOG_MODULE_DECLARE
 */
#define LOG_MODULE_REGISTER(...)                                                                   \
	COND_CODE_1(							\
		Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__),			\
		(_LOG_MODULE_DATA_CREATE(GET_ARG_N(1, __VA_ARGS__),	\
				      _LOG_LEVEL_RESOLVE(__VA_ARGS__))),\
		() \
	)                                                                       \
	LOG_MODULE_DECLARE(__VA_ARGS__)

/**
 * @brief Macro for declaring a log module (not registering it).
 *
 * Modules which are split up over multiple files must have exactly
 * one file use LOG_MODULE_REGISTER() to create module-specific state
 * and register the module with the logger core.
 *
 * The other files in the module should use this macro instead to
 * declare that same state. (Otherwise, LOG_INF() etc. will not be
 * able to refer to module-specific state variables.)
 *
 * Macro accepts one or two parameters:
 * - module name
 * - optional log level. If not provided then default log level is used in
 *  the file.
 *
 * Example usage:
 * - LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL)
 * - LOG_MODULE_DECLARE(foo)
 *
 * @note The module's state is declared only if LOG_LEVEL for the
 *       current source file is non-zero or it is not defined and
 *       CONFIG_LOG_DEFAULT_LEVEL is non-zero.  In other cases,
 *       this macro has no effect.
 * @see LOG_MODULE_REGISTER
 */
#define LOG_MODULE_DECLARE(...)                                                                    \
	extern const struct log_source_const_data Z_LOG_ITEM_CONST_DATA(                           \
		GET_ARG_N(1, __VA_ARGS__));                                                        \
	extern struct log_source_dynamic_data LOG_ITEM_DYNAMIC_DATA(GET_ARG_N(1, __VA_ARGS__));    \
                                                                                                   \
	Z_LOG_MODULE_PARTITION(K_APP_DMEM)                                                         \
	static const struct log_source_const_data *__log_current_const_data __unused =             \
		Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__)                                              \
			? &Z_LOG_ITEM_CONST_DATA(GET_ARG_N(1, __VA_ARGS__))                        \
			: NULL;                                                                    \
                                                                                                   \
	Z_LOG_MODULE_PARTITION(K_APP_DMEM)                                                         \
	static struct log_source_dynamic_data *__log_current_dynamic_data __unused =               \
		(Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__) &&                                          \
		 IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING))                                         \
			? &LOG_ITEM_DYNAMIC_DATA(GET_ARG_N(1, __VA_ARGS__))                        \
			: NULL;                                                                    \
                                                                                                   \
	Z_LOG_MODULE_PARTITION(K_APP_BMEM)                                                         \
	static const uint32_t __log_level __unused = _LOG_LEVEL_RESOLVE(__VA_ARGS__)

/**
 * @brief Macro for setting log level in the file or function where instance
 * logging API is used.
 *
 * @param level Level used in file or in function.
 *
 */
#define LOG_LEVEL_SET(level)                                                                       \
	static const uint32_t __log_level __unused = Z_LOG_RESOLVED_LEVEL(level, 0)

#ifdef CONFIG_LOG_CUSTOM_HEADER
/* This include must always be at the end of log.h */
#include <zephyr_custom_log.h>
#endif

/*
 * Eclipse CDT or JetBrains Clion parser is sometimes confused by logging API
 * code and freezes the whole IDE. Following lines hides LOG_x macros from them.
 */
#if defined(__CDT_PARSER__) || defined(__JETBRAINS_IDE__)
#undef LOG_ERR
#undef LOG_WRN
#undef LOG_INF
#undef LOG_DBG

#undef LOG_HEXDUMP_ERR
#undef LOG_HEXDUMP_WRN
#undef LOG_HEXDUMP_INF
#undef LOG_HEXDUMP_DBG

#define LOG_ERR(...) (void)0
#define LOG_WRN(...) (void)0
#define LOG_DBG(...) (void)0
#define LOG_INF(...) (void)0

#define LOG_HEXDUMP_ERR(...) (void)0
#define LOG_HEXDUMP_WRN(...) (void)0
#define LOG_HEXDUMP_DBG(...) (void)0
#define LOG_HEXDUMP_INF(...) (void)0

#undef LOG_ERR_RATELIMIT
#undef LOG_WRN_RATELIMIT
#undef LOG_INF_RATELIMIT
#undef LOG_DBG_RATELIMIT

#undef LOG_ERR_RATELIMIT_RATE
#undef LOG_WRN_RATELIMIT_RATE
#undef LOG_INF_RATELIMIT_RATE
#undef LOG_DBG_RATELIMIT_RATE

#undef LOG_HEXDUMP_ERR_RATELIMIT
#undef LOG_HEXDUMP_WRN_RATELIMIT
#undef LOG_HEXDUMP_INF_RATELIMIT
#undef LOG_HEXDUMP_DBG_RATELIMIT

#undef LOG_HEXDUMP_ERR_RATELIMIT_RATE
#undef LOG_HEXDUMP_WRN_RATELIMIT_RATE
#undef LOG_HEXDUMP_INF_RATELIMIT_RATE
#undef LOG_HEXDUMP_DBG_RATELIMIT_RATE

#define LOG_ERR_RATELIMIT(...) (void)0
#define LOG_WRN_RATELIMIT(...) (void)0
#define LOG_INF_RATELIMIT(...) (void)0
#define LOG_DBG_RATELIMIT(...) (void)0

#define LOG_HEXDUMP_ERR_RATELIMIT(...) (void)0
#define LOG_HEXDUMP_WRN_RATELIMIT(...) (void)0
#define LOG_HEXDUMP_INF_RATELIMIT(...) (void)0
#define LOG_HEXDUMP_DBG_RATELIMIT(...) (void)0

#define LOG_ERR_RATELIMIT_RATE(...) (void)0
#define LOG_WRN_RATELIMIT_RATE(...) (void)0
#define LOG_INF_RATELIMIT_RATE(...) (void)0
#define LOG_DBG_RATELIMIT_RATE(...) (void)0

#define LOG_HEXDUMP_ERR_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_WRN_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_INF_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_DBG_RATELIMIT_RATE(...) (void)0
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_H_ */
