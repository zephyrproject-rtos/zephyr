/*
 * Copyright (c) 2022 Converge
 * Copyright (c) 2023 Nobleo Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_custom.h>

static log_format_func_t log_custom_format_func;

void log_custom_output_msg_process(const struct log_output *output, struct log_msg *msg,
				   uint32_t flags)
{
	if (log_custom_format_func) {
		log_custom_format_func(output, msg, flags);
	}
}

void log_custom_output_msg_set(log_format_func_t format)
{
	log_custom_format_func = format;
}

static log_timestamp_format_func_t log_timestamp_format_func;

int log_custom_timestamp_print(const struct log_output *output, const log_timestamp_t timestamp,
			      const log_timestamp_printer_t printer)
{
	__ASSERT(log_timestamp_format_func != NULL, "custom timestamp format function not set");

	if (log_timestamp_format_func) {
		return log_timestamp_format_func(output, timestamp, printer);
	}

	return 0;
}

void log_custom_timestamp_set(log_timestamp_format_func_t format)
{
	log_timestamp_format_func = format;
}
