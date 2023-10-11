/*
 * Copyright (c) 2022 Converge
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_output.h>

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
