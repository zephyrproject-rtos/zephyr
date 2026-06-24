/*
 *  Copyright (c) 2026 Picoheart Inc.
 *  Copyright (c) 2026 LiuQian.andy <liuqian.andy@picoheart.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/exception.h>

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

size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	if (size < 2U) {
		return 0;
	}

	/*
	 * Recover the interrupted thread's exception stack frame.
	 *
	 * In ARM64 (arch/arm64/core/vector_table.S) every exception vector --
	 * including IRQ -- runs z_arm64_enter_exc first, which builds a full
	 * arch_esf (x0-x18, lr, fp, spsr, elr) on the interrupted thread's
	 * stack, then branches to _isr_wrapper. _isr_wrapper switches to the
	 * IRQ stack and saves the interrupted SP at [irq_stack - 16].
	 *
	 * arch_stack_walk() starts from esf->fp and walks the frame chain,
	 * emitting each frame's return address (fp[1]). It does NOT emit
	 * esf->elr (the faulting PC) or esf->lr, so those are emitted
	 * explicitly first; esf->lr covers the prologue/epilogue case where
	 * the frame chain is not yet established.
	 */
	const struct arch_esf *esf =
		*((struct arch_esf **)(((uintptr_t)_current_cpu->irq_stack) - 16));

	struct perf_trace_ctx ctx = {.buf = buf, .idx = 0, .size = size};

	buf[ctx.idx++] = (uintptr_t)esf->elr;
	buf[ctx.idx++] = (uintptr_t)esf->lr;

	arch_stack_walk(perf_trace_cb, &ctx, _current, esf);

	return ctx.idx;
}
