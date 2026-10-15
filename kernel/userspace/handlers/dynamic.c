/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/dynamic.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel/thread_stack.h>

static inline k_thread_stack_t *z_vrfy_k_thread_stack_alloc(size_t size, int flags)
{
	return z_impl_k_thread_stack_alloc(size, flags);
}
#include <zephyr/syscalls/k_thread_stack_alloc_mrsh.c>

static inline int z_vrfy_k_thread_stack_free(k_thread_stack_t *stack)
{
	/* The thread stack object must not be in initialized state.
	 *
	 * Thread stack objects are initialized when the thread is created
	 * and de-initialized when the thread is destroyed. Since we can't
	 * free a stack that is in use, we have to check that the caller
	 * has access to the object but that it is not in use anymore.
	 */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(stack, K_OBJ_THREAD_STACK_ELEMENT));

	return z_impl_k_thread_stack_free(stack);
}
#include <zephyr/syscalls/k_thread_stack_free_mrsh.c>
