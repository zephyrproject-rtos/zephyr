/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <string.h>
#include <soc.h>

void _thread_entry(k_thread_entry_t thread, void *arg1, void *arg2, void *arg3);

void _new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		size_t stack_size, k_thread_entry_t entry,
		void *arg1, void *arg2, void *arg3, int prio,
		unsigned int options)
{
	_ASSERT_VALID_PRIO(prio, entry);

	char *stack_memory = K_THREAD_STACK_BUFFER(stack);
	char *ctx = (char *)STACK_ROUND_DOWN(stack_memory + stack_size -
		sizeof(struct gpctx));
	struct gpctx *stack_init = (struct gpctx *)ctx;

	thread->callee_saved.sp = (u32_t)ctx;

	_new_thread_init(thread, stack_memory, stack_size, prio, options);

	stack_init->r[3] = (reg_t)entry;
	stack_init->r[4] = (reg_t)arg1;
	stack_init->r[5] = (reg_t)arg2;
	stack_init->r[6] = (reg_t)arg3;
	stack_init->r[27] = mips32_get_gp();
	stack_init->r[28] = (reg_t)ctx;

	/* core timer and sw0 interrupts default to on, EXL is
	 * also set as status will be written to this value
	 * inside an ISR
	 */

	stack_init->status = (reg_t)0x20008103;
	stack_init->cause = mips_getcr() & ~(CR_IP_MASK);
	stack_init->epc = (reg_t)_thread_entry;

	thread_monitor_init(thread);
}
