/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_arch_data.h"
#include "zephyr/arch/tricore/thread.h"
#include "zephyr/sys/util_macro.h"
#include <zephyr/arch/tricore/cr.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <ksched.h>
#include <sys/cdefs.h>
#include <zephyr/tracing/tracing.h>

#ifdef CONFIG_TRICORE_MPU
extern void z_tricore_mpu_configure_thread(struct k_thread *thread);
#endif

union z_tricore_context __kstackmem __aligned(4 * 16) z_tricore_csa[CONFIG_TRICORE_CSA_COUNT];

int arch_coprocessors_disable(struct k_thread *thread)
{
	return -ENOTSUP;
}

void z_tricore_reclaim_csa(struct k_thread *thread)
{
	uint32_t pcxi = thread->callee_saved.pcxi & 0xFFFFF;
	struct z_tricore_lower_context *csa;
	unsigned int key;

	if (pcxi == 0) {
		return;
	}

	csa = UINT_TO_POINTER(((pcxi & 0xF0000) << 12) | ((pcxi & 0xFFFF) << 6));
	key = irq_lock();
	__asm volatile("dsync" ::: "memory");

	while (csa->pcxi != 0) {
		csa->pcxi &= 0xFFFFF;
		csa = UINT_TO_POINTER(((csa->pcxi & 0xF0000) << 12) | ((csa->pcxi & 0xFFFF) << 6));
	}

	csa->pcxi = cr_read(TRICORE_FCX);
	cr_write(TRICORE_FCX, pcxi);
	thread->callee_saved.pcxi = 0;

	irq_unlock(key);
}

unsigned int z_tricore_create_context(struct k_thread *thread, k_thread_entry_t entry, void *p1,
				      void *p2, void *p3, char *stack_ptr)
{
	uint32_t icr = cr_read(TRICORE_ICR);

	__asm volatile("disable" ::: "memory");
	uint32_t fcx = cr_read(TRICORE_FCX);
	z_tricore_lower_context_t *lower =
		(z_tricore_lower_context_t *)(((fcx & 0xF0000) << 12) | ((fcx & 0xFFFF) << 6));
	z_tricore_upper_context_t *upper =
		(z_tricore_upper_context_t *)(((lower->pcxi & 0xF0000) << 12) |
					      ((lower->pcxi & 0xFFFF) << 6));
	cr_write(TRICORE_FCX, upper->pcxi);
	if (icr & 0x8000) {
		__asm volatile("enable" ::: "memory");
	}

	lower->a4 = (uint32_t)entry;
	lower->a5 = (uint32_t)p1;
	lower->a6 = (uint32_t)p2;
	lower->a7 = (uint32_t)p3;
	lower->a11 = (uint32_t)z_thread_entry;
	lower->pcxi |= (1 << 21) | (1 << 20); /* Set PIE and UL bits */

	upper->a10 = (uint32_t)stack_ptr;
	upper->a11 = (uint32_t)z_thread_entry;
	upper->psw = (1 << 7); /* Set CDE bit*/
	upper->pcxi = 0;

#if defined(CONFIG_USERSPACE)
	if (thread->base.user_options & K_USER) {
		upper->psw |= (1 << 10); /* User-1 mode */
	} else {
#endif
		upper->psw |= (1 << 8) | (2 << 10); /* GW & Privileged mode */
#if defined(CONFIG_USERSPACE)
	}
#endif

#if defined(CONFIG_TRICORE_MPU)
	{
		uint32_t prs = thread->arch.prs;

		upper->psw |= ((prs & 1U) << 12) |
			      ((prs & 2U) << 12) |
			      ((prs & 4U) << 13);
	}
#endif

	return fcx;
}

