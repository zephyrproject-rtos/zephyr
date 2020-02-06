/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <tracing_buffer.h>
#include <tracing_format_common.h>

static int str_put(int c, void *ctx)
{
	tracing_ctx_t *str_ctx = (tracing_ctx_t *)ctx;

	if (str_ctx->status == 0) {
		u8_t *buf;
		u32_t claimed_size;

		claimed_size = tracing_buffer_put_claim(&buf, 1);
		if (claimed_size) {
			*buf = (u8_t)c;
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

#if !defined(CONFIG_NEWLIB_LIBC) && !defined(CONFIG_ARCH_POSIX)
	(void)z_prf(str_put, (void *)&str_ctx, (char *)str, args);
#else
	z_vprintk(str_put, (void *)&str_ctx, str, args);
#endif

	if (str_ctx.status == 0) {
		tracing_buffer_put_finish(str_ctx.length);
		return true;
	}

	tracing_buffer_put_finish(0);
	return false;
}

bool tracing_format_raw_data_put(u8_t *data, u32_t size)
{
	u32_t space = tracing_buffer_space_get();

	if (space >= size) {
		tracing_buffer_put(data, size);
		return true;
	}

	return false;
}

bool tracing_format_data_put(tracing_data_t *tracing_data_array, u32_t count)
{
	u32_t total_size = 0U;

	for (u32_t i = 0; i < count; i++) {
		tracing_data_t *tracing_data =
				tracing_data_array + i;
		u8_t *data = tracing_data->data, *buf;
		u32_t length = tracing_data->length, claimed_size;

		do {
			claimed_size = tracing_buffer_put_claim(&buf, length);
			memcpy(buf, data, claimed_size);
			total_size += claimed_size;
			length -= claimed_size;
			data += claimed_size;
		} while (length && claimed_size);

		if (length && claimed_size == 0) {
			tracing_buffer_put_finish(0);
			return false;
		}
	}

	tracing_buffer_put_finish(total_size);
	return true;
}
