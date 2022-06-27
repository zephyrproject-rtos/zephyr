/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_FRONTEND_H_
#define LOG_FRONTEND_H_

#include <zephyr/logging/log_core.h>

/** @brief Initialize frontend.
 */
void log_frontend_init(void);

/** @brief Standard log with no arguments.
 *
 * @param str		String.
 * @param src_level	Log identification.
 */
void log_frontend_0(const char *str, struct log_msg_ids src_level);

/** @brief Standard log with one argument.
 *
 * @param str		String.
 * @param arg0		First argument.
 * @param src_level	Log identification.
 */
void log_frontend_1(const char *str,
		    log_arg_t arg0,
		    struct log_msg_ids src_level);

/** @brief Standard log with two arguments.
 *
 * @param str		String.
 * @param arg0		First argument.
 * @param arg1		Second argument.
 * @param src_level	Log identification.
 */
void log_frontend_2(const char *str,
		    log_arg_t arg0,
		    log_arg_t arg1,
		    struct log_msg_ids src_level);

/** @brief Standard log with three arguments.
 *
 * @param str		String.
 * @param arg0		First argument.
 * @param arg1		Second argument.
 * @param arg2		Third argument.
 * @param src_level	Log identification.
 */
void log_frontend_3(const char *str,
		    log_arg_t arg0,
		    log_arg_t arg1,
		    log_arg_t arg2,
		    struct log_msg_ids src_level);

/** @brief Standard log with arguments list.
 *
 * @param str		String.
 * @param args		Array with arguments.
 * @param narg		Number of arguments in the array.
 * @param src_level	Log identification.
 */
void log_frontend_n(const char *str,
		    log_arg_t *args,
		    uint32_t narg,
		    struct log_msg_ids src_level);

/** @brief Hexdump log.
 *
 * @param str		String.
 * @param data		Data.
 * @param length	Data length.
 * @param src_level	Log identification.
 */
void log_frontend_hexdump(const char *str,
			  const uint8_t *data,
			  uint32_t length,
			  struct log_msg_ids src_level);

/** @brief Log message.
 *
 * Message details does not contain timestamp. Since function is called in the
 * context of log message call, implementation can use its own timestamping scheme.
 *
 * @param source Pointer to a structure associated with given source. It points to
 * static structure or dynamic structure if runtime filtering is enabled.
 * @ref log_const_source_id or @ref log_dynamic_source_id can be used to determine
 * source id.
 *
 * @param desc Message descriptor.
 *
 * @param package Cbprintf package containing logging formatted string. Length s in @p desc.
 *
 * @param data Hexdump data. Length is in @p desc.
 */
void log_frontend_msg(const void *source,
		      const struct log_msg2_desc desc,
		      uint8_t *package, const void *data);

/** @brief Panic state notification. */
void log_frontend_panic(void);

#endif /* LOG_FRONTEND_H_ */
