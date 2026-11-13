/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the logging subsystem.
 * @ingroup log_api
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_H_

#include <zephyr/logging/log_instance.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_USERSPACE) && defined(CONFIG_LOG_ALWAYS_RUNTIME)
#include <zephyr/app_memory/app_memdomain.h>
#endif

#ifdef CONFIG_LOG_RATELIMIT
#include <zephyr/kernel.h>
#endif

#ifdef __cplusplus
extern "C" {
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
 * @name Standard logging
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

/** @} */

/**
 * @name Rate-limited logging
 *
 * Rate-limited variants drop repeated messages so that a frequently-hit call
 * site cannot flood the log. Each call site is limited independently using
 * atomic operations for thread safety, and a summary of skipped messages is
 * emitted when logging resumes. The plain variants use the default interval
 * @ref LOG_RATELIMIT_INTERVAL_MS; the `_RATE` variants take an explicit
 * interval, and the `_STATE` variants take an explicit @ref
 * log_ratelimit_state in addition to the interval.
 * @{
 */

/**
 * @brief Default interval for rate-limited log messages, in milliseconds.
 *
 * Period used by the rate-limited logging macros that do not take an explicit
 * rate (for example @ref LOG_ERR_RATELIMIT). It evaluates to
 * @kconfig{CONFIG_LOG_RATELIMIT_INTERVAL_MS} when @kconfig{CONFIG_LOG_RATELIMIT}
 * is enabled, and to 0 (no rate limiting) otherwise.
 */
#ifdef CONFIG_LOG_RATELIMIT
#define LOG_RATELIMIT_INTERVAL_MS CONFIG_LOG_RATELIMIT_INTERVAL_MS
#else
#define LOG_RATELIMIT_INTERVAL_MS 0
#endif

/**
 * @brief Rate limit state.
 *
 * One instance is required for each independently rate-limited source. The
 * rate-limited logging macros declare one such instance per call site, so
 * sources sharing a call site share the rate limit. Provide the state
 * explicitly with the `LOG_*_RATELIMIT_STATE` macros to rate limit each
 * object instance independently.
 *
 * The state must be zero-initialized (static storage or explicit initializer)
 * before its first use.
 */
struct log_ratelimit_state {
	/** Time of the last emitted message, 0 if no message was emitted yet. */
	atomic_t last_log_time;
	/** Number of messages suppressed since the last emitted message. */
	atomic_t skipped_count;
};

#ifdef CONFIG_LOG_RATELIMIT
/* Rate limit state machine. The time snapshots are passed explicitly so that
 * the state machine can be exercised with fabricated timestamps.
 *
 * @retval -1  Message shall be suppressed.
 * @retval >=0 Message shall be emitted; the value is the number of messages
 *             suppressed since the last emitted message.
 */
static inline int32_t z_log_ratelimit_update(struct log_ratelimit_state *state,
					     uint32_t rate_ms, uint32_t last, uint32_t now)
{
	/* 0 is reserved to mean "no message emitted yet" so it cannot be used as
	 * a timestamp. Biasing the very first millisecond to 1 is harmless.
	 */
	if (now == 0U) {
		now = 1U;
	}

	if ((last != 0U) && ((now - last) < rate_ms)) {
		atomic_inc(&state->skipped_count);
		return -1;
	}

	if (!atomic_cas(&state->last_log_time, last, now)) {
		/* Another context won the race for this interval. */
		atomic_inc(&state->skipped_count);
		return -1;
	}

	return (int32_t)atomic_clear(&state->skipped_count);
}

/**
 * @brief Check whether a message may be emitted for a given rate limit state.
 *
 * Internal helper used by all the `LOG_*_RATELIMIT*` macros.
 *
 * @param state   Rate limit state.
 * @param rate_ms Minimum interval in milliseconds between messages.
 *
 * @retval -1  Message shall be suppressed.
 * @retval >=0 Message shall be emitted; the value is the number of messages
 *             suppressed since the last emitted message.
 */
static inline int32_t z_log_ratelimit_take(struct log_ratelimit_state *state,
					   uint32_t rate_ms)
{
	/* Read the last timestamp before the current time: if a context is
	 * preempted in between and another context emits a message, last <= now
	 * still holds and the stale expectation makes the CAS fail, instead of
	 * a wrapped now - last passing the interval check and the CAS moving
	 * the timestamp backwards.
	 */
	uint32_t last = (uint32_t)atomic_get(&state->last_log_time);
	uint32_t now = k_uptime_get_32();

	return z_log_ratelimit_update(state, rate_ms, last, now);
}
#endif /* CONFIG_LOG_RATELIMIT */

/**
 * @brief Emit a rate-limited message using an explicit rate limit state.
 *
 * @param _level   Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ...      Arguments to pass to the logging function.
 */
#ifdef CONFIG_LOG_RATELIMIT
#define _LOG_RATELIMIT_STATE(_level, _state, _rate_ms, ...)                                        \
	do {                                                                                       \
		if (Z_LOG_CONST_LEVEL_CHECK(_level)) {                                             \
			int32_t __rl_skipped = z_log_ratelimit_take((_state), (_rate_ms));         \
			if (__rl_skipped >= 0) {                                                   \
				if (__rl_skipped > 0) {                                            \
					Z_LOG(_level, "Skipped %d messages", __rl_skipped);        \
				}                                                                  \
				Z_LOG(_level, __VA_ARGS__);                                        \
			}                                                                          \
		}                                                                                  \
	} while (0)
#elif defined(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)
#define _LOG_RATELIMIT_STATE(_level, _state, _rate_ms, ...) ((void)(_state))
#else
#define _LOG_RATELIMIT_STATE(_level, _state, _rate_ms, ...)                                        \
	do {                                                                                       \
		(void)(_state);                                                                    \
		Z_LOG(_level, __VA_ARGS__);                                                        \
	} while (0)
#endif

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
		if (Z_LOG_CONST_LEVEL_CHECK(_level)) {                                             \
			static struct log_ratelimit_state __rl_state;                            \
			_LOG_RATELIMIT_STATE(_level, &__rl_state, _rate_ms, __VA_ARGS__);          \
		}                                                                                  \
	} while (0)

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
#ifdef CONFIG_LOG_RATELIMIT
#define _LOG_RATELIMIT_LVL(_level, _rate_ms, ...)                                                  \
	_LOG_RATELIMIT_CORE(_level, _rate_ms, __VA_ARGS__)
