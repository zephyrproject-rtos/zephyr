/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) thread runtime interface
 * implementation
 */
#include <zephyr/kernel.h>
#include <xenlib_osal_platform.h>

xenlib_osal_tid_t xenlib_osal_thread_create(xenlib_osal_thread_t *thread,
					xenlib_osal_thread_stack_t *stack,
					size_t stack_size,
					xenlib_osal_thread_entry_t entry,
					void *arg1,
					void *arg2,
					void *arg3,
					int priority,
					int options,
					xenlib_osal_timeout_t delay)
{
	return k_thread_create(thread, stack, stack_size, entry, arg1, arg2,
					arg3, priority, options, delay);
}

int xenlib_osal_thread_name_set(xenlib_osal_tid_t thread,
					const char *name)
{
	return k_thread_name_set(thread, name);
}

int xenlib_osal_thread_join(xenlib_osal_thread_t *thread,
					xenlib_osal_timeout_t timeout)
{
	return k_thread_join(thread, timeout);
}
