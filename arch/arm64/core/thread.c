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

/*
 * Note about stack usage:
 *
 * [ see also comments in include/arch/arm64/thread_stack.h ]
 *
 * - kernel threads are running in EL1 using SP_EL1 as stack pointer during
 *   normal execution and during exceptions. They are by definition already
 *   running in a privileged stack that is their own.
 *
 * - user threads are running in EL0 using SP_EL0 as stack pointer during
 *   normal execution. When at exception is taken or a syscall is called the
 *   stack pointer switches to SP_EL1 and the execution starts using the
 *   privileged portion of the user stack without touching SP_EL0. This portion
 *   is marked as not user accessible in the MMU.
 *
 *   Kernel threads:
 *
 *    +---------------+ <- stack_ptr
 *  E |     ESF       |
 *  L |<<<<<<<<<<<<<<<| <- SP_EL1
 *  1 |               |
 *    +---------------+

 *
 *   User threads:
 *
 *    +---------------+ <- stack_ptr
 *  E |               |
 *  L |<<<<<<<<<<<<<<<| <- SP_EL0
 *  0 |               |
 *    +---------------+ ..............|
 *  E |     ESF       |               |  Privileged portion of the stack
 *  L +>>>>>>>>>>>>>>>+ <- SP_EL1     |_ used during exceptions and syscalls
 *  1 |               |               |  of size ARCH_THREAD_STACK_RESERVED
 *    +---------------+ <- stack_obj..|
 *
 *  When a new user thread is created or when a kernel thread switches to user
 *  mode the initial ESF is relocated to the privileged portion of the stack
 *  and the values of stack_ptr, SP_EL0 and SP_EL1 are correctly reset when
 *  going through arch_user_mode_enter() and z_arm64_userspace_enter()
 *
 */

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

	/*
	 * The ESF is now hosted at the top of the stack. For user threads this
	 * is also fine because at this stage they are still running in EL1.
	 * The context will be relocated by arch_user_mode_enter() before
	 * dropping into EL0.
	 */

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
	/* Keep using SP_EL1 */
	pInitCtx->spsr = SPSR_MODE_EL1H | DAIF_FIQ_BIT;

	/* thread birth happens through the exception return path */
	thread->arch.exception_depth = 1;

	/*
	 * We are saving SP_EL1 to pop out entry and parameters when going
	 * through z_arm64_exit_exc(). For user threads the definitive location
	 * of SP_EL1 will be set implicitly when going through
	 * z_arm64_userspace_enter() (see comments there)
	 */
	thread->callee_saved.sp_elx = (uint64_t)pInitCtx;

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

	/* Map the thread stack */
	z_arm64_thread_mem_domains_init(_current);

	/*
	 * Reset the SP_EL0 stack pointer to the stack top discarding any old
	 * context. The actual register is written in z_arm64_userspace_enter()
	 */
	stack_ptr = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				      _current->stack_info.size -
				      _current->stack_info.delta);

	/*
	 * Reconstruct the ESF from scratch to leverage the z_arm64_exit_exc()
	 * macro that will simulate a return from exception to move from EL1h
	 * to EL0t. On return we will be in userspace using SP_EL0.
	 *
	 * We relocate the ESF to the beginning of the privileged stack in the
	 * not user accessible part of the stack
	 */
	pInitCtx = (struct __esf *) (_current->stack_obj + ARCH_THREAD_STACK_RESERVED -
			sizeof(struct __esf));

	pInitCtx->spsr = DAIF_FIQ_BIT | SPSR_MODE_EL0T;
	pInitCtx->elr = (uint64_t)z_thread_entry;

	pInitCtx->x0 = (uint64_t)user_entry;
	pInitCtx->x1 = (uint64_t)p1;
	pInitCtx->x2 = (uint64_t)p2;
	pInitCtx->x3 = (uint64_t)p3;

	/* All the needed information is already in the ESF */
	z_arm64_userspace_enter(pInitCtx, stack_ptr);

	CODE_UNREACHABLE;
}
#endif
