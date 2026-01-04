/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_internal.h"

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/internal/syscall_handler.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0
#define BA_SIZE CONFIG_DYNAMIC_THREAD_POOL_SIZE
#else
#define BA_SIZE 1
#endif /* CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0 */

struct dyn_cb_data {
	k_tid_t tid;
	k_thread_stack_t *stack;
};

static K_THREAD_STACK_ARRAY_DEFINE(dynamic_stack, CONFIG_DYNAMIC_THREAD_POOL_SIZE,
				   K_THREAD_STACK_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE));
SYS_BITARRAY_DEFINE_STATIC(dynamic_ba, BA_SIZE);

#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER
#ifdef CONFIG_DYNAMIC_THREAD_ALLOC

static sys_dlist_t dyn_stacks_dlist;
static struct k_spinlock dyn_stacks_dlist_lock;

struct dyn_stack_info {
	sys_dnode_t node;
	k_thread_stack_t *stack;
	size_t size;
	uint32_t options;
};

#endif /* CONFIG_DYNAMIC_THREAD_ALLOC */
#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */

static k_thread_stack_t *z_thread_stack_alloc_pool(size_t size, int flags)
{
	int rv;
	size_t offset;
	k_thread_stack_t *stack;

	if (size > CONFIG_DYNAMIC_THREAD_STACK_SIZE) {
		LOG_DBG("stack size %zu is > pool stack size %zu", size,
			(size_t)CONFIG_DYNAMIC_THREAD_STACK_SIZE);
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

static k_thread_stack_t *z_thread_stack_alloc_dyn(size_t size, int flags)
{
	if ((flags & K_USER) == K_USER) {
#ifdef CONFIG_DYNAMIC_OBJECTS
		k_thread_stack_t *stack_ptr = k_object_alloc_size(K_OBJ_THREAD_STACK_ELEMENT, size);
#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER
		k_spinlock_key_t key;

		key = k_spin_lock(&dyn_stacks_dlist_lock);
		if (stack_ptr != NULL) {
			struct dyn_stack_info *info = z_thread_aligned_alloc(
				__alignof__(struct dyn_stack_info), sizeof(struct dyn_stack_info));

			if (info == NULL) {
				k_object_free(stack_ptr);
				k_spin_unlock(&dyn_stacks_dlist_lock, key);
				return NULL;
			}
			info->stack = stack_ptr;
			info->size = size;
			info->options = flags;
			sys_dlist_append(&dyn_stacks_dlist, &info->node);
		}
		k_spin_unlock(&dyn_stacks_dlist_lock, key);

#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */
		return stack_ptr;
#else
		/* Dynamic user stack needs a kobject, so if this option is not
		 * enabled we can't proceed.
		 */
		return NULL;
#endif /* CONFIG_DYNAMIC_OBJECTS */
	}

	k_thread_stack_t *stack_ptr =
		z_thread_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN, K_KERNEL_STACK_LEN(size));
#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER
	k_spinlock_key_t key;

	key = k_spin_lock(&dyn_stacks_dlist_lock);
	if (stack_ptr != NULL) {
		struct dyn_stack_info *info = z_thread_aligned_alloc(
			__alignof__(struct dyn_stack_info), sizeof(struct dyn_stack_info));

		if (info == NULL) {
			k_free(stack_ptr);
			k_spin_unlock(&dyn_stacks_dlist_lock, key);
			return NULL;
		}
		info->stack = stack_ptr;
		info->size = size;
		info->options = flags;
		sys_dlist_append(&dyn_stacks_dlist, &info->node);
	}

	k_spin_unlock(&dyn_stacks_dlist_lock, key);
#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */

	return stack_ptr;
}

k_thread_stack_t *z_impl_k_thread_stack_alloc(size_t size, int flags)
{
	k_thread_stack_t *stack = NULL;

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		stack = z_thread_stack_alloc_dyn(size, flags);
		if (stack == NULL && CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size, flags);
		}
	} else if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_POOL)) {
		if (CONFIG_DYNAMIC_THREAD_POOL_SIZE > 0) {
			stack = z_thread_stack_alloc_pool(size, flags);
		}

		if ((stack == NULL) && IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
			stack = z_thread_stack_alloc_dyn(size, flags);
		}
	}

	return stack;
}

