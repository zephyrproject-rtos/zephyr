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

/**
 * @brief Logger API
 * @defgroup log_api Logging API
 * @ingroup logger
 * @{
 */

#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERR   1
#define LOG_LEVEL_WRN   2
#define LOG_LEVEL_INF   3
#define LOG_LEVEL_DBG   4

/**
 * @brief Writes an ERROR level message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_ERR(...)    _LOG(LOG_LEVEL_ERR, __VA_ARGS__)

/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_WRN(...)   _LOG(LOG_LEVEL_WRN, __VA_ARGS__)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_INF(...)   _LOG(LOG_LEVEL_INF, __VA_ARGS__)

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param ... A string optionally containing printk valid conversion specifier,
 * followed by as many values as specifiers.
 */
#define LOG_DBG(...)    _LOG(LOG_LEVEL_DBG, __VA_ARGS__)

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
#define LOG_INST_ERR(_log_inst, ...) \
	_LOG_INSTANCE(LOG_LEVEL_ERR, _log_inst, __VA_ARGS__)

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
#define LOG_INST_WRN(_log_inst, ...) \
	_LOG_INSTANCE(LOG_LEVEL_WRN, _log_inst, __VA_ARGS__)

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
#define LOG_INST_INF(_log_inst, ...) \
	_LOG_INSTANCE(LOG_LEVEL_INF, _log_inst, __VA_ARGS__)

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
#define LOG_INST_DBG(_log_inst, ...) \
	_LOG_INSTANCE(LOG_LEVEL_DBG, _log_inst, __VA_ARGS__)

/**
 * @brief Writes an ERROR level hexdump message to the log.
 *
 * @details It's meant to report severe errors, such as those from which it's
 * not possible to recover.
 *
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_ERR(data, length) \
	_LOG_HEXDUMP(LOG_LEVEL_ERR, data, length)

/**
 * @brief Writes a WARNING level message to the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_WRN(data, length) \
	_LOG_HEXDUMP(LOG_LEVEL_WRN, data, length)

/**
 * @brief Writes an INFO level message to the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_INF(data, length) \
	_LOG_HEXDUMP(LOG_LEVEL_INF, data, length)

/**
 * @brief Writes a DEBUG level message to the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_HEXDUMP_DBG(data, length) \
	_LOG_HEXDUMP(LOG_LEVEL_DBG, data, length)

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
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_ERR(_log_inst, data, length) \
	_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_ERR, _log_inst, data, length)

/**
 * @brief Writes a WARNING level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to register messages related to unusual situations that
 * are not necessarily errors.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_WRN(_log_inst, data, length) \
	_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_WRN, _log_inst, data, length)

/**
 * @brief Writes an INFO level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to write generic user oriented messages.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param data   Pointer to the data to be logged.
 * @param length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_INF(_log_inst, data, length) \
	_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_INF, _log_inst, data, length)

/**
 * @brief Writes a DEBUG level hexdump message associated with the instance to
 *        the log.
 *
 * @details It's meant to write developer oriented information.
 *
 * @param _log_inst   Pointer to the log structure associated with the instance.
 * @param _data   Pointer to the data to be logged.
 * @param _length Length of data (in bytes).
 */
#define LOG_INST_HEXDUMP_DBG(_log_inst, _data, _length)	\
	_LOG_HEXDUMP_INSTANCE(LOG_LEVEL_DBG, _log_inst, _data, _length)

/**
 * @brief Writes an formatted string to the log.
 *
 * @details Conditionally compiled (see CONFIG_LOG_PRINTK). Function provides
 * printk functionality. It is inefficient compared to standard logging
 * because string formatting is performed in the call context and not deferred
 * to the log processing context (@ref log_process).
 *
 * @param fmt Formatted string to output.
 * @param ap  Variable parameters.
 *
 * return Number of bytes written.
 */
int log_printk(const char *fmt, va_list ap);


#define __DYNAMIC_MODULE_REGISTER(_name)\
	struct log_source_dynamic_data LOG_ITEM_DYNAMIC_DATA(_name)	\
	__attribute__ ((section("." STRINGIFY(				\
				     LOG_ITEM_DYNAMIC_DATA(_name))))	\
				     )					\
	__attribute__((used))

#define _LOG_RUNTIME_MODULE_REGISTER(_name)				\
	_LOG_EVAL(							\
		CONFIG_LOG_RUNTIME_FILTERING,				\
		(; __DYNAMIC_MODULE_REGISTER(_name)),			\
		()							\
	)

#define _LOG_MODULE_REGISTER(_name, _level)				     \
	const struct log_source_const_data LOG_ITEM_CONST_DATA(_name)	     \
	__attribute__ ((section("." STRINGIFY(LOG_ITEM_CONST_DATA(_name))))) \
	__attribute__((used)) = {					     \
		.name = STRINGIFY(_name),				     \
		.level = _level						     \
	}								     \
	_LOG_RUNTIME_MODULE_REGISTER(_name)

/** @brief Macro for registering a module.
 *
 * Module registration can be skipped in two cases:
 * - Module consists of more than one file and must be registered only by one
 *   file.
 * - Instance logging is used and there is no need to create module entry.
 *
 * @note Module is registered only if LOG_LEVEL for given module is non-zero or
 *       it is not defined and CONFIG_LOG_DEFAULT_LOG_LEVEL is non-zero.
 */
#define LOG_MODULE_REGISTER()						\
	_LOG_EVAL(							\
		_LOG_LEVEL(),						\
		(_LOG_MODULE_REGISTER(LOG_MODULE_NAME, _LOG_LEVEL())),	\
		()/*Empty*/						\
	)

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LOG_H_ */
