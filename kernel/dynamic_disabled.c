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
