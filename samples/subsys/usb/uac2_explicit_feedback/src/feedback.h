/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FEEDBACK_H_
#define FEEDBACK_H_

#include <stdint.h>

/* This sample is currently supporting only 48 kHz sample rate. */
#define SAMPLE_RATE         48000

struct feedback_ctx *feedback_init(void);
void feedback_reset_ctx(struct feedback_ctx *ctx, bool microframes);
void feedback_process(struct feedback_ctx *ctx);
void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued,
		    bool microframes);
uint32_t feedback_value(struct feedback_ctx *ctx);
#define SAMPLES_PER_SOF 48
#endif /* FEEDBACK_H_ */
