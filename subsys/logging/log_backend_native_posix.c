/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <logging/log_backend_native_posix.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <device.h>
#include <uart.h>

static u8_t buf[2048];

int char_out(u8_t *data, size_t length, void *ctx)
{
	for (size_t i = 0; i < length; i++) {
		putchar(data[i]);
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output, char_out, buf, 1);

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_msg_get(msg);

	u32_t flags = 0;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		if (posix_trace_over_tty(0)) {
			flags |= LOG_OUTPUT_FLAG_COLORS;
		}
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(&log_output, msg, flags);

	log_msg_put(msg);

}

static void panic(struct log_backend const *const backend)
{
	/* Nothing to be done, this backend can always process logs */
}

const struct log_backend_api log_backend_native_posix_api = {
	.put = put,
	.panic = panic,
};
