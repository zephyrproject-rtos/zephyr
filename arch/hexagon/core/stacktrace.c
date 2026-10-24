/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/exception.h>
#include <zephyr/kernel_structs.h>
#include <switch_frame.h>

#define MAX_STACK_FRAMES CONFIG_ARCH_STACKWALK_MAX_FRAMES

static bool in_stack_bound(uintptr_t addr, size_t length,
			   const struct k_thread *thread)
{
#ifdef CONFIG_THREAD_STACK_INFO
	uintptr_t start;
	size_t size;

	if (thread == NULL) {
		return false;
	}

	start = thread->stack_info.start;
	size = thread->stack_info.size;
	return (addr >= start) && (addr - start <= size) &&
		(length <= size - (addr - start));
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(length);
	ARG_UNUSED(thread);
	return true;
#endif
}

static bool in_stack_frame(uintptr_t addr, const struct k_thread *thread)
{
	return in_stack_bound(addr, 2U * sizeof(uint32_t), thread);
}

static void walk_stackframe(stack_trace_callback_fn callback_fn, void *cookie,
			    const struct k_thread *thread, uintptr_t fp, uintptr_t lr)
{
	for (uint32_t i = 0U; i < MAX_STACK_FRAMES; i++) {
		if (lr == 0U || !callback_fn(cookie, lr)) {
			return;
		}

		if (fp == 0U || !in_stack_frame(fp, thread)) {
			return;
		}

		const uint32_t *frame = (const uint32_t *)fp;
		lr = frame[1];
		uintptr_t next_fp = frame[0];

		if (next_fp <= fp) {
			return;
		}
		fp = next_fp;
	}
}

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
			     const struct k_thread *thread, const struct arch_esf *esf)
{
	uintptr_t fp;
	uintptr_t lr;

	if (callback_fn == NULL) {
		return;
	}

	if (esf != NULL) {
		fp = esf->r30_fp;
		lr = esf->pc;
	} else if ((thread == NULL) || (thread == _current)) {
		__asm__ volatile("%[fp] = r30" : [fp] "=r"(fp));
		__asm__ volatile("%[lr] = r31" : [lr] "=r"(lr));
		thread = _current;
	} else {
		uintptr_t frame_addr = (uintptr_t)thread->switch_handle;
		const uint32_t *frame;

		if (!in_stack_bound(frame_addr, SWITCH_FRAME_SIZE, thread)) {
			return;
		}

		frame = (const uint32_t *)frame_addr;
		fp = frame[SWITCH_FP_LR / 4];
		lr = frame[SWITCH_FP_LR / 4 + 1];
	}

	walk_stackframe(callback_fn, cookie, thread, fp, lr);
}
