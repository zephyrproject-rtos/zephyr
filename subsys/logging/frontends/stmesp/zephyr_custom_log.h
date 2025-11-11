/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Custom logging
 */

#ifndef SOC_NORDIC_COMMON_ZEPHYR_CUSTOM_LOG_H_
#define SOC_NORDIC_COMMON_ZEPHYR_CUSTOM_LOG_H_

#include <zephyr/logging/log_frontend_stmesp.h>
#include <zephyr/sys/cbprintf_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Undef to override those macros. */
#undef LOG_ERR
#undef LOG_WRN
#undef LOG_INF
#undef LOG_DBG
#undef LOG_INST_ERR
#undef LOG_INST_WRN
#undef LOG_INST_INF
#undef LOG_INST_DBG

/** @brief Optimized macro for log message with no arguments.
 *
 * In order to compress information, logging level is stringified and prepended
 * to the string.
 *
 * @param _level Level.
 * @param _module_const_source Constant module structure.
 * @param _source Source structure (dynamic or constant).
 * @param ... String (with no arguments).
 */
#define Z_LOG_STMESP_0(_level, _source, ...)                                                       \
	do {                                                                                       \
		if (!Z_LOG_LEVEL_ALL_CHECK(_level, __log_current_const_data, _source)) {           \
			break;                                                                     \
		}                                                                                  \
		LOG_FRONTEND_STMESP_LOG0(_source, STRINGIFY(_level) __VA_ARGS__);                  \
	} while (0)

/** @brief Determine if first argument is a numeric value that fits in 32 bit word.
 *
 * @return 1 if yes and 0 if not.
 */
#define Z_LOG_STMESP_1_ARG_CHECK(...)                                                              \
	COND_CODE_1(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
			(Z_CBPRINTF_IS_WORD_NUM(GET_ARG_N(2, __VA_ARGS__, dummy))), (0))

/** @brief Optimized macro for log message with 1 numeric argument.
 *
 * In order to compress information, logging level is stringified and prepended
 * to the string.
 *
 * @param _level Level.
 * @param _module_const_source Constant module structure.
 * @param _source Source structure (dynamic or constant).
 * @param ... String (with 1 argument).
 */
#define Z_LOG_STMESP_1(_level, _source, ...)                                                       \
	do {                                                                                       \
		/* Do turbo logging only if argument fits in 32 bit word. */                       \
		if (!Z_LOG_STMESP_1_ARG_CHECK(__VA_ARGS__)) {                                      \
			COND_CODE_1(CONFIG_LOG_FRONTEND_STMESP_TURBO_DROP_OTHERS, (),              \
				(Z_LOG(_level, __VA_ARGS__)));                                     \
			break;                                                                     \
		}                                                                                  \
		if (!Z_LOG_LEVEL_ALL_CHECK(_level, __log_current_const_data, _source)) {           \
			break;                                                                     \
		}                                                                                  \
		LOG_FRONTEND_STMESP_LOG1(_source, STRINGIFY(_level) __VA_ARGS__, dummy);           \
	} while (0)

/** @brief Top level logging macro.
 *
 * Macro is using special approach for short log message (0 or 1 numeric argument)
 * and proceeds with standard approach (or optionally drops) for remaining messages.
 *
 * @param _level Severity level.
 * @param _source Pointer to a structure associated with the source.
 * @param ... String with arguments.
 */
#define Z_LOG_STMESP(_level, _source, ...)                                                         \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),                                     \
		(Z_LOG_STMESP_0(_level, _source, __VA_ARGS__)), (                        \
		COND_CODE_1(NUM_VA_ARGS_LESS_1(__VA_ARGS__),                             \
			(Z_LOG_STMESP_1(_level, _source, __VA_ARGS__)),                  \
			(                                                                \
			if (!IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_TURBO_DROP_OTHERS)) { \
				Z_LOG(_level, __VA_ARGS__);                              \
			}))))

/* Overridden logging API macros. */
#define LOG_ERR(...) Z_LOG_STMESP(LOG_LEVEL_ERR, Z_LOG_CURRENT_DATA(), __VA_ARGS__)
#define LOG_WRN(...) Z_LOG_STMESP(LOG_LEVEL_WRN, Z_LOG_CURRENT_DATA(), __VA_ARGS__)
#define LOG_INF(...) Z_LOG_STMESP(LOG_LEVEL_INF, Z_LOG_CURRENT_DATA(), __VA_ARGS__)
#define LOG_DBG(...) Z_LOG_STMESP(LOG_LEVEL_DBG, Z_LOG_CURRENT_DATA(), __VA_ARGS__)

#define LOG_INST_ERR(_inst, ...) Z_LOG_STMESP(LOG_LEVEL_ERR, Z_LOG_INST(_inst), __VA_ARGS__)
#define LOG_INST_WRN(_inst, ...) Z_LOG_STMESP(LOG_LEVEL_WRN, Z_LOG_INST(_inst), __VA_ARGS__)
#define LOG_INST_INF(_inst, ...) Z_LOG_STMESP(LOG_LEVEL_INF, Z_LOG_INST(_inst), __VA_ARGS__)
#define LOG_INST_DBG(_inst, ...) Z_LOG_STMESP(LOG_LEVEL_DBG, Z_LOG_INST(_inst), __VA_ARGS__)

#if CONFIG_LOG_FRONTEND_STMESP_TURBO_DROP_OTHERS
#undef LOG_RAW
#undef LOG_PRINTK
#undef LOG_HEXDUMP_ERR
#undef LOG_HEXDUMP_WRN
#undef LOG_HEXDUMP_INF
#undef LOG_HEXDUMP_DBG
#undef LOG_INST_HEXDUMP_ERR
#undef LOG_INST_HEXDUMP_WRN
#undef LOG_INST_HEXDUMP_INF
#undef LOG_INST_HEXDUMP_DBG

#define LOG_RAW(...)                                                                               \
	do {                                                                                       \
		if (0) {                                                                           \
			Z_LOG_PRINTK(1, __VA_ARGS__);                                              \
		}                                                                                  \
	} while (0)

#define LOG_PRINTK(...)                                                                            \
	do {                                                                                       \
		if (0) {                                                                           \
			Z_LOG_PRINTK(1, __VA_ARGS__);                                              \
		}                                                                                  \
	} while (0)

#define LOG_HEXDUMP_ERR(_data, _length, _str)             (void)_data
#define LOG_HEXDUMP_WRN(_data, _length, _str)             (void)_data
#define LOG_HEXDUMP_INF(_data, _length, _str)             (void)_data
#define LOG_HEXDUMP_DBG(_data, _length, _str)             (void)_data
#define LOG_INST_HEXDUMP_ERR(_inst, _data, _length, _str) (void)_data
#define LOG_INST_HEXDUMP_WRN(_inst, _data, _length, _str) (void)_data
#define LOG_INST_HEXDUMP_INF(_inst, _data, _length, _str) (void)_data
#define LOG_INST_HEXDUMP_DBG(_inst, _data, _length, _str) (void)_data
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOC_NORDIC_COMMON_ZEPHYR_CUSTOM_LOG_H_ */
