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

#ifdef CONFIG_USERSPACE
static bool is_user(struct k_thread *thread)
{
	return (thread->base.user_options & K_USER) != 0;
}
#endif

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	z_arch_esf_t *pInitCtx;

	pInitCtx = Z_STACK_PTR_TO_FRAME(struct __esf, stack_ptr);

	pInitCtx->x0 = (uint64_t)entry;
	pInitCtx->x1 = (uint64_t)p1;
	pInitCtx->x2 = (uint64_t)p2;
	pInitCtx->x3 = (uint64_t)p3;

	/*
	 * - ELR_ELn: to be used by eret in z_arm64_exit_exc() to return
	 *   to z_thread_entry() with entry in x0(entry_point) and the
	 *   parameters already in place in x1(arg1), x2(arg2), x3(arg3).
	 * - SPSR_ELn: to enable IRQs (we are masking FIQs).
	 */
#ifdef CONFIG_USERSPACE
	/*
	 * If the new thread is a user thread we jump into
	 * arch_user_mode_enter() when still in EL1.
	 */
	if (is_user(thread)) {
		pInitCtx->elr = (uint64_t)arch_user_mode_enter;
	} else {
		pInitCtx->elr = (uint64_t)z_thread_entry;
	}
#else
	pInitCtx->elr = (uint64_t)z_thread_entry;
#endif
	pInitCtx->spsr = SPSR_MODE_EL1T | DAIF_FIQ_BIT;

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

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	z_arch_esf_t *pInitCtx;
	uintptr_t stack_ptr;

	/* Reset the stack pointer to the base discarding any old context */
	stack_ptr = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				      _current->stack_info.size -
				      _current->stack_info.delta);

	/*
	 * Reconstruct the ESF from scratch to leverage the z_arm64_exit_exc()
	 * macro that will simulate a return from exception to move from EL1t
	 * to EL0t. On return we will be in userspace.
	 */
	pInitCtx = Z_STACK_PTR_TO_FRAME(struct __esf, stack_ptr);

	pInitCtx->spsr = DAIF_FIQ_BIT | SPSR_MODE_EL0T;
	pInitCtx->elr = (uint64_t)z_thread_entry;

	pInitCtx->x0 = (uint64_t)user_entry;
	pInitCtx->x1 = (uint64_t)p1;
	pInitCtx->x2 = (uint64_t)p2;
	pInitCtx->x3 = (uint64_t)p3;

	/* All the needed information is already in the ESF */
	z_arm64_userspace_enter(pInitCtx);

	CODE_UNREACHABLE;
}
#endif
