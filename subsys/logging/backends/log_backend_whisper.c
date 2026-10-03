/*
 * Copyright (c) 2019, 2026 Intel Corporation Inc.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>

#define CONSOLE_OUT_ADDR (DT_REG_ADDR(DT_CHOSEN(zephyr_console)))

#define CHAR_BUF_SIZE                                                                              \
	(IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? 1 : CONFIG_LOG_BACKEND_WHISPER_OUTPUT_BUFFER_SIZE)

static uint8_t whisper_log_buf[CHAR_BUF_SIZE];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_WHISPER_OUTPUT_DEFAULT;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	for (int i = 0; i < length; i++) {
		sys_write32((uint32_t)data[i], CONSOLE_OUT_ADDR);

		/* Make sure register write is done before proceeding. */
		__asm__ volatile("fence iorw,iorw");
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_whisper, char_out, whisper_log_buf, sizeof(whisper_log_buf));

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_whisper, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output_whisper);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_whisper, cnt);
}

const struct log_backend_api log_backend_whisper_api = {
	.process = process,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_whisper, log_backend_whisper_api, true);
