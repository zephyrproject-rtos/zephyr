/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FEEDBACK_H_
#define FEEDBACK_H_

#include <stdint.h>

/* Nominal number of samples received on each SOF. This sample is currently
 * supporting only 48 kHz sample rate.
 */
#define SAMPLES_PER_SOF     48

struct feedback_ctx *feedback_init(void);
void feedback_reset_ctx(struct feedback_ctx *ctx);
void feedback_process(struct feedback_ctx *ctx);
void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued);
uint32_t feedback_value(struct feedback_ctx *ctx);

#endif /* FEEDBACK_H_ */