#elif defined(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)
#define _LOG_RATELIMIT_LVL(_level, _rate_ms, ...)                                                  \
	do {                                                                                       \
		(void)0;                                                                           \
	} while (0)
#else
#define _LOG_RATELIMIT_LVL(_level, _rate_ms, ...) Z_LOG(_level, __VA_ARGS__)
#endif

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
 * @brief Emit a rate-limited hexdump using an explicit rate limit state.
 *
 * @param _level   Log level (LOG_LEVEL_ERR, LOG_LEVEL_WRN, etc.)
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#ifdef CONFIG_LOG_RATELIMIT
#define _LOG_HEXDUMP_RATELIMIT_STATE(_level, _state, _rate_ms, _data, _length, _str)               \
	do {                                                                                       \
		if (Z_LOG_CONST_LEVEL_CHECK(_level)) {                                             \
			int32_t __rl_skipped = z_log_ratelimit_take((_state), (_rate_ms));         \
			if (__rl_skipped >= 0) {                                                   \
				if (__rl_skipped > 0) {                                            \
					Z_LOG(_level, "Skipped %d hexdump messages",               \
				      __rl_skipped);                                       \
				}                                                                  \
				Z_LOG_HEXDUMP(_level, _data, _length, _str);                       \
			}                                                                          \
		}                                                                                  \
	} while (0)
#elif defined(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)
#define _LOG_HEXDUMP_RATELIMIT_STATE(_level, _state, _rate_ms, _data, _length, _str)               \
	do {                                                                                       \
		(void)(_state);                                                                    \
		(void)(_data);                                                                     \
		(void)(_str);                                                                      \
	} while (0)
#else
#define _LOG_HEXDUMP_RATELIMIT_STATE(_level, _state, _rate_ms, _data, _length, _str)               \
	do {                                                                                       \
		(void)(_state);                                                                    \
		Z_LOG_HEXDUMP(_level, _data, _length, _str);                                       \
	} while (0)
