/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/thread.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <string.h>
#include <zephyr/sys/check.h>
#include <ksched.h>
#include <kthread.h>
#include <kernel_internal.h>
#include <kuser.h>

#ifdef CONFIG_THREAD_CUSTOM_DATA
static inline void z_vrfy_k_thread_custom_data_set(void *data)
{
	z_impl_k_thread_custom_data_set(data);
}
#include <zephyr/syscalls/k_thread_custom_data_set_mrsh.c>

static inline void *z_vrfy_k_thread_custom_data_get(void)
{
	return z_impl_k_thread_custom_data_get();
}
#include <zephyr/syscalls/k_thread_custom_data_get_mrsh.c>
#endif /* CONFIG_THREAD_CUSTOM_DATA */

static inline int z_vrfy_k_is_preempt_thread(void)
{
	return z_impl_k_is_preempt_thread();
}
#include <zephyr/syscalls/k_is_preempt_thread_mrsh.c>

static inline int z_vrfy_k_thread_priority_get(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_priority_get(thread);
}
#include <zephyr/syscalls/k_thread_priority_get_mrsh.c>

static inline int z_vrfy_k_thread_name_set(k_tid_t thread, const char *str)
{
#ifdef CONFIG_THREAD_NAME
	char name[CONFIG_THREAD_MAX_NAME_LEN];

	if (thread != NULL) {
		if (K_SYSCALL_OBJ(thread, K_OBJ_THREAD) != 0) {
			return -EINVAL;
		}
	}

	/* In theory we could copy directly into thread->name, but
	 * the current z_vrfy / z_impl split does not provide a
	 * means of doing so.
	 */
	if (k_usermode_string_copy(name, str, sizeof(name)) != 0) {
		return -EFAULT;
	}

	return z_impl_k_thread_name_set(thread, name);
#else
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <zephyr/syscalls/k_thread_name_set_mrsh.c>

static inline int z_vrfy_k_thread_name_copy(k_tid_t thread,
					    char *buf, size_t size)
{
#ifdef CONFIG_THREAD_NAME
	size_t len;
	struct k_object *ko = k_object_find(thread);

	/* Special case: we allow reading the names of initialized threads
	 * even if we don't have permission on them
	 */
	if ((ko == NULL) || (ko->type != K_OBJ_THREAD) ||
		((ko->flags & K_OBJ_FLAG_INITIALIZED) == 0)) {
		return -EINVAL;
	}
	if (K_SYSCALL_MEMORY_WRITE(buf, size) != 0) {
		return -EFAULT;
	}
	len = strlen(thread->name);
	if ((len + 1) > size) {
		return -ENOSPC;
	}

	return k_usermode_to_copy((void *)buf, thread->name, len + 1);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <zephyr/syscalls/k_thread_name_copy_mrsh.c>

#ifdef CONFIG_THREAD_RUNTIME_STACK_SAFETY
int z_vrfy_k_thread_runtime_stack_unused_threshold_pct_set(struct k_thread *thread,
							   uint32_t pct)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	return z_impl_k_thread_runtime_stack_unused_threshold_pct_set(thread, pct);
}
#include <zephyr/syscalls/k_thread_runtime_stack_unused_threshold_pct_set_mrsh.c>

int z_vrfy_k_thread_runtime_stack_unused_threshold_set(struct k_thread *thread,
						       size_t threshold)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	return z_impl_k_thread_runtime_stack_unused_threshold_set(thread, threshold);
}
#include <zephyr/syscalls/k_thread_runtime_stack_unused_threshold_set_mrsh.c>

size_t z_vrfy_k_thread_runtime_stack_unused_threshold_get(struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	return z_impl_k_thread_runtime_stack_unused_threshold_get(thread);
}
#include <zephyr/syscalls/k_thread_runtime_stack_unused_threshold_get_mrsh.c>
#endif /* CONFIG_THREAD_RUNTIME_STACK_SAFETY */

k_tid_t z_vrfy_k_thread_create(struct k_thread *new_thread,
			       k_thread_stack_t *stack,
			       size_t stack_size, k_thread_entry_t entry,
			       void *p1, void *p2, void *p3,
			       int prio, uint32_t options, k_timeout_t delay)
{
	size_t total_size, stack_obj_size;
	struct k_object *stack_object;

	/* The thread and stack objects *must* be in an uninitialized state */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(new_thread, K_OBJ_THREAD));

	/* No need to check z_stack_is_user_capable(), it won't be in the
	 * object table if it isn't
	 */
	stack_object = k_object_find(stack);
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_object_validation_check(stack_object, stack,
						K_OBJ_THREAD_STACK_ELEMENT,
						_OBJ_INIT_FALSE) == 0,
				    "bad stack object"));

	/* Verify that the stack size passed in is OK by computing the total
	 * size and comparing it with the size value in the object metadata
	 */
	K_OOPS(K_SYSCALL_VERIFY_MSG(!size_add_overflow(K_THREAD_STACK_RESERVED,
						       stack_size, &total_size),
				    "stack size overflow (%zu+%zu)",
				    stack_size,
				    K_THREAD_STACK_RESERVED));

	/* Testing less-than-or-equal since additional room may have been
	 * allocated for alignment constraints
	 */
#ifdef CONFIG_GEN_PRIV_STACKS
	stack_obj_size = stack_object->data.stack_data->size;
