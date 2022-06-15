/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/irq.h>
#include <zephyr/arch/posix/posix_trace.h>

#define _STDOUT_BUF_SIZE 256
static char stdout_buff[_STDOUT_BUF_SIZE];
static int n_pend; /* Number of pending characters in buffer */
static uint32_t log_format_current = CONFIG_LOG_BACKEND_NATIVE_POSIX_OUTPUT_DEFAULT;

static void preprint_char(int c)
{
	int printnow = 0;

	if (c == '\r') {
		/* Discard carriage returns */
		return;
	}
	if (c != '\n') {
		stdout_buff[n_pend++] = c;
		stdout_buff[n_pend] = 0;
	} else {
		printnow = 1;
	}

	if (n_pend >= _STDOUT_BUF_SIZE - 1) {
		printnow = 1;
	}

	if (printnow) {
		posix_print_trace("%s\n", stdout_buff);
		n_pend = 0;
		stdout_buff[0] = 0;
	}
}

static uint8_t buf[_STDOUT_BUF_SIZE];

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	for (size_t i = 0; i < length; i++) {
		preprint_char(data[i]);
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_posix, char_out, buf, sizeof(buf));

static void panic(struct log_backend const *const backend)
{
	log_output_flush(&log_output_posix);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_output_dropped_process(&log_output_posix, cnt);
}

static void process(const struct log_backend *const backend,
		union log_msg2_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_posix, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

const struct log_backend_api log_backend_native_posix_api = {
	.process = process,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_native_posix,
		   log_backend_native_posix_api,
		   true);
