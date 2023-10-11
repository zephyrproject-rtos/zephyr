/*
 * Copyright (c) 2021 Converge
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_CUSTOM_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_CUSTOM_H_

#include <zephyr/logging/log_output.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Custom logging output formatting.
 * @ingroup log_output
 * @{
 */

/** @brief Process log messages from an external output function set with
 * log_custom_output_msg_set
 *
 * Function is using provided context with the buffer and output function to
 * process formatted string and output the data.
 *
 * @param log_output Pointer to the log output instance.
 * @param msg Log message.
 * @param flags Optional flags.
 */
void log_custom_output_msg_process(const struct log_output *log_output,
				 struct log_msg *msg, uint32_t flags);

/** @brief Set the formatting log function that will be applied with LOG_OUTPUT_CUSTOM
 *
 * @param format Pointer to the external formatter function
 */
void log_custom_output_msg_set(log_format_func_t format);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_CUSTOM_H_ */
