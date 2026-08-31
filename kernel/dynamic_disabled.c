/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/internal/syscall_handler.h>

k_thread_stack_t *z_impl_k_thread_stack_alloc(size_t size, int flags)
{
	ARG_UNUSED(size);
	ARG_UNUSED(flags);

	return NULL;
}

#ifdef CONFIG_USERSPACE
static inline k_thread_stack_t *z_vrfy_k_thread_stack_alloc(size_t size, int flags)
{
	/* No check needed: neither argument is dereferenced, allocation always fails. */
	return z_impl_k_thread_stack_alloc(size, flags);
}
#include <zephyr/syscalls/k_thread_stack_alloc_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_thread_stack_free(k_thread_stack_t *stack)
{
	ARG_UNUSED(stack);

	return -ENOSYS;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_stack_free(k_thread_stack_t *stack)
{
	/* No check needed: stack is not dereferenced, -ENOSYS is always returned. */
	return z_impl_k_thread_stack_free(stack);
}
#include <zephyr/syscalls/k_thread_stack_free_mrsh.c>
#endif /* CONFIG_USERSPACE */
