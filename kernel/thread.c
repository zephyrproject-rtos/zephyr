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

#include <kernel_structs.h>
#include <misc/printk.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <ksched.h>
#include <wait_q.h>
#include <atomic.h>
#include <syscall_handler.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <init.h>
#include <tracing.h>

extern struct _static_thread_data _static_thread_data_list_start[];
extern struct _static_thread_data _static_thread_data_list_end[];

#define _FOREACH_STATIC_THREAD(thread_data)              \
	for (struct _static_thread_data *thread_data =   \
	     _static_thread_data_list_start;             \
	     thread_data < _static_thread_data_list_end; \
	     thread_data++)

void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data)
{
#if defined(CONFIG_THREAD_MONITOR)
	struct k_thread *thread;
	unsigned int key;

	__ASSERT(user_cb, "user_cb can not be NULL");

	/*
	 * Lock is needed to make sure that the _kernel.threads is not being
	 * modified by the user_cb either dircetly or indircetly.
	 * The indircet ways are through calling k_thread_create and
	 * k_thread_abort from user_cb.
	 */
	key = irq_lock();
	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		user_cb(thread, user_data);
	}
	irq_unlock(key);
#endif
}

int k_is_in_isr(void)
{
	return _is_in_isr();
}

/*
 * This function tags the current thread as essential to system operation.
 * Exceptions raised by this thread will be treated as a fatal system error.
 */
void _thread_essential_set(void)
{
	_current->base.user_options |= K_ESSENTIAL;
}

/*
 * This function tags the current thread as not essential to system operation.
 * Exceptions raised by this thread may be recoverable.
 * (This is the default tag for a thread.)
 */
void _thread_essential_clear(void)
{
	_current->base.user_options &= ~K_ESSENTIAL;
}

/*
 * This routine indicates if the current thread is an essential system thread.
 *
 * Returns non-zero if current thread is essential, zero if it is not.
 */
int _is_thread_essential(void)
{
	return _current->base.user_options & K_ESSENTIAL;
}

#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
void k_busy_wait(u32_t usec_to_wait)
{
#if defined(CONFIG_TICKLESS_KERNEL) && \
	    !defined(CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK)
int saved_always_on = k_enable_sys_clock_always_on();
#endif
	/* use 64-bit math to prevent overflow when multiplying */
	u32_t cycles_to_wait = (u32_t)(
		(u64_t)usec_to_wait *
		(u64_t)sys_clock_hw_cycles_per_sec /
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
#if defined(CONFIG_TICKLESS_KERNEL) && \
	    !defined(CONFIG_BUSY_WAIT_USES_ALTERNATE_CLOCK)
	_sys_clock_always_on = saved_always_on;
#endif
}
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
void _impl_k_thread_custom_data_set(void *value)
{
	_current->custom_data = value;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_custom_data_set, data)
{
	_impl_k_thread_custom_data_set((void *)data);
	return 0;
}
#endif

void *_impl_k_thread_custom_data_get(void)
{
	return _current->custom_data;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER0_SIMPLE(k_thread_custom_data_get);
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_THREAD_CUSTOM_DATA */

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Remove a thread from the kernel's list of active threads.
 */
void _thread_monitor_exit(struct k_thread *thread)
{
	unsigned int key = irq_lock();

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

	irq_unlock(key);
}
#endif /* CONFIG_THREAD_MONITOR */

#ifdef CONFIG_STACK_SENTINEL
/* Check that the stack sentinel is still present
 *
 * The stack sentinel feature writes a magic value to the lowest 4 bytes of
 * the thread's stack when the thread is initialized. This value gets checked
 * in a few places:
 *
 * 1) In k_yield() if the current thread is not swapped out
 * 2) After servicing a non-nested interrupt
 * 3) In _Swap(), check the sentinel in the outgoing thread
 *
 * Item 2 requires support in arch/ code.
 *
 * If the check fails, the thread will be terminated appropriately through
 * the system fatal error handler.
 */
void _check_stack_sentinel(void)
{
	u32_t *stack;

	if (_current->base.thread_state & _THREAD_DUMMY) {
		return;
	}

	stack = (u32_t *)_current->stack_info.start;
	if (*stack != STACK_SENTINEL) {
		/* Restore it so further checks don't trigger this same error */
		*stack = STACK_SENTINEL;
		_k_except_reason(_NANO_ERR_STACK_CHK_FAIL);
	}
}
#endif

#ifdef CONFIG_MULTITHREADING
void _impl_k_thread_start(struct k_thread *thread)
{
	int key = irq_lock(); /* protect kernel queues */

	if (_has_thread_started(thread)) {
		irq_unlock(key);
		return;
	}

	_mark_thread_as_started(thread);
	_ready_thread(thread);
	_reschedule(key);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_start, K_OBJ_THREAD, struct k_thread *);
#endif
#endif

#ifdef CONFIG_MULTITHREADING
static void schedule_new_thread(struct k_thread *thread, s32_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (delay == 0) {
		k_thread_start(thread);
	} else {
		s32_t ticks = _TICK_ALIGN + _ms_to_ticks(delay);
		int key = irq_lock();

		_add_thread_timeout(thread, NULL, ticks);
		irq_unlock(key);
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

	/* Don't need to worry about alignment of the size here, _new_thread()
	 * is required to do it
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

void _setup_new_thread(struct k_thread *new_thread,
		       k_thread_stack_t *stack, size_t stack_size,
		       k_thread_entry_t entry,
		       void *p1, void *p2, void *p3,
		       int prio, u32_t options)
{
	stack_size = adjust_stack_size(stack_size);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
#ifndef CONFIG_THREAD_USERSPACE_LOCAL_DATA_ARCH_DEFER_SETUP
	/* reserve space on top of stack for local data */
	stack_size = STACK_ROUND_DOWN(stack_size
			- sizeof(*new_thread->userspace_local_data));
#endif
#endif

	_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
		    prio, options);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
#ifndef CONFIG_THREAD_USERSPACE_LOCAL_DATA_ARCH_DEFER_SETUP
	/* don't set again if the arch's own code in _new_thread() has
	 * already set the pointer.
	 */
	new_thread->userspace_local_data =
		(struct _thread_userspace_local_data *)
		(K_THREAD_STACK_BUFFER(stack) + stack_size);
#endif
#endif

#ifdef CONFIG_THREAD_MONITOR
	new_thread->entry.pEntry = entry;
	new_thread->entry.parameter1 = p1;
	new_thread->entry.parameter2 = p2;
	new_thread->entry.parameter3 = p3;

	unsigned int key = irq_lock();

	new_thread->next_thread = _kernel.threads;
	_kernel.threads = new_thread;
	irq_unlock(key);
#endif
#ifdef CONFIG_USERSPACE
	_k_object_init(new_thread);
	_k_object_init(stack);
	new_thread->stack_obj = stack;

	/* Any given thread has access to itself */
	k_object_access_grant(new_thread, new_thread);
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
	if (_current->mem_domain_info.mem_domain) {
		k_mem_domain_add_thread(_current->mem_domain_info.mem_domain,
					new_thread);
	}

	if (options & K_INHERIT_PERMS) {
		_thread_perms_inherit(_current, new_thread);
	}
#endif
#ifdef CONFIG_SCHED_DEADLINE
	new_thread->base.prio_deadline = 0;
#endif
	new_thread->resource_pool = _current->resource_pool;
	sys_trace_thread_create(new_thread);
}

#ifdef CONFIG_MULTITHREADING
k_tid_t _impl_k_thread_create(struct k_thread *new_thread,
			      k_thread_stack_t *stack,
			      size_t stack_size, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, s32_t delay)
{
	__ASSERT(!_is_in_isr(), "Threads may not be created in ISRs");

	_setup_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
			  prio, options);

	if (delay != K_FOREVER) {
		schedule_new_thread(new_thread, delay);
	}

	return new_thread;
}


#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_create,
		  new_thread_p, stack_p, stack_size, entry, p1, more_args)
{
	int prio;
	u32_t options, delay;
	u32_t total_size;
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	u32_t guard_size;
#endif
	struct _k_object *stack_object;
	struct k_thread *new_thread = (struct k_thread *)new_thread_p;
	volatile struct _syscall_10_args *margs =
		(volatile struct _syscall_10_args *)more_args;
	k_thread_stack_t *stack = (k_thread_stack_t *)stack_p;

	/* The thread and stack objects *must* be in an uninitialized state */
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(new_thread, K_OBJ_THREAD));
	stack_object = _k_object_find(stack);
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(!_obj_validation_check(stack_object, stack,
						K_OBJ__THREAD_STACK_ELEMENT,
						_OBJ_INIT_FALSE),
				    "bad stack object"));

#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	/* Verify that the stack size passed in is OK by computing the total
	 * size and comparing it with the size value in the object metadata
	 *
	 * We skip this check for SoCs which utilize MPUs with power of two
	 * alignment requirements as the guard is allocated out of the stack
	 * size and not allocated in addition to the stack size
	 */
	guard_size = (u32_t)K_THREAD_STACK_BUFFER(stack) - (u32_t)stack;
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(!__builtin_uadd_overflow(guard_size,
							     stack_size,
							     &total_size),
				    "stack size overflow (%u+%u)", stack_size,
				    guard_size));
#else
	total_size = stack_size;
#endif
	/* They really ought to be equal, make this more strict? */
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(total_size <= stack_object->data,
				    "stack size %u is too big, max is %u",
				    total_size, stack_object->data));

	/* Verify the struct containing args 6-10 */
	Z_OOPS(Z_SYSCALL_MEMORY_READ(margs, sizeof(*margs)));

	/* Stash struct arguments in local variables to prevent switcheroo
	 * attacks
	 */
	prio = margs->arg8;
	options = margs->arg9;
	delay = margs->arg10;
	compiler_barrier();

	/* User threads may only create other user threads and they can't
	 * be marked as essential
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(options & K_USER));
	Z_OOPS(Z_SYSCALL_VERIFY(!(options & K_ESSENTIAL)));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(_is_valid_prio(prio, NULL)));
	Z_OOPS(Z_SYSCALL_VERIFY(_is_prio_lower_or_equal(prio,
							_current->base.prio)));

	_setup_new_thread((struct k_thread *)new_thread, stack, stack_size,
			  (k_thread_entry_t)entry, (void *)p1,
			  (void *)margs->arg6, (void *)margs->arg7, prio,
			  options);

	if (delay != K_FOREVER) {
		schedule_new_thread(new_thread, delay);
	}

	return new_thread_p;
}
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */

/* LCOV_EXCL_START */
int _impl_k_thread_cancel(k_tid_t tid)
{
	struct k_thread *thread = tid;

	unsigned int key = irq_lock();

	if (_has_thread_started(thread) ||
	    !_is_thread_timeout_active(thread)) {
		irq_unlock(key);
		return -EINVAL;
	}

	(void)_abort_thread_timeout(thread);
	_thread_monitor_exit(thread);

	irq_unlock(key);

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE(k_thread_cancel, K_OBJ_THREAD, struct k_thread *);
#endif
/* LCOV_EXCL_STOP */

void _k_thread_single_suspend(struct k_thread *thread)
{
	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	}

	_mark_thread_as_suspended(thread);
}

void _impl_k_thread_suspend(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_suspend(thread);

	sys_trace_thread_suspend(thread);

	if (thread == _current) {
		(void)_Swap(key);
	} else {
		irq_unlock(key);
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_suspend, K_OBJ_THREAD, k_tid_t);
#endif

void _k_thread_single_resume(struct k_thread *thread)
{
	_mark_thread_as_not_suspended(thread);
	_ready_thread(thread);
}

void _impl_k_thread_resume(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_resume(thread);

	sys_trace_thread_resume(thread);
	_reschedule(key);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_thread_resume, K_OBJ_THREAD, k_tid_t);
#endif

void _k_thread_single_abort(struct k_thread *thread)
{
	if (thread->fn_abort != NULL) {
		thread->fn_abort();
	}

	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	} else {
		if (_is_thread_pending(thread)) {
			_unpend_thread_no_timeout(thread);
		}
		if (_is_thread_timeout_active(thread)) {
			(void)_abort_thread_timeout(thread);
		}
	}

	thread->base.thread_state |= _THREAD_DEAD;

	sys_trace_thread_abort(thread);

#ifdef CONFIG_USERSPACE
	/* Clear initailized state so that this thread object may be re-used
	 * and triggers errors if API calls are made on it from user threads
	 */
	_k_object_uninit(thread->stack_obj);
	_k_object_uninit(thread);

	/* Revoke permissions on thread's ID so that it may be recycled */
	_thread_perms_all_clear(thread);
#endif
}

#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_USERSPACE
extern char __object_access_start[];
extern char __object_access_end[];

static void grant_static_access(void)
{
	struct _k_object_assignment *pos;

	for (pos = (struct _k_object_assignment *)__object_access_start;
	     pos < (struct _k_object_assignment *)__object_access_end;
	     pos++) {
		for (int i = 0; pos->objects[i] != NULL; i++) {
			k_object_access_grant(pos->objects[i],
					      pos->thread);
		}
	}
}
#endif /* CONFIG_USERSPACE */

void _init_static_threads(void)
{
	unsigned int  key;

	_FOREACH_STATIC_THREAD(thread_data) {
		_setup_new_thread(
			thread_data->init_thread,
			thread_data->init_stack,
			thread_data->init_stack_size,
			thread_data->init_entry,
			thread_data->init_p1,
			thread_data->init_p2,
			thread_data->init_p3,
			thread_data->init_prio,
			thread_data->init_options);

		thread_data->init_thread->init_data = thread_data;
	}

#ifdef CONFIG_USERSPACE
	grant_static_access();
#endif
	_sched_lock();

	/*
	 * Non-legacy static threads may be started immediately or after a
	 * previously specified delay. Even though the scheduler is locked,
	 * ticks can still be delivered and processed. Lock interrupts so
	 * that the countdown until execution begins from the same tick.
	 *
	 * Note that static threads defined using the legacy API have a
	 * delay of K_FOREVER.
	 */
	key = irq_lock();
	_FOREACH_STATIC_THREAD(thread_data) {
		if (thread_data->init_delay != K_FOREVER) {
			schedule_new_thread(thread_data->init_thread,
					    thread_data->init_delay);
		}
	}
	irq_unlock(key);
	k_sched_unlock();
}
#endif

void _init_thread_base(struct _thread_base *thread_base, int priority,
		       u32_t initial_state, unsigned int options)
{
	/* k_q_node is initialized upon first insertion in a list */

	thread_base->user_options = (u8_t)options;
	thread_base->thread_state = (u8_t)initial_state;

	thread_base->prio = priority;

	thread_base->sched_locked = 0;

	/* swap_data does not need to be initialized */

	_init_thread_timeout(thread_base);
}

void k_thread_access_grant(struct k_thread *thread, ...)
{
#ifdef CONFIG_USERSPACE
	va_list args;
	va_start(args, thread);

	while (true) {
		void *object = va_arg(args, void *);
		if (object == NULL) {
			break;
		}
		k_object_access_grant(object, thread);
	}
	va_end(args);
#else
	ARG_UNUSED(thread);
#endif
}

FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
					    void *p1, void *p2, void *p3)
{
	_current->base.user_options |= K_USER;
	_thread_essential_clear();
#ifdef CONFIG_THREAD_MONITOR
	_current->entry.pEntry = entry;
	_current->entry.parameter1 = p1;
	_current->entry.parameter2 = p2;
	_current->entry.parameter3 = p3;
#endif
#ifdef CONFIG_USERSPACE
	_arch_user_mode_enter(entry, p1, p2, p3);
#else
	/* XXX In this case we do not reset the stack */
	_thread_entry(entry, p1, p2, p3);
#endif
}
