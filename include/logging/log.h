/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOG_H_
#define LOG_H_

#include <logging/log_instance.h>
#include <logging/log_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERR   1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DBG   4

/** @brief Module specific log level.
 *
 * Must be defined before including this file to override default settings.
 *
 * @note This header file should only be included by source files.
 *
 */
#ifndef LOG_LEVEL
#define LOG_SRC_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#else
#define LOG_SRC_LEVEL LOG_LEVEL
#endif

/**
 * @brief Writes an ERROR level message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR(...)    LOG_INTERNAL_MODULE(LOG_LEVEL_ERR, __VA_ARGS__)


/**
 * @brief Writes an WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WARN(...)   LOG_INTERNAL_MODULE(LOG_LEVEL_WARN, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INFO(...)   LOG_INTERNAL_MODULE(LOG_LEVEL_INFO, __VA_ARGS__)

/**
 * @brief Writes an DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG(...)    LOG_INTERNAL_MODULE(LOG_LEVEL_DBG, __VA_ARGS__)

/**
 * @brief Writes an ERROR level message to the log.
 *
 * It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_ERR(p_inst, ...) \
	LOG_INTERNAL_INSTANCE(LOG_LEVEL_ERR, _p_inst, __VA_ARGS__)

/**
 * @brief Writes an WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_WARN(p_inst, ...) \
	LOG_INTERNAL_INSTANCE(LOG_LEVEL_WARN, p_inst, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_INFO(p_inst, ...) \
	LOG_INTERNAL_INSTANCE(LOG_LEVEL_INFO, p_inst, __VA_ARGS__)

/**
 * @brief Writes an DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INST_DBG(p_inst, ...) \
	LOG_INTERNAL_INSTANCE(LOG_LEVEL_DBG, p_inst, __VA_ARGS__)

/**
 * @brief Writes an ERROR level hexdump message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_ERR(p_data, length)	\
	LOG_INTERNAL_HEXDUMP_MODULE(LOG_LEVEL_ERR, p_data, length)

/**
 * @brief Writes an WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_WARN(p_data, length) \
	LOG_INTERNAL_HEXDUMP_MODULE(LOG_LEVEL_WARN, p_data, length)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_INFO(p_data, length) \
	LOG_INTERNAL_HEXDUMP_MODULE(LOG_LEVEL_INFO, p_data, length)

/**
 * @brief Writes an DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_DBG(p_data, length)	\
	LOG_INTERNAL_HEXDUMP_MODULE(LOG_LEVEL_DBG, p_data, length)

/**
 * @brief Writes an ERROR level hexdump message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_ERR(p_inst, p_data, length) \
	LOG_INTERNAL_HEXDUMP_INSTANCE(LOG_LEVEL_ERR, p_inst, p_data, length)

/**
 * @brief Writes an WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_WARN(p_inst, p_data, length) \
	LOG_INTERNAL_HEXDUMP_INSTANCE(LOG_LEVEL_WARN, p_inst, p_data, length)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_INFO(p_inst, p_data, length) \
	LOG_INTERNAL_HEXDUMP_INSTANCE(LOG_LEVEL_INFO, p_inst, p_data, length)

/**
 * @brief Writes an DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param p_inst Pointer to the log filter associated with the instance.
 * @param p_data Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_DBG(p_inst, p_data, length) \
	LOG_INTERNAL_HEXDUMP_INSTANCE(LOG_LEVEL_DBG, p_inst, p_data, length)

/**
 * @brief Writes an formatted string to the log.
 *
 * @details Conditionally compiled (@ref CONFIG_LOG_PRINTK). Function provides
 * printk functionality. It is ineffective compared to standard logging
 * because string formatting is performed in the call context and not deferred to
 * the known context.
 *
 * @param fmt Formatted string to output.
 * @param ap  Variable parameters.
 *
 * return Number of bytes written.
 */
int log_printk(const char *fmt, va_list ap);

/* Register a module unless explicitly skipped (using LOG_SKIP_MODULE_REGISTER).
 * Skipping may be used in 2 cases:
 * - Module consists of more than one file and must be registered only by one file.
 * - Instance logging is used and there is no need to create module entry.*/

#if defined(LOG_MODULE_NAME) && LOG_SRC_LEVEL > LOG_LEVEL_NONE
#define LOG_MODULE_REGISTER()					   \
	LOG_INTERNAL_ROM_ITEM_REGISTER(LOG_MODULE_NAME,		   \
				       STRINGIFY(LOG_MODULE_NAME), \
				       LOG_SRC_LEVEL);		   \
	LOG_INTERNAL_RAM_ITEM_REGISTER(LOG_MODULE_NAME)
#else
#define LOG_MODULE_REGISTER() /* Empty */
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOG_H_ */
