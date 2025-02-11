/*
 * Copyright (c) 2021 Converge
 * Copyright (c) 2023 Nobleo Technology
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
 * @brief Prototype of a printer function that can print the given timestamp
 * into a specific logger instance.
 *
 * Example usage:
 * @code{.c}
 * log_timestamp_printer_t *printer = ...;
 * printer(log_instance, "%02u:%02u", hours, minutes);
 * @endcode
 *
 * @param output The logger instance to write to
 * @param fmt The format string
 * @param ... optional arguments for the format string
 */
typedef int (*log_timestamp_printer_t)(const struct log_output *output, const char *fmt, ...);

/**
 * @brief Prototype of the function that will apply custom formatting
 * to a timestamp when LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP
 *
 * Example function:
 * @code{.c}
 * int custom_timestamp_formatter(const struct log_output* output,
 *                                const log_timestamp_t timestamp,
 *                                const log_timestamp_printer_t printer) {
 *     return printer(output, "%d ", timestamp);
 * }
 * @endcode
 *
 * @param output The logger instance to write to
 * @param timestamp
 * @param printer The printing function to use when formatting the timestamp.
 */
typedef int (*log_timestamp_format_func_t)(const struct log_output *output,
					   const log_timestamp_t timestamp,
					   const log_timestamp_printer_t printer);

/** @brief Format the timestamp with a external function.
 *
 * Function is using provided context with the buffer and output function to
 * process formatted string and output the data.
 *
 * @param output Pointer to the log output instance.
 * @param timestamp
 * @param printer The printing function to use when formatting the timestamp.
 */
int log_custom_timestamp_print(const struct log_output *output, const log_timestamp_t timestamp,
			      const log_timestamp_printer_t printer);

/** @brief Set the timestamp formatting function that will be applied
 * when LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP
 *
 * @param format Pointer to the external formatter function
 */
void log_custom_timestamp_set(log_timestamp_format_func_t format);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_OUTPUT_CUSTOM_H_ */
