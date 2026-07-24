/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CVSD_H_
#define CVSD_H_

#include <stddef.h>
#include <stdint.h>

struct cvsd_ctx {
	int32_t accum;
	int16_t prev;
};

void cvsd_init(struct cvsd_ctx *ctx);
void cvsd_decode(struct cvsd_ctx *ctx, const uint8_t *in, size_t in_len, int16_t *out);
void cvsd_encode(struct cvsd_ctx *ctx, const int16_t *in, size_t samples, uint8_t *out);

#endif /* CVSD_H_ */
