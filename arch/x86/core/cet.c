/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Indirect Branch Tracking
 *
 * Indirect Branch Tracking (IBT) setup routines.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/x86/msr.h>
#include <zephyr/arch/x86/cet.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

#include <kernel_arch_data.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_X86_64
#define TOKEN_OFFSET 5
#else
#define TOKEN_OFFSET 4
#endif

#ifdef CONFIG_HW_SHADOW_STACK
#ifdef CONFIG_HW_SHADOW_STACK_ALLOW_REUSE
extern void arch_shadow_stack_reset(k_tid_t thread);
#endif /* CONFIG_HW_SHADOW_STACK_ALLOW_REUSE */

int arch_thread_hw_shadow_stack_attach(k_tid_t thread,
				       arch_thread_hw_shadow_stack_t *stack,
				       size_t stack_size)
{
	/* Can't attach to NULL */
	if (stack == NULL) {
		LOG_ERR("Can't set NULL shadow stack for thread %p\n", thread);
		return -EINVAL;
	}

	/* Or if the thread already has a shadow stack. */
	if (thread->arch.shstk_addr != NULL) {
#ifdef CONFIG_HW_SHADOW_STACK_ALLOW_REUSE
		/* Allow reuse of the shadow stack if the base and size are the same */
		if (thread->arch.shstk_base == stack &&
		    thread->arch.shstk_size == stack_size) {
			unsigned int key;

			key = arch_irq_lock();
			arch_shadow_stack_reset(thread);
			arch_irq_unlock(key);

			return 0;
		}
#endif
		LOG_ERR("Shadow stack already set up for thread %p\n", thread);
		return -EINVAL;
	}

	thread->arch.shstk_addr = stack + (stack_size -
					   TOKEN_OFFSET * sizeof(*stack)) / sizeof(*stack);
	thread->arch.shstk_size = stack_size;
	thread->arch.shstk_base = stack;

	return 0;
}
#endif

void z_x86_cet_enable(void)
{
#ifdef CONFIG_X86_64
	__asm volatile (
			"movq %cr4, %rax\n\t"
			"orq $0x800000, %rax\n\t"
			"movq %rax, %cr4\n\t"
			);
#else
	__asm volatile (
			"movl %cr4, %eax\n\t"
			"orl $0x800000, %eax\n\t"
			"movl %eax, %cr4\n\t"
			);
#endif
}

#ifdef CONFIG_X86_CET_IBT
void z_x86_ibt_enable(void)
{
	uint64_t msr = z_x86_msr_read(X86_S_CET_MSR);

	msr |= X86_S_CET_MSR_ENDBR | X86_S_CET_MSR_NO_TRACK;
	z_x86_msr_write(X86_S_CET_MSR, msr);
}
#endif

#ifdef CONFIG_X86_CET_VERIFY_KERNEL_SHADOW_STACK
void z_x86_cet_shadow_stack_panic(k_tid_t *thread)
{
	LOG_ERR("Shadow stack enabled, but outgoing thread [%p] struct "
		"missing shadow stack pointer", thread);

	k_panic();
}
#endif
