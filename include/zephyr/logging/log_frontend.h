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
		      const struct log_msg_desc desc,
		      uint8_t *package, const void *data);

/** @brief Panic state notification. */
void log_frontend_panic(void);

#endif /* LOG_FRONTEND_H_ */
