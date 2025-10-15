/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/cbprintf.h>
#include <tracing_buffer.h>
#include <tracing_format_common.h>

static int str_put(int c, void *ctx)
{
	tracing_ctx_t *str_ctx = (tracing_ctx_t *)ctx;

	if (str_ctx->status == 0) {
		if (tracing_buffer_put((uint8_t *)&c, 1)) {
			str_ctx->length++;
		} else {
			str_ctx->status = -1;
		}
	}

	return 0;
}

bool tracing_format_string_put(const char *str, va_list args)
{
	tracing_ctx_t str_ctx = {0};

	(void)cbvprintf(str_put, (void *)&str_ctx, str, args);

	if (str_ctx.status == 0) {
		return true;
	}

	return false;
}

bool tracing_format_raw_data_put(uint8_t *data, uint32_t size)
{
	uint32_t space = tracing_buffer_space_get();

	if (space >= size) {
		tracing_buffer_put(data, size);
		return true;
	}

	return false;
}

bool tracing_format_data_put(tracing_data_t *tracing_data_array, uint32_t count)
{
	uint32_t write_size = 0;

	for (size_t i = 0; i < count; i++) {
		write_size += tracing_data_array[i].length;
	}

	if (tracing_buffer_space_get() < write_size) {
		return false;
	}

	for (uint32_t i = 0; i < count; i++) {
		tracing_buffer_put(tracing_data_array[i].data, tracing_data_array[i].length);
	}

	return true;
}
