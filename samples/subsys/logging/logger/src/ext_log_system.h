/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EXT_LOG_SYSTEM_H
#define EXT_LOG_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/** Log message priority levels */
enum ext_log_level {
	EXT_LOG_CRITICAL,
	EXT_LOG_ERROR,
	EXT_LOG_WARNING,
	EXT_LOG_NOTICE,
	EXT_LOG_INFO,
	EXT_LOG_DEBUG
};

/** Log message handler type. */
typedef void (*ext_log_handler)(enum ext_log_level level,
				const char *format, ...);

/** @brief Set log handler function.
 *
 * @param handler External log handler.
 */
void ext_log_handler_set(ext_log_handler handler);

/** @brief Example function which is using custom log API. */
void ext_log_system_foo(void);

/** @brief Custom log API. */
#define ext_log(level, ...) log_handler(level, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* EXT_LOG_SYSTEM_H */
