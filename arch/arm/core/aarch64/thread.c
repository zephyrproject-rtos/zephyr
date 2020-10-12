/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM64 Cortex-A
 *
 * Core thread related primitives for the ARM64 Cortex-A
 */

#include <kernel.h>
#include <ksched.h>
#include <wait_q.h>
#include <arch/cpu.h>

struct init_stack_frame {
	/* top of the stack / most recently pushed */

	/* SPSR_ELn and ELR_ELn */
	uint64_t spsr;
	uint64_t elr;

	/*
	 * Registers restored by z_arm64_exit_exc(). We are not interested in
	 * registers x4 -> x18 + x30 but we need to account for those anyway
	 */
	uint64_t unused[16];

	/*
	 * z_arm64_exit_exc() pulls these off the stack and into argument
	 * registers before calling z_thread_entry():
	 * -  x2 <- arg2
	 * -  x3 <- arg3
	 * -  x0 <- entry_point
	 * -  x1 <- arg1
	 */
	uint64_t arg2;
	uint64_t arg3;
	uint64_t entry_point;
	uint64_t arg1;

	/* least recently pushed */
};

/*
 * An initial context, to be "restored" by z_arm64_context_switch(), is put at
 * the other end of the stack, and thus reusable by the stack when not needed
 * anymore.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct init_stack_frame *pInitCtx;

	pInitCtx = Z_STACK_PTR_TO_FRAME(struct init_stack_frame, stack_ptr);

	pInitCtx->entry_point = (uint64_t)entry;
	pInitCtx->arg1 = (uint64_t)p1;
	pInitCtx->arg2 = (uint64_t)p2;
	pInitCtx->arg3 = (uint64_t)p3;

	/*
	 * - ELR_ELn: to be used by eret in z_arm64_exit_exc() to return
	 *   to z_thread_entry() with entry in x0(entry_point) and the
	 *   parameters already in place in x1(arg1), x2(arg2), x3(arg3).
	 * - SPSR_ELn: to enable IRQs (we are masking FIQs).
	 */
	pInitCtx->elr = (uint64_t)z_thread_entry;
	pInitCtx->spsr = SPSR_MODE_EL1H | DAIF_FIQ;

	/*
	 * We are saving SP to pop out entry and parameters when going through
	 * z_arm64_exit_exc()
	 */
	thread->callee_saved.sp = (uint64_t)pInitCtx;

	thread->switch_handle = thread;
}

void *z_arch_get_next_switch_handle(struct k_thread **old_thread)
{
	*old_thread =  _current;

	return z_get_next_switch_handle(*old_thread);
}
