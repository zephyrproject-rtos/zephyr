/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * Log message type
 */
enum log_dict_output_msg_type {
	MSG_NORMAL = 0,
	MSG_DROPPED_MSG = 1,
};

/**
 * Output header for one dictionary based log message.
 */
struct log_dict_output_normal_msg_hdr_t {
	uint8_t type;
	uint32_t domain:3;
	uint32_t level:3;
	uint32_t package_len:10;
	uint32_t data_len:12;
	uintptr_t source;
	log_timestamp_t timestamp;
} __packed;

/**
 * Output for one dictionary based log message about
 * dropped messages.
 */
struct log_dict_output_dropped_msg_t {
	uint8_t type;
	uint16_t num_dropped_messages;
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
void log_dict_output_msg2_process(const struct log_output *log_output,
				  struct log_msg2 *msg, uint32_t flags);

/** @brief Process dropped messages indication for dictionary-based logging.
 *
 * Function prints error message indicating lost log messages.
 *
 * @param output Pointer to the log output instance.
 * @param cnt        Number of dropped messages.
 */
void log_dict_output_dropped_process(const struct log_output *output, uint32_t cnt);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_DICT_H_ */
