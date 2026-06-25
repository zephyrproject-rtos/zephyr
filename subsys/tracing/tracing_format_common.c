/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/sys/ring_buffer.h>
#include <tracing_buffer.h>
#include <tracing_format_common.h>

static int str_put(int c, void *ctx)
{
	tracing_ctx_t *str_ctx = (tracing_ctx_t *)ctx;
	struct ring_buf *rb = &str_ctx->rb;

	if (str_ctx->status == 0) {
		uint8_t *buf;
		uint32_t claimed_size;

		claimed_size = ring_buf_put_ptr(rb, &buf);
		if (claimed_size) {
			*buf = (uint8_t)c;
			str_ctx->length++;
			ring_buf_commit(rb, 1);
		} else {
			str_ctx->status = -1;
		}
	}

	return 0;
}

bool tracing_format_string_put(const char *str, va_list args)
{
	struct ring_buf *rb = tracing_buffer_get_ring_buf();
	tracing_ctx_t str_ctx = {0};

	str_ctx.rb = ring_buf_snapshot(rb);

	(void)cbvprintf(str_put, (void *)&str_ctx, str, args);

	if (str_ctx.status == 0) {
		ring_buf_commit(rb, str_ctx.length);
		return true;
	}

	return false;
}

bool tracing_format_raw_data_put(uint8_t *data, uint32_t size)
{
	struct ring_buf *rb = tracing_buffer_get_ring_buf();
	uint32_t space = ring_buf_space_get(rb);

	if (space >= size) {
		ring_buf_put(rb, data, size);
		return true;
	}

	return false;
}

bool tracing_format_data_put(tracing_data_t *tracing_data_array, uint32_t count)
{
	struct ring_buf rb = ring_buf_snapshot(tracing_buffer_get_ring_buf());
	uint32_t total_size = 0U;

	for (uint32_t i = 0; i < count; i++) {
		tracing_data_t *tracing_data =
				tracing_data_array + i;
		uint8_t *data = tracing_data->data, *buf;
		uint32_t claimed_size;
		uint32_t length = tracing_data->length;

		do {
			claimed_size = MIN(ring_buf_put_ptr(&rb, &buf), length);
			memcpy(buf, data, claimed_size);
			total_size += claimed_size;
			length -= claimed_size;
			data += claimed_size;
			ring_buf_commit(&rb, claimed_size);
		} while (length && claimed_size);

		if (length && claimed_size == 0) {
			return false;
		}
	}

	ring_buf_commit(tracing_buffer_get_ring_buf(), total_size);
	return true;
}
