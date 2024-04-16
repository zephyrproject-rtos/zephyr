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

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/arch/cpu.h>

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
 *   is marked as not user accessible in the MMU/MPU.
 *
 * - a stack guard region will be added bellow the kernel stack when
 *   ARM64_STACK_PROTECTION is enabled. In this case, SP_EL0 will always point
 *   to the safe exception stack in the kernel space. For the kernel thread,
 *   SP_EL0 will not change always pointing to safe exception stack. For the
 *   userspace thread, SP_EL0 will switch from the user stack to the safe
 *   exception stack when entering the EL1 mode, and restore to the user stack
 *   when backing to userspace (EL0).
 *
 *   Kernel threads:
 *
 * High memory addresses
 *
 *    +---------------+ <- stack_ptr
 *  E |     ESF       |
 *  L |<<<<<<<<<<<<<<<| <- SP_EL1
 *  1 |               |
 *    +---------------+ <- stack limit
 *    |  Stack guard  | } Z_ARM64_STACK_GUARD_SIZE (protected by MMU/MPU)
 *    +---------------+ <- stack_obj
 *
 * Low Memory addresses
 *
 *
 *   User threads:
 *
 * High memory addresses
 *
 *    +---------------+ <- stack_ptr
 *  E |               |
 *  L |<<<<<<<<<<<<<<<| <- SP_EL0
 *  0 |               |
 *    +---------------+ ..............|
 *  E |     ESF       |               |  Privileged portion of the stack
 *  L +>>>>>>>>>>>>>>>+ <- SP_EL1     |_ used during exceptions and syscalls
 *  1 |               |               |  of size ARCH_THREAD_STACK_RESERVED
 *    +---------------+ <- stack limit|
 *    |  Stack guard  | } Z_ARM64_STACK_GUARD_SIZE (protected by MMU/MPU)
 *    +---------------+ <- stack_obj
 *
 * Low Memory addresses
 *
 *  When a kernel thread switches to user mode the SP_EL0 and SP_EL1
 *  values are reset accordingly in arch_user_mode_enter().
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
	extern void z_arm64_exit_exc(void);
	z_arch_esf_t *pInitCtx;

	/*
	 * Clean the thread->arch to avoid unexpected behavior because the
	 * thread->arch might be dirty
	 */
	memset(&thread->arch, 0, sizeof(thread->arch));

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
	 * of SP_EL1 will be set in arch_user_mode_enter().
	 */
	thread->callee_saved.sp_elx = (uint64_t)pInitCtx;
	thread->callee_saved.lr = (uint64_t)z_arm64_exit_exc;

	thread->switch_handle = thread;
#if defined(CONFIG_ARM64_STACK_PROTECTION)
	thread->arch.stack_limit = (uint64_t)stack + Z_ARM64_STACK_GUARD_SIZE;
	z_arm64_thread_mem_domains_init(thread);
#endif
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	uintptr_t stack_el0, stack_el1;
	uint64_t tmpreg;

	/* Map the thread stack */
	z_arm64_thread_mem_domains_init(_current);

	/* Top of the user stack area */
	stack_el0 = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				      _current->stack_info.size -
				      _current->stack_info.delta);

	/* Top of the privileged non-user-accessible part of the stack */
	stack_el1 = (uintptr_t)(_current->stack_obj + ARCH_THREAD_STACK_RESERVED);

	register void *x0 __asm__("x0") = user_entry;
	register void *x1 __asm__("x1") = p1;
	register void *x2 __asm__("x2") = p2;
	register void *x3 __asm__("x3") = p3;

	/* we don't want to be disturbed when playing with SPSR and ELR */
	arch_irq_lock();

	/* set up and drop into EL0 */
	__asm__ volatile (
	"mrs	%[tmp], tpidrro_el0\n\t"
	"orr	%[tmp], %[tmp], %[is_usermode_flag]\n\t"
	"msr	tpidrro_el0, %[tmp]\n\t"
	"msr	elr_el1, %[elr]\n\t"
	"msr	spsr_el1, %[spsr]\n\t"
	"msr	sp_el0, %[sp_el0]\n\t"
	"mov	sp, %[sp_el1]\n\t"
	"eret"
	: [tmp] "=&r" (tmpreg)
	: "r" (x0), "r" (x1), "r" (x2), "r" (x3),
	  [is_usermode_flag] "i" (TPIDRROEL0_IN_EL0),
	  [elr] "r" (z_thread_entry),
	  [spsr] "r" (DAIF_FIQ_BIT | SPSR_MODE_EL0T),
	  [sp_el0] "r" (stack_el0),
	  [sp_el1] "r" (stack_el1)
	: "memory");

	CODE_UNREACHABLE;
}
#endif
