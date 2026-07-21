/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *  Copyright (c) 2026 Picoheart Inc.
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

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
	 * In RISC-V (arch/riscv/core/isr.S) the esf is built on the
	 * interrupted thread's stack; a pointer to it is saved at
	 * irq_stack - 16 before the core switches to the IRQ stack.
	 *
	 * arch_stack_walk() starts from esf->mepc / esf->s0 and walks the
	 * frame chain, emitting esf->mepc and esf->ra first, then each
	 * frame's return address. Each frame is validated against the
	 * thread stack bounds and the text region, so the hand-rolled
	 * unwinder that used to live here is no longer needed.
	 */
	const struct arch_esf *esf =
		*((struct arch_esf **)(((uintptr_t)_current_cpu->irq_stack) - 16));

	struct perf_trace_ctx ctx = {.buf = buf, .idx = 0, .size = size};

	arch_stack_walk(perf_trace_cb, &ctx, _current, esf);

	return ctx.idx;
}
