/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/syscall_handler.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0
#define BA_SIZE CONFIG_DYNAMIC_THREAD_POOL_SIZE
#else
#define BA_SIZE 1
#endif

struct dyn_cb_data {
	k_tid_t tid;
	k_thread_stack_t *stack;
};

static K_THREAD_STACK_ARRAY_DEFINE(dynamic_stack, CONFIG_DYNAMIC_THREAD_POOL_SIZE,
				   CONFIG_DYNAMIC_THREAD_STACK_SIZE);
SYS_BITARRAY_DEFINE_STATIC(dynamic_ba, BA_SIZE);

static k_thread_stack_t *z_thread_stack_alloc_dyn(size_t align, size_t size)
{
	return z_thread_aligned_alloc(align, size);
}

static k_thread_stack_t *z_thread_stack_alloc_pool(size_t size)
{
	int rv;
	size_t offset;
	k_thread_stack_t *stack;

	if (size > CONFIG_DYNAMIC_THREAD_STACK_SIZE) {
		LOG_DBG("stack size %zu is > pool stack size %d", size,
			CONFIG_DYNAMIC_THREAD_STACK_SIZE);
		return NULL;
	}

	rv = sys_bitarray_alloc(&dynamic_ba, 1, &offset);
	if (rv < 0) {
		LOG_DBG("unable to allocate stack from pool");
		return NULL;
	}

	__ASSERT_NO_MSG(offset < CONFIG_DYNAMIC_THREAD_POOL_SIZE);

	stack = (k_thread_stack_t *)&dynamic_stack[offset];

	return stack;
}

static k_thread_stack_t *stack_alloc_dyn(size_t size, int flags)
{
	if ((flags & K_USER) == K_USER) {
#ifdef CONFIG_DYNAMIC_OBJECTS
		return k_object_alloc_size(K_OBJ_THREAD_STACK_ELEMENT, size);
#else
		/* Dynamic user stack needs a kobject, so if this option is not
		 * enabled we can't proceed.
		 */
		return NULL;
#endif
	}

	return z_thread_stack_alloc_dyn(Z_KERNEL_STACK_OBJ_ALIGN,
			Z_KERNEL_STACK_SIZE_ADJUST(size));
}

k_thread_stack_t *z_impl_k_thread_stack_alloc(size_t size, int flags)
{
	k_thread_stack_t *stack = NULL;

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		stack = stack_alloc_dyn(size, flags);
		if (stack == NULL && CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size);
		}
	} else if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_POOL)) {
		if (CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size);
		}

		if ((stack == NULL) && IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
			stack = stack_alloc_dyn(size, flags);
		}
	}

	return stack;
}

#ifdef CONFIG_USERSPACE
static inline k_thread_stack_t *z_vrfy_k_thread_stack_alloc(size_t size, int flags)
{
	return z_impl_k_thread_stack_alloc(size, flags);
}
#include <syscalls/k_thread_stack_alloc_mrsh.c>
#endif

static void dyn_cb(const struct k_thread *thread, void *user_data)
{
	struct dyn_cb_data *const data = (struct dyn_cb_data *)user_data;

	if (data->stack == (k_thread_stack_t *)thread->stack_info.start) {
		__ASSERT(data->tid == NULL, "stack %p is associated with more than one thread!",
			 (void *)thread->stack_info.start);
		data->tid = (k_tid_t)thread;
	}
}

int z_impl_k_thread_stack_free(k_thread_stack_t *stack)
{
	char state_buf[16] = {0};
	struct dyn_cb_data data = {.stack = stack};

	/* Get a possible tid associated with stack */
	k_thread_foreach(dyn_cb, &data);

	if (data.tid != NULL) {
		/* Check if thread is in use */
		if (k_thread_state_str(data.tid, state_buf, sizeof(state_buf)) != state_buf) {
			LOG_ERR("tid %p is invalid!", data.tid);
			return -EINVAL;
		}

		if (!(strcmp("dummy", state_buf) == 0) || (strcmp("dead", state_buf) == 0)) {
			LOG_ERR("tid %p is in use!", data.tid);
			return -EBUSY;
		}
	}

	if (CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
		if (IS_ARRAY_ELEMENT(dynamic_stack, stack)) {
			if (sys_bitarray_free(&dynamic_ba, 1, ARRAY_INDEX(dynamic_stack, stack))) {
				LOG_ERR("stack %p is not allocated!", stack);
				return -EINVAL;
			}

			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
#ifdef CONFIG_USERSPACE
		if (z_object_find(stack)) {
			k_object_free(stack);
		} else {
			k_free(stack);
		}
#else
		k_free(stack);
#endif
	} else {
		LOG_ERR("Invalid stack %p", stack);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_stack_free(k_thread_stack_t *stack)
{
	return z_impl_k_thread_stack_free(stack);
}
#include <syscalls/k_thread_stack_free_mrsh.c>
#endif
