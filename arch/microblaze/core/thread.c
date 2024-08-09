/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/kernel.h>

void z_thread_entry(k_thread_entry_t thread, void *arg1, void *arg2, void *arg3);

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack, char *stack_ptr,
				 k_thread_entry_t entry, void *arg1, void *arg2, void *arg3)
{
	struct __esf *stack_init;

	/* Initial stack frame for thread */
	stack_init =
		(struct __esf *)Z_STACK_PTR_ALIGN(Z_STACK_PTR_TO_FRAME(struct __esf, stack_ptr));

	/* Setup the initial stack frame */
	stack_init->r5 = (uint32_t)entry;
	stack_init->r6 = (uint32_t)arg1;
	stack_init->r7 = (uint32_t)arg2;
	stack_init->r8 = (uint32_t)arg3;
	stack_init->r14 = (uint32_t)z_thread_entry;

	/* Initialise the stack to stack_init */
	thread->callee_saved.r1 = (uint32_t)stack_init;
	/* Threads start with irq_unlocked */
	thread->callee_saved.key = 1;
	/* and return value set to default */
	thread->callee_saved.retval = -EAGAIN;
}

/**
 * @brief can be used as a safe stub for thread_abort implementation
 *
 * @return FUNC_NORETURN
 */
FUNC_NORETURN void arch_thread_sleep_forever(void)
{
	k_sleep(K_FOREVER);
	CODE_UNREACHABLE;
}
