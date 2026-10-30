/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>

struct perf_trace_ctx {
	uintptr_t *buf;
	size_t idx;
	size_t size;
	bool full;
};

static bool perf_trace_cb(void *cookie, unsigned long addr)
{
	struct perf_trace_ctx *ctx = cookie;

	if (ctx->idx >= ctx->size) {
		ctx->full = true;
		return false;
	}

	ctx->buf[ctx->idx++] = (uintptr_t)addr;
	return true;
}

int arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	struct perf_trace_ctx ctx = {.buf = buf, .idx = 0U, .size = size};

	if (size == 0U) {
		return 0;
	}

	if (!k_is_in_isr()) {
		return -EAGAIN;
	}

	arch_stack_walk(perf_trace_cb, &ctx, _current, NULL);

	/* Zero is reserved for "buffer full"; an empty walk is a skipped sample. */
	if (ctx.full) {
		return 0;
	}

	if (ctx.idx == 0U) {
		return -EAGAIN;
	}

	return (int)ctx.idx;
}
