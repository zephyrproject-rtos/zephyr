/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/thread.h>
#include <string.h>
#include <kernel_internal.h>
#include <switch_frame.h>

extern void z_hexagon_thread_start(void);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	uintptr_t stack_end = ROUND_DOWN((uintptr_t)stack_ptr, ARCH_STACK_PTR_ALIGN);

	/*
	 * Build a fake switch frame on the stack so that the first
	 * z_hexagon_arch_switch() into this thread restores callee-saved
	 * registers and does jumpr r31 to z_hexagon_thread_start.
	 *
	 * Layout (growing downward):
	 *
	 *   stack_end
	 *     [SWITCH_FP_LR]       +0x30  : r30(0) | r31(thread_start)
	 *     ...callee-saved...
	 *     [SWITCH_R1716]       +0x00  : r16(entry) | r17(p1)
	 *   frame_base  <-- switch_handle (== new SP after switch)
	 */
	uintptr_t frame_base = ROUND_DOWN(stack_end - SWITCH_FRAME_SIZE, 8);
	uint32_t *frame = (uint32_t *)frame_base;

	memset(frame, 0, SWITCH_FRAME_SIZE);

	/* Callee-saved register slots: r16=entry, r17=p1, r18=p2, r19=p3 */
	frame[SWITCH_R1716 / 4]     = (uint32_t)entry;  /* r16 */
	frame[SWITCH_R1716 / 4 + 1] = (uint32_t)p1;     /* r17 */
	frame[SWITCH_R1918 / 4]     = (uint32_t)p2;      /* r18 */
	frame[SWITCH_R1918 / 4 + 1] = (uint32_t)p3;      /* r19 */

	/*
	 * SWITCH_FP_LR slot:
	 *   r30 (FP) = 0 (no parent frame -- terminates backtraces)
	 *   r31 (LR) = z_hexagon_thread_start (where jumpr r31 will go)
	 */
	frame[SWITCH_FP_LR / 4]     = 0;                                /* r30 */
	frame[SWITCH_FP_LR / 4 + 1] = (uint32_t)z_hexagon_thread_start; /* r31 */

	thread->switch_handle = (void *)frame_base;
}

char *arch_k_thread_stack_buffer(k_thread_stack_t *stack)
{
	return (char *)stack;
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);
	return -ENOTSUP;
}
