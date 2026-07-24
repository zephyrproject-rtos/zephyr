/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <cortex_m/exception.h>
#include <cortex_m/stack.h>

#define PERF_CORTEX_M_XPSR_T_Msk BIT(24)

struct perf_cortex_m_sample {
	const struct arch_esf *esf;
	uint32_t exc_return;
	uint32_t fp;
	bool active;
};

struct perf_cortex_m_trace {
	uintptr_t *buf;
	size_t size;
	size_t count;
	bool overflow;
};

static volatile struct perf_cortex_m_sample perf_sample;

static bool in_text_region(uintptr_t addr)
{
	return (addr >= (uintptr_t)__text_region_start) && (addr < (uintptr_t)__text_region_end);
}

static bool range_contains(uintptr_t start, uintptr_t end, uintptr_t addr, size_t size)
{
	return (addr >= start) && (addr <= end) && (size <= (end - addr));
}

static bool thread_stack_range_contains(k_tid_t thread, uintptr_t addr, size_t size)
{
	uintptr_t start = thread->stack_info.start;
	uintptr_t end = start + thread->stack_info.size;

	return range_contains(start, end, addr, size);
}

static bool interrupt_stack_range_contains(uintptr_t addr, size_t size)
{
	unsigned int cpu = arch_curr_cpu()->id;
	uintptr_t start = (uintptr_t)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu]);
	uintptr_t end = start + K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[cpu]);

	return range_contains(start, end, addr, size);
}

static bool valid_basic_frame(const struct arch_esf *esf)
{
	/* The hardware-stacked PC must be executable code, and xPSR.T must
	 * be set for a valid Cortex-M return state.
	 */
	return in_text_region((uintptr_t)esf->basic.pc) &&
	       ((esf->basic.xpsr & PERF_CORTEX_M_XPSR_T_Msk) != 0U);
}

void z_arm_perf_sample_enter(const struct arch_esf *esf, uint32_t exc_return, uint32_t fp)
{
	/* The SysTick wrapper records the interrupted context before the
	 * normal timer ISR uses the handler stack.
	 */
	perf_sample.esf = esf;
	perf_sample.exc_return = exc_return;
	perf_sample.fp = fp;
	perf_sample.active = true;
}

void z_arm_perf_sample_exit(void)
{
	perf_sample.active = false;
	perf_sample.esf = NULL;
	perf_sample.exc_return = 0U;
	perf_sample.fp = 0U;
}

static bool add_trace_addr(void *arg, unsigned long addr)
{
	struct perf_cortex_m_trace *trace = arg;

	if (trace->count >= trace->size) {
		/* perf.c treats a zero-length trace as a full buffer, so report
		 * overflow by discarding this partial sample.
		 */
		trace->overflow = true;
		return false;
	}

	if (!in_text_region((uintptr_t)addr)) {
		return false;
	}

	trace->buf[trace->count++] = (uintptr_t)addr;

	return true;
}

static size_t unwind_stack(const struct arch_esf *esf, uint32_t exc_return, uint32_t fp,
			   uintptr_t *buf, size_t size)
{
	struct perf_cortex_m_trace trace = {
		.buf = buf,
		.size = size,
	};
	struct arch_esf unwind_esf = {
		.basic = esf->basic,
	};
	_callee_saved_t callee = {
		.v4 = fp,
		.psp = POINTER_TO_UINT(esf),
	};

	/* arch_stack_walk() expects the same extra ESF metadata used by the
	 * fault path. Recreate only the fields needed to unwind from the
	 * hardware exception frame captured on SysTick entry.
	 */
	unwind_esf.extra_info.callee = &callee;
	unwind_esf.extra_info.exc_return = exc_return;
	unwind_esf.extra_info.msp = POINTER_TO_UINT(esf);

	arch_stack_walk(add_trace_addr, &trace, _current, &unwind_esf);

	return trace.overflow ? 0U : trace.count;
}

static bool valid_sample_stack(const struct arch_esf *esf, uint32_t exc_return)
{
	if ((exc_return & EXC_RETURN_SPSEL_Msk) == EXC_RETURN_SPSEL_PROCESS) {
		return ((exc_return & EXC_RETURN_MODE_Msk) == EXC_RETURN_MODE_THREAD) &&
		       thread_stack_range_contains(_current, (uintptr_t)esf, sizeof(esf->basic)) &&
		       ((esf->basic.xpsr & IPSR_ISR_Msk) == 0U);
	}

	return ((exc_return & EXC_RETURN_MODE_Msk) == EXC_RETURN_MODE_HANDLER) &&
	       interrupt_stack_range_contains((uintptr_t)esf, sizeof(esf->basic)) &&
	       ((esf->basic.xpsr & IPSR_ISR_Msk) != 0U);
}

static size_t stack_trace_from_sample(uintptr_t *buf, size_t size)
{
	const struct arch_esf *esf = (const struct arch_esf *)perf_sample.esf;
	uint32_t exc_return = perf_sample.exc_return;
#ifdef CONFIG_FRAME_POINTER
	uint32_t fp = perf_sample.fp;
#endif

	if (!perf_sample.active || esf == NULL ||
	    ((exc_return & EXC_RETURN_INDICATOR_PREFIX) != EXC_RETURN_INDICATOR_PREFIX) ||
	    !IS_ALIGNED((uintptr_t)esf, sizeof(uint32_t)) ||
	    !valid_sample_stack(esf, exc_return) ||
	    !valid_basic_frame(esf)) {
		return 0;
	}

	return unwind_stack(esf, exc_return, fp, buf, size);
}

size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	if (size < 1U) {
		return 0;
	}

	return stack_trace_from_sample(buf, size);
}
