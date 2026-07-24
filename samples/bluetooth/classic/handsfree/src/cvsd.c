/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cvsd.h"

#include <zephyr/sys/util.h>

#define CVSD_STEP  25
#define CVSD_MIN   -32768
#define CVSD_MAX   32767

void cvsd_init(struct cvsd_ctx *ctx)
{
	ctx->accum = 0;
	ctx->prev = 0;
}

void cvsd_decode(struct cvsd_ctx *ctx, const uint8_t *in, size_t in_len, int16_t *out)
{
	for (size_t i = 0; i < in_len; i++) {
		uint8_t byte = in[i];

		for (int bit = 7; bit >= 0; bit--) {
			int32_t step = CVSD_STEP;
			int32_t diff = (byte & BIT(bit)) ? step : -step;

			ctx->accum += diff;
			ctx->accum = CLAMP(ctx->accum, CVSD_MIN, CVSD_MAX);

			*out++ = (int16_t)ctx->accum;
		}
	}
}

void cvsd_encode(struct cvsd_ctx *ctx, const int16_t *in, size_t samples, uint8_t *out)
{
	size_t byte_idx = 0;
	uint8_t byte = 0;

	for (size_t i = 0; i < samples; i++) {
		int16_t sample = in[i];
		int bit = (sample >= ctx->prev) ? 1 : 0;
		int32_t step = CVSD_STEP;
		int32_t diff = bit ? step : -step;

		ctx->accum += diff;
		ctx->accum = CLAMP(ctx->accum, CVSD_MIN, CVSD_MAX);

		ctx->prev = sample;
		byte |= (bit << (7 - (i & 7)));

		if ((i & 7) == 7) {
			out[byte_idx++] = byte;
			byte = 0;
		}
	}
}