#endif

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
		if (Z_LOG_CONST_LEVEL_CHECK(_level)) {                                             \
			static struct log_ratelimit_state __rl_state;                            \
			_LOG_HEXDUMP_RATELIMIT_STATE(_level, &__rl_state, _rate_ms, _data,         \
						     _length, _str);                               \
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
#ifdef CONFIG_LOG_RATELIMIT
#define _LOG_HEXDUMP_RATELIMIT_LVL(_level, _rate_ms, _data, _length, _str)                         \
	_LOG_HEXDUMP_RATELIMIT_CORE(_level, _rate_ms, _data, _length, _str)
#elif defined(CONFIG_LOG_RATELIMIT_FALLBACK_DROP)
#define _LOG_HEXDUMP_RATELIMIT_LVL(_level, _rate_ms, _data, _length, _str)                         \
	do {                                                                                       \
		(void)0;                                                                           \
	} while (0)
#else
#define _LOG_HEXDUMP_RATELIMIT_LVL(_level, _rate_ms, _data, _length, _str)                         \
	Z_LOG_HEXDUMP(_level, _data, _length, _str)
#endif

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
 * @brief Writes an ERROR level message to the log with rate limiting and an
 *        explicit rate limit state.
 *
 * @details Same as @ref LOG_ERR_RATELIMIT_RATE, except that the rate limit
 * state is provided by the caller instead of being declared at the call site.
 * This allows sources that share a call site (for example several device
 * instances) to be rate limited independently, by giving each of them its own
 * state. The state must be zero-initialized before its first use.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR_RATELIMIT_STATE(_state, _rate_ms, ...)                                             \
	_LOG_RATELIMIT_STATE(LOG_LEVEL_ERR, _state, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message to the log with rate limiting and an
 *        explicit rate limit state.
 *
 * @details See @ref LOG_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN_RATELIMIT_STATE(_state, _rate_ms, ...)                                             \
	_LOG_RATELIMIT_STATE(LOG_LEVEL_WRN, _state, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log with rate limiting and an
 *        explicit rate limit state.
 *
 * @details See @ref LOG_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INF_RATELIMIT_STATE(_state, _rate_ms, ...)                                             \
	_LOG_RATELIMIT_STATE(LOG_LEVEL_INF, _state, _rate_ms, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message to the log with rate limiting and an
 *        explicit rate limit state.
 *
 * @details See @ref LOG_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG_RATELIMIT_STATE(_state, _rate_ms, ...)                                             \
	_LOG_RATELIMIT_STATE(LOG_LEVEL_DBG, _state, _rate_ms, __VA_ARGS__)

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
 * @brief Writes an ERROR level hexdump message to the log with rate limiting
 *        and an explicit rate limit state.
 *
 * @details Same as @ref LOG_ERR_RATELIMIT_STATE, but for hexdump messages.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_ERR_RATELIMIT_STATE(_state, _rate_ms, _data, _length, _str)                    \
	_LOG_HEXDUMP_RATELIMIT_STATE(LOG_LEVEL_ERR, _state, _rate_ms, _data, _length, _str)

/**
 * @brief Writes a WARNING level hexdump message to the log with rate limiting
 *        and an explicit rate limit state.
 *
 * @details See @ref LOG_HEXDUMP_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_WRN_RATELIMIT_STATE(_state, _rate_ms, _data, _length, _str)                    \
	_LOG_HEXDUMP_RATELIMIT_STATE(LOG_LEVEL_WRN, _state, _rate_ms, _data, _length, _str)

/**
 * @brief Writes an INFO level hexdump message to the log with rate limiting
 *        and an explicit rate limit state.
 *
 * @details See @ref LOG_HEXDUMP_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_INF_RATELIMIT_STATE(_state, _rate_ms, _data, _length, _str)                    \
	_LOG_HEXDUMP_RATELIMIT_STATE(LOG_LEVEL_INF, _state, _rate_ms, _data, _length, _str)

/**
 * @brief Writes a DEBUG level hexdump message to the log with rate limiting
 *        and an explicit rate limit state.
 *
 * @details See @ref LOG_HEXDUMP_ERR_RATELIMIT_STATE.
 *
 * @param _state   Pointer to the rate limit state.
 * @param _rate_ms Minimum interval in milliseconds between log messages.
 * @param _data    Pointer to the data to be logged.
 * @param _length  Length of data (in bytes).
 * @param _str     Persistent, raw string.
 */
#define LOG_HEXDUMP_DBG_RATELIMIT_STATE(_state, _rate_ms, _data, _length, _str)                    \
	_LOG_HEXDUMP_RATELIMIT_STATE(LOG_LEVEL_DBG, _state, _rate_ms, _data, _length, _str)

/** @} */

/**
 * @name Raw output
 * @{
 */

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

/** @} */

/**
 * @name Instance logging
 * @{
 */

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

/** @} */

/**
 * @name Hexdump logging
 * @{
 */

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

/** @} */

/**
 * @name Instance hexdump logging
 * @{
 */

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

/** @} */

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
#if defined(CONFIG_USERSPACE) && defined(CONFIG_LOG_ALWAYS_RUNTIME)
extern struct k_mem_partition k_log_partition;
#define Z_LOG_MODULE_PARTITION(_k_app_mem) _k_app_mem(k_log_partition)
#else
#define Z_LOG_MODULE_PARTITION(_k_app_mem)
#endif

/* Declare the data of the log module. The declarations must precede the
 * definition created by LOG_MODULE_REGISTER(): redeclaring an object after its
 * definition is reported by -Wtentative-definition-compat (Clang 21).
 */
#define Z_LOG_MODULE_DECLARE_EXTERN(...)                                                           \
	extern const struct log_source_const_data Z_LOG_ITEM_CONST_DATA(                           \
		GET_ARG_N(1, __VA_ARGS__));                                                        \
	extern struct log_source_dynamic_data LOG_ITEM_DYNAMIC_DATA(GET_ARG_N(1, __VA_ARGS__))

/* Create the file local pointers to the data of the log module. */
#define Z_LOG_MODULE_DECLARE_LOCAL(...)                                                            \
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
 * @name Module registration
 * @{
 */

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
	Z_LOG_MODULE_DECLARE_EXTERN(__VA_ARGS__);                                                  \
	COND_CODE_1(							\
		Z_DO_LOG_MODULE_REGISTER(__VA_ARGS__),			\
		(_LOG_MODULE_DATA_CREATE(GET_ARG_N(1, __VA_ARGS__),	\
				      _LOG_LEVEL_RESOLVE(__VA_ARGS__))),\
		() \
	)                                                                       \
	Z_LOG_MODULE_DECLARE_LOCAL(__VA_ARGS__)

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
	Z_LOG_MODULE_DECLARE_EXTERN(__VA_ARGS__);                                                  \
	Z_LOG_MODULE_DECLARE_LOCAL(__VA_ARGS__)

/**
 * @brief Macro for setting log level in the file or function where instance
 * logging API is used.
 *
 * @param level Level used in file or in function.
 *
 */
#define LOG_LEVEL_SET(level)                                                                       \
	static const uint32_t __log_level __unused = Z_LOG_RESOLVED_LEVEL(level, 0)

/** @} */

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

#undef LOG_ERR_RATELIMIT_STATE
#undef LOG_WRN_RATELIMIT_STATE
#undef LOG_INF_RATELIMIT_STATE
#undef LOG_DBG_RATELIMIT_STATE

#undef LOG_HEXDUMP_ERR_RATELIMIT
#undef LOG_HEXDUMP_WRN_RATELIMIT
#undef LOG_HEXDUMP_INF_RATELIMIT
#undef LOG_HEXDUMP_DBG_RATELIMIT

#undef LOG_HEXDUMP_ERR_RATELIMIT_RATE
#undef LOG_HEXDUMP_WRN_RATELIMIT_RATE
#undef LOG_HEXDUMP_INF_RATELIMIT_RATE
#undef LOG_HEXDUMP_DBG_RATELIMIT_RATE

#undef LOG_HEXDUMP_ERR_RATELIMIT_STATE
#undef LOG_HEXDUMP_WRN_RATELIMIT_STATE
#undef LOG_HEXDUMP_INF_RATELIMIT_STATE
#undef LOG_HEXDUMP_DBG_RATELIMIT_STATE

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

#define LOG_ERR_RATELIMIT_STATE(...) (void)0
#define LOG_WRN_RATELIMIT_STATE(...) (void)0
#define LOG_INF_RATELIMIT_STATE(...) (void)0
#define LOG_DBG_RATELIMIT_STATE(...) (void)0

#define LOG_HEXDUMP_ERR_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_WRN_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_INF_RATELIMIT_RATE(...) (void)0
#define LOG_HEXDUMP_DBG_RATELIMIT_RATE(...) (void)0

#define LOG_HEXDUMP_ERR_RATELIMIT_STATE(...) (void)0
#define LOG_HEXDUMP_WRN_RATELIMIT_STATE(...) (void)0
#define LOG_HEXDUMP_INF_RATELIMIT_STATE(...) (void)0
#define LOG_HEXDUMP_DBG_RATELIMIT_STATE(...) (void)0
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_H_ */