#if defined(CONFIG_USERSPACE)
/* System calls can block, so they run on the thread's privileged stack */
static void z_tricore_priv_stack_init(struct k_thread *thread)
{
	uintptr_t start = (uintptr_t)z_priv_stack_find(thread->stack_obj);

	thread->arch.priv_stack_end = (uint32_t)Z_STACK_PTR_ALIGN(
		start + K_KERNEL_STACK_RESERVED + CONFIG_PRIVILEGED_STACK_SIZE);
}
#endif

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
		     k_thread_entry_t entry, void *p1, void *p2, void *p3)
{
	uint8_t prs = FIELD_GET(K_PROTECTION_SET_MASK, thread->base.user_options);

#if defined(CONFIG_TRICORE_MPU) && defined(CONFIG_USERSPACE)
	/* PRS 0 is reserved for ISR / syscall / kernel context */
	if ((prs == 0U) && ((thread->base.user_options & K_USER) != 0U)) {
		prs = 1U;
	}
#endif
	thread->arch.prs = prs;

#if CONFIG_CPU_TC18
	thread->callee_saved.pprs = prs;
#endif

#if defined(CONFIG_TRICORE_MPU) && defined(CONFIG_USERSPACE)
	if ((thread->base.user_options & K_USER) != 0U) {
		z_tricore_priv_stack_init(thread);
		stack_ptr -= Z_TRICORE_USER_STACK_SYSCALL_SLACK;
		stack_ptr = (char *)ROUND_DOWN((uintptr_t)stack_ptr, ARCH_STACK_PTR_ALIGN);
	}
#endif

	thread->callee_saved.pcxi = z_tricore_create_context(thread, entry, p1, p2, p3, stack_ptr);

	/* our switch handle is the thread pointer itself */
	thread->switch_handle = thread;
}

void z_impl_k_thread_abort(k_tid_t thread)
{
	bool self_abort = (thread == _current);
	unsigned int key = 0U;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, abort, thread);

	if (self_abort) {
		/* An interrupt switching away before z_thread_abort() marks the
		 * thread dead would reclaim the CSAs of a still runnable thread.
		 */
		key = irq_lock();
		_current_cpu->arch.to_reclaim = thread;
	}

	z_thread_abort(thread);

	if (self_abort) {
		irq_unlock(key);
	} else {
		z_tricore_reclaim_csa(thread);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, abort, thread);
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry, void *p1, void *p2, void *p3)
{
	struct k_thread *current = _current;
	uint32_t prs = current->arch.prs;
	char *sp = (char *)(current->stack_info.start + current->stack_info.size);
	uint32_t psw;

	z_tricore_priv_stack_init(current);

	(void)arch_irq_lock();

	/* A kernel thread runs on the kernel set, PRS 0 */
	if (prs == 0U) {
		prs = 1U;
		current->arch.prs = prs;
	}

#ifdef CONFIG_TRICORE_MPU
	sp -= Z_TRICORE_USER_STACK_SYSCALL_SLACK;
	sp = (char *)ROUND_DOWN((uintptr_t)sp, ARCH_STACK_PTR_ALIGN);
#endif

	__asm__ volatile("" : : "r"(prs), "r"(sp) : "memory");

#ifdef CONFIG_TRICORE_MPU
	z_tricore_mpu_configure_thread(current);
#endif

	psw = (1U << 7) | (1U << 10);
	psw |= ((prs & 1U) << 12) | ((prs & 2U) << 12) | ((prs & 4U) << 13);

#if CONFIG_CPU_TC18
	__asm__ volatile("mtcr %[r], %[v]\n\tisync"
			 : : [r] "i"(TRICORE_PPRS), [v] "d"(prs) : "memory");
#endif

	__asm__ volatile(
		"mov.a %%a4, %[entry]\n\t"
		"mov.a %%a5, %[arg1]\n\t"
		"mov.a %%a6, %[arg2]\n\t"
		"mov.a %%a7, %[arg3]\n\t"
		"mov.a %%a10, %[stack]\n\t"
		"enable\n\t"
		"mtcr " STRINGIFY(TRICORE_PSW) ", %[psw_val]\n\t"
		"isync\n\t"
		"j z_thread_entry\n\t"
		:
		: [entry] "d"((uint32_t)(uintptr_t)user_entry),
		  [arg1] "d"((uint32_t)(uintptr_t)p1),
		  [arg2] "d"((uint32_t)(uintptr_t)p2),
		  [arg3] "d"((uint32_t)(uintptr_t)p3),
		  [stack] "d"((uint32_t)(uintptr_t)sp),
		  [psw_val] "d"(psw)
		: "memory", "a4", "a5", "a6", "a7");

	CODE_UNREACHABLE;
}

#endif /* CONFIG_USERSPACE */

#ifndef CONFIG_MULTITHREADING

FUNC_NORETURN void z_tricore_switch_to_main_no_multithreading(k_thread_entry_t main_entry, void *p1,
							      void *p2, void *p3)
{
	main_entry(p1, p2, p3);

	/* main() returned with CONFIG_MULTITHREADING=n: there is no
	 * scheduler to fall back to, so loop forever to honor the
	 * FUNC_NORETURN contract.
	 */
	for (;;) {
	}
	CODE_UNREACHABLE;
}
#endif /* !CONFIG_MULTITHREADING */
