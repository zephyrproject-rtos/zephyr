/* Copyright (c) 2024 Contributors to the logging subsystem.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stddef.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/arch/common/semihost.h>

static uint8_t buf[CONFIG_LOG_BACKEND_SEMIHOST_BUFFER_SIZE];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_SEMIHOST_OUTPUT_DEFAULT;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
#define SEMIHOST_STDOUT (1)
	int ret = semihost_write(SEMIHOST_STDOUT, data, length);

	if (ret) {
		return ret;
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_semihost, char_out, buf, sizeof(buf));

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	log_output_flush(&log_output_semihost);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_output_dropped_process(&log_output_semihost, cnt);
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	ARG_UNUSED(backend);

	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_semihost, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);

	log_format_current = log_type;
	return 0;
}

const struct log_backend_api log_backend_semihost_api = {
	.process = process,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_semihost, log_backend_semihost_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_SEMIHOST_AUTOSTART));
