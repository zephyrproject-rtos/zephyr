/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>

k_thread_stack_t *z_impl_k_thread_stack_alloc(size_t size, int flags)
{
	ARG_UNUSED(size);
	ARG_UNUSED(flags);

	return NULL;
}

int z_impl_k_thread_stack_free(k_thread_stack_t *stack)
{
	ARG_UNUSED(stack);

	return -ENOSYS;
}

#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER
static inline bool is_stack_in_section(k_thread_stack_t *stack_base, size_t stack_size,
				       char *section_start, char *section_end)
{
	uintptr_t stack_start_addr = (uintptr_t)stack_base;
	uintptr_t stack_end_addr = (uintptr_t)stack_base + stack_size;
	uintptr_t section_start_addr = (uintptr_t)section_start;
	uintptr_t section_end_addr = (uintptr_t)section_end;

	return (stack_start_addr >= section_start_addr) && (stack_end_addr <= section_end_addr);
}

bool z_impl_k_thread_stack_is_valid(k_thread_stack_t *stack, size_t stack_size, uint32_t options)
{
	bool is_valid_stack = false;

	extern char z_kernel_stacks_start[];
	extern char z_kernel_stacks_end[];

	/**
	 * A kernel thread can have either stack allocated with
	 * K_KERNEL_STACK_DEFINE or K_THREAD_STACK_DEFINE.
	 *
	 * But a user space thread must have a stack allocated with
	 * K_THREAD_STACK_DEFINE.
	 */

#ifdef CONFIG_USERSPACE
	extern char z_user_stacks_start[];
	extern char z_user_stacks_end[];

	if (options & K_USER) {
		/* If userspace is enabled, user threads MUST be in the user stack section. */
		is_valid_stack = is_stack_in_section(stack, stack_size, z_user_stacks_start,
						     z_user_stacks_end);
	} else {
		is_valid_stack = is_stack_in_section(stack, stack_size, z_kernel_stacks_start,
						     z_kernel_stacks_end);
		if (!is_valid_stack) {
			/**
			 * Kernel threads can also be placed in the user stack section
			 * if CONFIG_USERSPACE is enabled, which happens if
			 * K_THREAD_STACK_DEFINE is used for them.
			 */
			is_valid_stack = is_stack_in_section(stack, stack_size, z_user_stacks_start,
							     z_user_stacks_end);
		}
	}
#else
	/**
	 * If userspace is disabled, K_USER option is effectively ignored;
	 * threads are created in kernel space. Check against kernel stacks.
	 */
	is_valid_stack =
		is_stack_in_section(stack, stack_size, z_kernel_stacks_start, z_kernel_stacks_end);
#endif /* CONFIG_USERSPACE */

	return is_valid_stack;
}
#else  /* !CONFIG_VALIDATE_THREAD_STACK_POINTER */
bool z_impl_k_thread_stack_is_valid(k_thread_stack_t *stack, size_t stack_size, uint32_t options)
{
	return true;
}
#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */
