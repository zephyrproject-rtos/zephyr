/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel thread support
 *
 * This module provides general purpose thread support.
 */

#include <kernel.h>

#include <toolchain.h>
#include <linker/sections.h>

#include <spinlock.h>
#include <kernel_structs.h>
#include <sys/math_extras.h>
#include <sys_clock.h>
#include <drivers/timer/system_timer.h>
#include <ksched.h>
#include <wait_q.h>
#include <sys/atomic.h>
#include <syscall_handler.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <init.h>
#include <debug/tracing.h>
#include <stdbool.h>

static struct k_spinlock lock;

#define _FOREACH_STATIC_THREAD(thread_data)              \
	Z_STRUCT_SECTION_FOREACH(_static_thread_data, thread_data)

void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data)
{
#if defined(CONFIG_THREAD_MONITOR)
	struct k_thread *thread;
	k_spinlock_key_t key;

	__ASSERT(user_cb != NULL, "user_cb can not be NULL");

	/*
	 * Lock is needed to make sure that the _kernel.threads is not being
	 * modified by the user_cb either directly or indirectly.
	 * The indirect ways are through calling k_thread_create and
	 * k_thread_abort from user_cb.
	 */
	key = k_spin_lock(&lock);
	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		user_cb(thread, user_data);
	}
	k_spin_unlock(&lock, key);
#endif
}

bool k_is_in_isr(void)
{
	return z_arch_is_in_isr();
}

/*
 * This function tags the current thread as essential to system operation.
 * Exceptions raised by this thread will be treated as a fatal system error.
 */
void z_thread_essential_set(void)
{
	_current->base.user_options |= K_ESSENTIAL;
}

/*
 * This function tags the current thread as not essential to system operation.
 * Exceptions raised by this thread may be recoverable.
 * (This is the default tag for a thread.)
 */
void z_thread_essential_clear(void)
{
	_current->base.user_options &= ~K_ESSENTIAL;
}

/*
 * This routine indicates if the current thread is an essential system thread.
 *
 * Returns true if current thread is essential, false if it is not.
 */
bool z_is_thread_essential(void)
{
	return (_current->base.user_options & K_ESSENTIAL) == K_ESSENTIAL;
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
void z_impl_k_busy_wait(u32_t usec_to_wait)
{
#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
	/* use 64-bit math to prevent overflow when multiplying */
	u32_t cycles_to_wait = (u32_t)(
		(u64_t)usec_to_wait *
		(u64_t)sys_clock_hw_cycles_per_sec() /
		(u64_t)USEC_PER_SEC
	);
	u32_t start_cycles = k_cycle_get_32();

	for (;;) {
		u32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#else
	z_arch_busy_wait(usec_to_wait);
#endif /* CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT */
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_busy_wait(u32_t usec_to_wait)
{
	z_impl_k_busy_wait(usec_to_wait);
}
#include <syscalls/k_busy_wait_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_SYS_CLOCK_EXISTS */

#ifdef CONFIG_THREAD_CUSTOM_DATA
void z_impl_k_thread_custom_data_set(void *value)
{
	_current->custom_data = value;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_custom_data_set(void *data)
{
	z_impl_k_thread_custom_data_set(data);
}
#include <syscalls/k_thread_custom_data_set_mrsh.c>
#endif

void *z_impl_k_thread_custom_data_get(void)
{
	return _current->custom_data;
}

#ifdef CONFIG_USERSPACE
static inline void *z_vrfy_k_thread_custom_data_get(void)
{
	return z_impl_k_thread_custom_data_get();
}
#include <syscalls/k_thread_custom_data_get_mrsh.c>

#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_THREAD_CUSTOM_DATA */

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Remove a thread from the kernel's list of active threads.
 */
void z_thread_monitor_exit(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (thread == _kernel.threads) {
		_kernel.threads = _kernel.threads->next_thread;
	} else {
		struct k_thread *prev_thread;

		prev_thread = _kernel.threads;
		while ((prev_thread != NULL) &&
			(thread != prev_thread->next_thread)) {
			prev_thread = prev_thread->next_thread;
		}
		if (prev_thread != NULL) {
			prev_thread->next_thread = thread->next_thread;
		}
	}

	k_spin_unlock(&lock, key);
}
#endif

int z_impl_k_thread_name_set(struct k_thread *thread, const char *value)
{
#ifdef CONFIG_THREAD_NAME
	if (thread == NULL) {
		thread = _current;
	}

	strncpy(thread->name, value, CONFIG_THREAD_MAX_NAME_LEN);
	thread->name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';
	sys_trace_thread_name_set(thread);
	return 0;
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(value);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_name_set(struct k_thread *t, const char *str)
{
#ifdef CONFIG_THREAD_NAME
	size_t len;
	int err;

	if (t != NULL) {
		if (Z_SYSCALL_OBJ(t, K_OBJ_THREAD) != 0) {
			return -EINVAL;
		}
	}

	len = z_user_string_nlen(str, CONFIG_THREAD_MAX_NAME_LEN, &err);
	if (err != 0) {
		return -EFAULT;
	}
	if (Z_SYSCALL_MEMORY_READ(str, len) != 0) {
		return -EFAULT;
	}

	return z_impl_k_thread_name_set(t, str);
#else
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <syscalls/k_thread_name_set_mrsh.c>
#endif /* CONFIG_USERSPACE */

const char *k_thread_name_get(struct k_thread *thread)
{
#ifdef CONFIG_THREAD_NAME
	return (const char *)thread->name;
#else
	ARG_UNUSED(thread);
	return NULL;
#endif /* CONFIG_THREAD_NAME */
}

int z_impl_k_thread_name_copy(k_tid_t thread_id, char *buf, size_t size)
{
#ifdef CONFIG_THREAD_NAME
	strncpy(buf, thread_id->name, size);
	return 0;
#else
	ARG_UNUSED(thread_id);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}

const char *k_thread_state_str(k_tid_t thread_id)
{
	switch (thread_id->base.thread_state) {
	case 0:
		return "";
		break;
	case _THREAD_DUMMY:
		return "dummy";
		break;
	case _THREAD_PENDING:
		return "pending";
		break;
	case _THREAD_PRESTART:
		return "restart";
		break;
	case _THREAD_DEAD:
		return "dead";
		break;
	case _THREAD_SUSPENDED:
		return "suspended";
		break;
	case _THREAD_ABORTING:
		return "aborting";
		break;
	case _THREAD_QUEUED:
		return "queued";
		break;
	}
	return "unknown";
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_name_copy(k_tid_t t, char *buf, size_t size)
{
#ifdef CONFIG_THREAD_NAME
	size_t len;
	struct _k_object *ko = z_object_find(t);

	/* Special case: we allow reading the names of initialized threads
	 * even if we don't have permission on them
	 */
	if (t == NULL || ko->type != K_OBJ_THREAD ||
	    (ko->flags & K_OBJ_FLAG_INITIALIZED) == 0) {
		return -EINVAL;
	}
	if (Z_SYSCALL_MEMORY_WRITE(buf, size) != 0) {
		return -EFAULT;
	}
	len = strlen(t->name);
	if (len + 1 > size) {
		return -ENOSPC;
	}

	return z_user_to_copy((void *)buf, t->name, len + 1);
#else
	ARG_UNUSED(t);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <syscalls/k_thread_name_copy_mrsh.c>
#endif /* CONFIG_USERSPACE */


#ifdef CONFIG_STACK_SENTINEL
/* Check that the stack sentinel is still present
 *
 * The stack sentinel feature writes a magic value to the lowest 4 bytes of
 * the thread's stack when the thread is initialized. This value gets checked
 * in a few places:
 *
 * 1) In k_yield() if the current thread is not swapped out
 * 2) After servicing a non-nested interrupt
 * 3) In z_swap(), check the sentinel in the outgoing thread
 *
 * Item 2 requires support in arch/ code.
 *
 * If the check fails, the thread will be terminated appropriately through
 * the system fatal error handler.
 */
void z_check_stack_sentinel(void)
{
	u32_t *stack;

	if ((_current->base.thread_state & _THREAD_DUMMY) != 0) {
		return;
	}

	stack = (u32_t *)_current->stack_info.start;
	if (*stack != STACK_SENTINEL) {
		/* Restore it so further checks don't trigger this same error */
		*stack = STACK_SENTINEL;
		z_except_reason(K_ERR_STACK_CHK_FAIL);
	}
}
#endif

#ifdef CONFIG_MULTITHREADING
void z_impl_k_thread_start(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&lock); /* protect kernel queues */

	if (z_has_thread_started(thread)) {
		k_spin_unlock(&lock, key);
		return;
	}

	z_mark_thread_as_started(thread);
	z_ready_thread(thread);
	z_reschedule(&lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_start(struct k_thread *thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_start(thread);
}
#include <syscalls/k_thread_start_mrsh.c>
#endif
#endif

#ifdef CONFIG_MULTITHREADING
static void schedule_new_thread(struct k_thread *thread, s32_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (delay == 0) {
		k_thread_start(thread);
	} else {
		s32_t ticks = _TICK_ALIGN + z_ms_to_ticks(delay);

		z_add_thread_timeout(thread, ticks);
	}
#else
	ARG_UNUSED(delay);
	k_thread_start(thread);
#endif
}
#endif

#if !CONFIG_STACK_POINTER_RANDOM
static inline size_t adjust_stack_size(size_t stack_size)
{
	return stack_size;
}
#else
int z_stack_adjust_initialized;

static inline size_t adjust_stack_size(size_t stack_size)
{
	size_t random_val;

	if (!z_stack_adjust_initialized) {
		random_val = z_early_boot_rand32_get();
	} else {
		random_val = sys_rand32_get();
	}

	/* Don't need to worry about alignment of the size here,
	 * z_arch_new_thread() is required to do it.
	 *
	 * FIXME: Not the best way to get a random number in a range.
	 * See #6493
	 */
	const size_t fuzz = random_val % CONFIG_STACK_POINTER_RANDOM;

	if (unlikely(fuzz * 2 > stack_size)) {
		return stack_size;
	}

	return stack_size - fuzz;
}
#if defined(CONFIG_STACK_GROWS_UP)
	/* This is so rare not bothering for now */
#error "Stack pointer randomization not implemented for upward growing stacks"
#endif /* CONFIG_STACK_GROWS_UP */

#endif /* CONFIG_STACK_POINTER_RANDOM */

/*
 * Note:
 * The caller must guarantee that the stack_size passed here corresponds
 * to the amount of stack memory available for the thread.
 */
void z_setup_new_thread(struct k_thread *new_thread,
		       k_thread_stack_t *stack, size_t stack_size,
		       k_thread_entry_t entry,
		       void *p1, void *p2, void *p3,
		       int prio, u32_t options, const char *name)
{
#ifdef CONFIG_USERSPACE
	z_object_init(new_thread);
	z_object_init(stack);
	new_thread->stack_obj = stack;

	/* Any given thread has access to itself */
	k_object_access_grant(new_thread, new_thread);
#endif
	stack_size = adjust_stack_size(stack_size);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
#ifndef CONFIG_THREAD_USERSPACE_LOCAL_DATA_ARCH_DEFER_SETUP
	/* reserve space on top of stack for local data */
	stack_size = STACK_ROUND_DOWN(stack_size
			- sizeof(*new_thread->userspace_local_data));
#endif
#endif

	z_arch_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
			  prio, options);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
#ifndef CONFIG_THREAD_USERSPACE_LOCAL_DATA_ARCH_DEFER_SETUP
	/* don't set again if the arch's own code in z_arch_new_thread() has
	 * already set the pointer.
	 */
	new_thread->userspace_local_data =
		(struct _thread_userspace_local_data *)
		(Z_THREAD_STACK_BUFFER(stack) + stack_size);
#endif
#endif

#ifdef CONFIG_THREAD_MONITOR
	new_thread->entry.pEntry = entry;
	new_thread->entry.parameter1 = p1;
	new_thread->entry.parameter2 = p2;
	new_thread->entry.parameter3 = p3;

	k_spinlock_key_t key = k_spin_lock(&lock);

	new_thread->next_thread = _kernel.threads;
	_kernel.threads = new_thread;
	k_spin_unlock(&lock, key);
#endif
#ifdef CONFIG_THREAD_NAME
	if (name != NULL) {
		strncpy(new_thread->name, name,
			CONFIG_THREAD_MAX_NAME_LEN - 1);
		/* Ensure NULL termination, truncate if longer */
		new_thread->name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';
	}
#endif
#ifdef CONFIG_SCHED_CPU_MASK
	new_thread->base.cpu_mask = -1;
#endif
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	/* _current may be null if the dummy thread is not used */
	if (!_current) {
		new_thread->resource_pool = NULL;
		return;
	}
#endif
#ifdef CONFIG_USERSPACE
	/* New threads inherit any memory domain membership by the parent */
	if (_current->mem_domain_info.mem_domain != NULL) {
		k_mem_domain_add_thread(_current->mem_domain_info.mem_domain,
					new_thread);
	}

	if ((options & K_INHERIT_PERMS) != 0U) {
		z_thread_perms_inherit(_current, new_thread);
	}
#endif
#ifdef CONFIG_SCHED_DEADLINE
	new_thread->base.prio_deadline = 0;
#endif
	new_thread->resource_pool = _current->resource_pool;
	sys_trace_thread_create(new_thread);
}

#ifdef CONFIG_MULTITHREADING
k_tid_t z_impl_k_thread_create(struct k_thread *new_thread,
			      k_thread_stack_t *stack,
			      size_t stack_size, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, s32_t delay)
{
	__ASSERT(!z_arch_is_in_isr(), "Threads may not be created in ISRs");

	/* Special case, only for unit tests */
#if defined(CONFIG_TEST) && defined(CONFIG_ARCH_HAS_USERSPACE) && !defined(CONFIG_USERSPACE)
	__ASSERT((options & K_USER) == 0,
		 "Platform is capable of user mode, and test thread created with K_USER option,"
		 " but neither CONFIG_TEST_USERSPACE nor CONFIG_USERSPACE is set\n");
#endif

	z_setup_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
			  prio, options, NULL);

	if (delay != K_FOREVER) {
		schedule_new_thread(new_thread, delay);
	}

	return new_thread;
}


#ifdef CONFIG_USERSPACE
k_tid_t z_vrfy_k_thread_create(struct k_thread *new_thread,
			       k_thread_stack_t *stack,
			       size_t stack_size, k_thread_entry_t entry,
			       void *p1, void *p2, void *p3,
			       int prio, u32_t options, s32_t delay)
{
	u32_t total_size;
	struct _k_object *stack_object;

	/* The thread and stack objects *must* be in an uninitialized state */
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(new_thread, K_OBJ_THREAD));
	stack_object = z_object_find(stack);
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(z_obj_validation_check(stack_object, stack,
						K_OBJ__THREAD_STACK_ELEMENT,
						_OBJ_INIT_FALSE) == 0,
				    "bad stack object"));

	/* Verify that the stack size passed in is OK by computing the total
	 * size and comparing it with the size value in the object metadata
	 */
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(!u32_add_overflow(K_THREAD_STACK_RESERVED,
						      stack_size, &total_size),
				    "stack size overflow (%u+%u)",
				    (unsigned int) stack_size,
				    K_THREAD_STACK_RESERVED));

	/* Testing less-than-or-equal since additional room may have been
	 * allocated for alignment constraints
	 */
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(total_size <= stack_object->data,
				    "stack size %u is too big, max is %u",
				    total_size, stack_object->data));

	/* User threads may only create other user threads and they can't
	 * be marked as essential
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(options & K_USER));
	Z_OOPS(Z_SYSCALL_VERIFY(!(options & K_ESSENTIAL)));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(_is_valid_prio(prio, NULL)));
	Z_OOPS(Z_SYSCALL_VERIFY(z_is_prio_lower_or_equal(prio,
							_current->base.prio)));

	z_setup_new_thread(new_thread, stack, stack_size,
			   entry, p1, p2, p3, prio, options, NULL);

	if (delay != K_FOREVER) {
		schedule_new_thread(new_thread, delay);
	}

	return new_thread;
}
#include <syscalls/k_thread_create_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */

void z_thread_single_suspend(struct k_thread *thread)
{
	if (z_is_thread_ready(thread)) {
		z_remove_thread_from_ready_q(thread);
	}

	z_mark_thread_as_suspended(thread);
}

void z_impl_k_thread_suspend(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	z_thread_single_suspend(thread);

	sys_trace_thread_suspend(thread);

	if (thread == _current) {
		z_reschedule(&lock, key);
	} else {
		k_spin_unlock(&lock, key);
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_suspend(struct k_thread *thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_suspend(thread);
}
#include <syscalls/k_thread_suspend_mrsh.c>
#endif

void z_thread_single_resume(struct k_thread *thread)
{
	z_mark_thread_as_not_suspended(thread);
	z_ready_thread(thread);
}

void z_impl_k_thread_resume(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	z_thread_single_resume(thread);

	sys_trace_thread_resume(thread);
	z_reschedule(&lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_resume(struct k_thread *thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_resume(thread);
}
#include <syscalls/k_thread_resume_mrsh.c>
#endif

void z_thread_single_abort(struct k_thread *thread)
{
	if (thread->fn_abort != NULL) {
		thread->fn_abort();
	}

	if (IS_ENABLED(CONFIG_SMP)) {
		z_sched_abort(thread);
	}

	if (z_is_thread_ready(thread)) {
		z_remove_thread_from_ready_q(thread);
	} else {
		if (z_is_thread_pending(thread)) {
			z_unpend_thread_no_timeout(thread);
		}
		if (z_is_thread_timeout_active(thread)) {
			(void)z_abort_thread_timeout(thread);
		}
	}

	thread->base.thread_state |= _THREAD_DEAD;

	sys_trace_thread_abort(thread);

#ifdef CONFIG_USERSPACE
	/* Clear initialized state so that this thread object may be re-used
	 * and triggers errors if API calls are made on it from user threads
	 */
	z_object_uninit(thread->stack_obj);
	z_object_uninit(thread);

	/* Revoke permissions on thread's ID so that it may be recycled */
	z_thread_perms_all_clear(thread);
#endif
}

#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_USERSPACE

static void grant_static_access(void)
{
	Z_STRUCT_SECTION_FOREACH(_k_object_assignment, pos) {
		for (int i = 0; pos->objects[i] != NULL; i++) {
			k_object_access_grant(pos->objects[i],
					      pos->thread);
		}
	}
}
#endif /* CONFIG_USERSPACE */

void z_init_static_threads(void)
{
	_FOREACH_STATIC_THREAD(thread_data) {
		z_setup_new_thread(
			thread_data->init_thread,
			thread_data->init_stack,
			thread_data->init_stack_size,
			thread_data->init_entry,
			thread_data->init_p1,
			thread_data->init_p2,
			thread_data->init_p3,
			thread_data->init_prio,
			thread_data->init_options,
			thread_data->init_name);

		thread_data->init_thread->init_data = thread_data;
	}

#ifdef CONFIG_USERSPACE
	grant_static_access();
#endif

	/*
	 * Non-legacy static threads may be started immediately or
	 * after a previously specified delay. Even though the
	 * scheduler is locked, ticks can still be delivered and
	 * processed. Take a sched lock to prevent them from running
	 * until they are all started.
	 *
	 * Note that static threads defined using the legacy API have a
	 * delay of K_FOREVER.
	 */
	k_sched_lock();
	_FOREACH_STATIC_THREAD(thread_data) {
		if (thread_data->init_delay != K_FOREVER) {
			schedule_new_thread(thread_data->init_thread,
					    thread_data->init_delay);
		}
	}
	k_sched_unlock();
}
#endif

void z_init_thread_base(struct _thread_base *thread_base, int priority,
		       u32_t initial_state, unsigned int options)
{
	/* k_q_node is initialized upon first insertion in a list */

	thread_base->user_options = (u8_t)options;
	thread_base->thread_state = (u8_t)initial_state;

	thread_base->prio = priority;

	thread_base->sched_locked = 0U;

#ifdef CONFIG_SMP
	thread_base->is_idle = 0;
#endif

	/* swap_data does not need to be initialized */

	z_init_thread_timeout(thread_base);
}

FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
					    void *p1, void *p2, void *p3)
{
	_current->base.user_options |= K_USER;
	z_thread_essential_clear();
#ifdef CONFIG_THREAD_MONITOR
	_current->entry.pEntry = entry;
	_current->entry.parameter1 = p1;
	_current->entry.parameter2 = p2;
	_current->entry.parameter3 = p3;
#endif
#ifdef CONFIG_USERSPACE
	z_arch_user_mode_enter(entry, p1, p2, p3);
#else
	/* XXX In this case we do not reset the stack */
	z_thread_entry(entry, p1, p2, p3);
#endif
}

/* These spinlock assertion predicates are defined here because having
 * them in spinlock.h is a giant header ordering headache.
 */
#ifdef SPIN_VALIDATE
bool z_spin_lock_valid(struct k_spinlock *l)
{
	uintptr_t thread_cpu = l->thread_cpu;

	if (thread_cpu) {
		if ((thread_cpu & 3) == _current_cpu->id) {
			return false;
		}
	}
	return true;
}

bool z_spin_unlock_valid(struct k_spinlock *l)
{
	if (l->thread_cpu != (_current_cpu->id | (uintptr_t)_current)) {
		return false;
	}
	l->thread_cpu = 0;
	return true;
}

void z_spin_lock_set_owner(struct k_spinlock *l)
{
	l->thread_cpu = _current_cpu->id | (uintptr_t)_current;
}

#endif

int z_impl_k_float_disable(struct k_thread *thread)
{
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	return z_arch_float_disable(thread);
#else
	return -ENOSYS;
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_float_disable(struct k_thread *thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_float_disable(thread);
}
#include <syscalls/k_float_disable_mrsh.c>

static inline void z_vrfy_k_thread_abort(k_tid_t thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(!(thread->base.user_options & K_ESSENTIAL),
				    "aborting essential thread %p", thread));

	z_impl_k_thread_abort((struct k_thread *)thread);
}
#include <syscalls/k_thread_abort_mrsh.c>

#endif /* CONFIG_USERSPACE */
