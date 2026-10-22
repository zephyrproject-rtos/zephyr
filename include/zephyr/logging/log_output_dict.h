/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for dictionary-based log output.
 * @ingroup log_output
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_DICT_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_DICT_H_

#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_msg.h>
#include <stdarg.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup log_output
 * @{
 */

/** @brief Dictionary output message type. */
enum log_dict_output_msg_type {
	MSG_NORMAL = 0,      /**< Normal log message. */
	MSG_DROPPED_MSG = 1, /**< Notification about dropped messages. */
};

/** @brief On-wire header for one dictionary-based log message. */
struct log_dict_output_normal_msg_hdr_t {
	uint8_t type;             /**< Message type, see @ref log_dict_output_msg_type. */
	uint32_t domain:4;        /**< Domain ID. */
	uint32_t level:4;         /**< Severity level. */
	uint32_t package_len:16;  /**< Length of the cbprintf package, in bytes. */
	uint32_t data_len:16;     /**< Length of the appended hexdump data, in bytes. */
	uintptr_t source;         /**< Address identifying the log source. */
	log_timestamp_t timestamp; /**< Message timestamp. */
} __packed;

/** @brief On-wire dictionary-based message reporting dropped messages. */
struct log_dict_output_dropped_msg_t {
	uint8_t type;                  /**< Message type, see @ref log_dict_output_msg_type. */
	uint16_t num_dropped_messages; /**< Number of dropped messages. */
} __packed;

/** @brief Process log messages v2 for dictionary-based logging.
 *
 * Function is using provided context with the buffer and output function to
 * process formatted string and output the data.
 *
 * @param log_output Pointer to the log output instance.
 * @param msg Log message.
 * @param flags Optional flags.
 */
void log_dict_output_msg_process(const struct log_output *log_output,
				 struct log_msg *msg, uint32_t flags);

/** @brief Process dropped messages indication for dictionary-based logging.
 *
 * Function prints error message indicating lost log messages.
 *
 * @param output Pointer to the log output instance.
 * @param cnt        Number of dropped messages.
 */
void log_dict_output_dropped_process(const struct log_output *output, uint32_t cnt);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_DICT_H_ */