#else
	stack_obj_size = stack_object->data.stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */
	K_OOPS(K_SYSCALL_VERIFY_MSG(total_size <= stack_obj_size,
				    "stack size %zu is too big, max is %zu",
				    total_size, stack_obj_size));

	/* User threads may only create other user threads and they can't
	 * be marked as essential
	 */
	K_OOPS(K_SYSCALL_VERIFY(options & K_USER));
	K_OOPS(K_SYSCALL_VERIFY(!(options & K_ESSENTIAL)));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	K_OOPS(K_SYSCALL_VERIFY(_is_valid_prio(prio, NULL)));
	K_OOPS(K_SYSCALL_VERIFY(z_is_prio_lower_or_equal(prio,
							_current->base.prio)));

	z_setup_new_thread(new_thread, stack, stack_size,
			   entry, p1, p2, p3, prio, options, NULL);

	thread_schedule_new(new_thread, delay);

	return new_thread;
}
#include <zephyr/syscalls/k_thread_create_mrsh.c>

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
int z_vrfy_k_thread_stack_space_get(const struct k_thread *thread,
				    size_t *unused_ptr)
{
	size_t unused;
	int ret;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	ret = z_impl_k_thread_stack_space_get(thread, &unused);
	CHECKIF(ret != 0) {
		return ret;
	}

	ret = k_usermode_to_copy(unused_ptr, &unused, sizeof(size_t));
	CHECKIF(ret != 0) {
		return ret;
	}

	return 0;
}
#include <zephyr/syscalls/k_thread_stack_space_get_mrsh.c>
#endif /* defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) */

static inline k_ticks_t z_vrfy_k_thread_timeout_remaining_ticks(
						    const struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_timeout_remaining_ticks(thread);
}
#include <zephyr/syscalls/k_thread_timeout_remaining_ticks_mrsh.c>

static inline k_ticks_t z_vrfy_k_thread_timeout_expires_ticks(
						  const struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_timeout_expires_ticks(thread);
}
#include <zephyr/syscalls/k_thread_timeout_expires_ticks_mrsh.c>

static inline void z_vrfy_k_thread_priority_set(k_tid_t thread, int prio)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	K_OOPS(K_SYSCALL_VERIFY_MSG(_is_valid_prio(prio, NULL),
				    "invalid thread priority %d", prio));
#ifndef CONFIG_USERSPACE_THREAD_MAY_RAISE_PRIORITY
	K_OOPS(K_SYSCALL_VERIFY_MSG((int8_t)prio >= thread->base.prio,
				    "thread priority may only be downgraded (%d < %d)",
				    prio, thread->base.prio));
#endif /* CONFIG_USERSPACE_THREAD_MAY_RAISE_PRIORITY */
	z_impl_k_thread_priority_set(thread, prio);
}
#include <zephyr/syscalls/k_thread_priority_set_mrsh.c>

static inline void z_vrfy_k_thread_suspend(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_suspend(thread);
}
#include <zephyr/syscalls/k_thread_suspend_mrsh.c>

static inline void z_vrfy_k_thread_resume(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_resume(thread);
}
#include <zephyr/syscalls/k_thread_resume_mrsh.c>

static inline void z_vrfy_k_wakeup(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_wakeup(thread);
}
#include <zephyr/syscalls/k_wakeup_mrsh.c>

/* Special case: don't oops if the thread is uninitialized.  This is because
 * the initialization bit does double-duty for thread objects; if false, means
 * the thread object is truly uninitialized, or the thread ran and exited for
 * some reason.
 *
 * Return true in this case indicating we should just do nothing and return
 * success to the caller.
 */
static bool thread_obj_validate(struct k_thread *thread)
{
	struct k_object *ko = k_object_find(thread);
	int ret = k_object_validate(ko, K_OBJ_THREAD, _OBJ_INIT_TRUE);

	switch (ret) {
	case 0:
		return false;
	case -EINVAL:
		return true;
	default:
#ifdef CONFIG_LOG
		k_object_dump_error(ret, thread, ko, K_OBJ_THREAD);
#endif /* CONFIG_LOG */
		/* ret is a non-zero error code here (the 0 and -EINVAL cases
		 * are handled above), so this branch must always oops. Passing
		 * ret as the "verify" expression would treat the failure code as
		 * success and fall through to CODE_UNREACHABLE; verify ret == 0
		 * so the oops is actually raised.
		 */
		K_OOPS(K_SYSCALL_VERIFY_MSG(ret == 0, "access denied"));
	}
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

static inline int z_vrfy_k_thread_join(struct k_thread *thread,
				       k_timeout_t timeout)
{
	if (thread_obj_validate(thread)) {
		return 0;
	}

	return z_impl_k_thread_join(thread, timeout);
}
#include <zephyr/syscalls/k_thread_join_mrsh.c>

static inline void z_vrfy_k_thread_abort(k_tid_t thread)
{
	if (thread_obj_validate(thread)) {
		return;
	}

	K_OOPS(K_SYSCALL_VERIFY_MSG(!z_is_thread_essential(thread),
				    "aborting essential thread %p", thread));

	z_impl_k_thread_abort((struct k_thread *)thread);
}
#include <zephyr/syscalls/k_thread_abort_mrsh.c>

static inline void z_vrfy_k_yield(void)
{
	z_impl_k_yield();
}
#include <zephyr/syscalls/k_yield_mrsh.c>