#ifdef CONFIG_USERSPACE
static inline k_thread_stack_t *z_vrfy_k_thread_stack_alloc(size_t size, int flags)
{
	return z_impl_k_thread_stack_alloc(size, flags);
}
#include <zephyr/syscalls/k_thread_stack_alloc_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
	struct dyn_cb_data data = {.stack = stack};

	/* Get a possible tid associated with stack */
	k_thread_foreach(dyn_cb, &data);

	if (data.tid != NULL) {
		if (!(z_is_thread_state_set(data.tid, _THREAD_DUMMY) ||
		      z_is_thread_state_set(data.tid, _THREAD_DEAD))) {
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
#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER
		k_spinlock_key_t key;
		sys_dnode_t *pnode, *pnode_safe;
		struct dyn_stack_info *pinfo;

		key = k_spin_lock(&dyn_stacks_dlist_lock);

		SYS_DLIST_FOR_EACH_NODE_SAFE(&dyn_stacks_dlist, pnode, pnode_safe) {
			pinfo = CONTAINER_OF(pnode, struct dyn_stack_info, node);
			if (pinfo->stack == stack) {
#ifdef CONFIG_USERSPACE
				if (k_object_find(stack)) {
					k_object_free(stack);
				} else {
					k_free(stack);
				}
#else
				k_free(stack);
#endif /* CONFIG_USERSPACE */
				sys_dlist_remove(&pinfo->node);
				k_free(pinfo);
				k_spin_unlock(&dyn_stacks_dlist_lock, key);
				return 0;
			}
		}
		k_spin_unlock(&dyn_stacks_dlist_lock, key);
		return -EINVAL;
#else /* !CONFIG_VALIDATE_THREAD_STACK_POINTER */
#ifdef CONFIG_USERSPACE
		if (k_object_find(stack)) {
			k_object_free(stack);
		} else {
			k_free(stack);
		}
#else
		k_free(stack);
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */
	} else {
		LOG_DBG("Invalid stack %p", stack);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
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
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_VALIDATE_THREAD_STACK_POINTER

#ifdef CONFIG_DYNAMIC_THREAD_ALLOC
static inline bool is_dynamic_stack_valid(k_thread_stack_t *stack, size_t stack_size,
					  uint32_t options)
{
	bool is_valid = false;
	k_spinlock_key_t key;
	sys_dnode_t *pnode;
	struct dyn_stack_info *pinfo;

	key = k_spin_lock(&dyn_stacks_dlist_lock);

	SYS_DLIST_FOR_EACH_NODE(&dyn_stacks_dlist, pnode) {
		pinfo = CONTAINER_OF(pnode, struct dyn_stack_info, node);
		if (pinfo->stack != stack) {
			continue;
		}

		if (pinfo->size >= stack_size) {
			const bool user_required = (options & K_USER) != 0U;
			const bool user_allowed = (pinfo->options & K_USER) != 0U;

			/* If user is required, it must be enabled */
			is_valid = (!user_required) || user_allowed;
		}

		break;
	}

	k_spin_unlock(&dyn_stacks_dlist_lock, key);
	return is_valid;
}
#endif /* CONFIG_DYNAMIC_THREAD_ALLOC */

static inline bool is_stack_in_section(k_thread_stack_t *stack_base, size_t stack_size,
				       char *section_start, char *section_end)
{
	uintptr_t stack_start_addr = (uintptr_t)stack_base;
	uintptr_t stack_end_addr = (uintptr_t)stack_base + stack_size;
	uintptr_t section_start_addr = (uintptr_t)section_start;
	uintptr_t section_end_addr = (uintptr_t)section_end;

	return (stack_start_addr >= section_start_addr) && (stack_end_addr <= section_end_addr);
}

static inline bool is_static_stack_valid(k_thread_stack_t *stack, size_t stack_size,
					 uint32_t options)
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

bool z_impl_k_thread_stack_is_valid(k_thread_stack_t *stack, size_t stack_size, uint32_t options)
{
	/* If the stack belongs to dynamic stack pool, it must not be in freed state */
	if (IS_ARRAY_ELEMENT(dynamic_stack, stack)) {
		int val;
		int ret =
			sys_bitarray_test_bit(&dynamic_ba, ARRAY_INDEX(dynamic_stack, stack), &val);
		if (ret != 0) {
			return false;
		}

		if (val == 0) {
			return false;
		}

		if (stack_size > CONFIG_DYNAMIC_THREAD_STACK_SIZE) {
			return false;
		}

		return true;
	}

	bool is_valid_stack = is_static_stack_valid(stack, stack_size, options);

#ifdef CONFIG_DYNAMIC_THREAD_ALLOC
	is_valid_stack = is_valid_stack || is_dynamic_stack_valid(stack, stack_size, options);
#endif /* CONFIG_DYNAMIC_THREAD_ALLOC */

	return is_valid_stack;
}

int z_thread_validate_stack_init(void)
{
	sys_dlist_init(&dyn_stacks_dlist);
	return 0;
}

SYS_INIT(z_thread_validate_stack_init, EARLY, 0);

#else  /* !CONFIG_VALIDATE_THREAD_STACK_POINTER */
bool z_impl_k_thread_stack_is_valid(k_thread_stack_t *stack, size_t stack_size, uint32_t options)
{
	return true;
}
#endif /* CONFIG_VALIDATE_THREAD_STACK_POINTER */

#ifdef CONFIG_USERSPACE
static inline bool z_vrfy_k_thread_stack_is_valid(k_thread_stack_t *stack, size_t stack_size,
						  uint32_t options)
{
	return z_impl_k_thread_stack_is_valid(stack, stack_size, options);
}
#include <zephyr/syscalls/k_thread_stack_is_valid_mrsh.c>
#endif /* CONFIG_USERSPACE */
