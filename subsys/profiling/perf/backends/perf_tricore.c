/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/sys/util.h>

#include <kernel_arch_data.h>

/* PCXI recorded by _isr_wrapper() on entry, per nesting level. */
uint32_t z_tricore_perf_pcxi[CONFIG_PROFILING_PERF_TRICORE_MAX_NESTING];

struct perf_trace_ctx {
	uintptr_t *buf;
	size_t idx;
	size_t size;
};

static bool perf_trace_cb(void *cookie, unsigned long addr)
{
	struct perf_trace_ctx *ctx = cookie;

	if (ctx->idx >= ctx->size) {
		return false;
	}

	ctx->buf[ctx->idx++] = (uintptr_t)addr;
	return true;
}

int arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	struct perf_trace_ctx ctx = {.buf = buf, .idx = 0U, .size = size};
	const struct arch_esf *esf;
	uint32_t depth;

	if (size == 0U) {
		return 0;
	}

	depth = arch_curr_cpu()->nested;
	if (depth == 0U || depth > CONFIG_PROFILING_PERF_TRICORE_MAX_NESTING) {
		return -EAGAIN;
	}

	esf = UINT_TO_POINTER(z_tricore_pcx_to_addr(z_tricore_perf_pcxi[depth - 1U]));

	/*
	 * A nested ISR interrupted another ISR; the chain above it is not a
	 * thread call chain, so keep only the interrupted PC.
	 */
	ctx.size = (depth > 1U) ? 1U : size;

	arch_stack_walk(perf_trace_cb, &ctx, _current, esf);

	/* Zero is reserved for "buffer full"; an empty walk is a skipped sample. */
	if (ctx.idx == 0U) {
		return -EAGAIN;
	}

	return (int)ctx.idx;
}
