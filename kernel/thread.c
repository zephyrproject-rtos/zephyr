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
#include <sections.h>

#include <kernel_structs.h>
#include <misc/printk.h>
#include <sys_clock.h>
#include <drivers/system_timer.h>
#include <ksched.h>
#include <wait_q.h>

extern struct _static_thread_data _static_thread_data_list_start[];
extern struct _static_thread_data _static_thread_data_list_end[];

#define _FOREACH_STATIC_THREAD(thread_data)              \
	for (struct _static_thread_data *thread_data =   \
	     _static_thread_data_list_start;             \
	     thread_data < _static_thread_data_list_end; \
	     thread_data++)

#ifdef CONFIG_FP_SHARING
static inline void _task_group_adjust(struct _static_thread_data *thread_data)
{
	/*
	 * set thread options corresponding to legacy FPU and SSE task groups
	 * so thread spawns properly; EXE and SYS task groups need no adjustment
	 */
	if (thread_data->init_groups & K_TASK_GROUP_FPU) {
		thread_data->init_options |= K_FP_REGS;
	}
#ifdef CONFIG_SSE
	if (thread_data->init_groups & K_TASK_GROUP_SSE) {
		thread_data->init_options |= K_SSE_REGS;
	}
#endif /* CONFIG_SSE */
}
#else
#define _task_group_adjust(thread_data) do { } while (0)
#endif /* CONFIG_FP_SHARING */

/* Legacy API */
#if defined(CONFIG_LEGACY_KERNEL)
int sys_execution_context_type_get(void)
{
	if (k_is_in_isr())
		return NANO_CTX_ISR;

	if (_current->base.prio < 0)
		return NANO_CTX_FIBER;

	return NANO_CTX_TASK;
}
#endif

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

void k_busy_wait(uint32_t usec_to_wait)
{
	/* use 64-bit math to prevent overflow when multiplying */
	uint32_t cycles_to_wait = (uint32_t)(
		(uint64_t)usec_to_wait *
		(uint64_t)sys_clock_hw_cycles_per_sec /
		(uint64_t)USEC_PER_SEC
	);
	uint32_t start_cycles = k_cycle_get_32();

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}

#ifdef CONFIG_THREAD_CUSTOM_DATA

void k_thread_custom_data_set(void *value)
{
	_current->custom_data = value;
}

void *k_thread_custom_data_get(void)
{
	return _current->custom_data;
}

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
		while (thread != prev_thread->next_thread) {
			prev_thread = prev_thread->next_thread;
		}
		prev_thread->next_thread = thread->next_thread;
	}

	irq_unlock(key);
}
#endif /* CONFIG_THREAD_MONITOR */

/*
 * Common thread entry point function (used by all threads)
 *
 * This routine invokes the actual thread entry point function and passes
 * it three arguments. It also handles graceful termination of the thread
 * if the entry point function ever returns.
 *
 * This routine does not return, and is marked as such so the compiler won't
 * generate preamble code that is only used by functions that actually return.
 */
FUNC_NORETURN void _thread_entry(void (*entry)(void *, void *, void *),
				 void *p1, void *p2, void *p3)
{
	entry(p1, p2, p3);

#ifdef CONFIG_MULTITHREADING
	if (_is_thread_essential()) {
		_NanoFatalErrorHandler(_NANO_ERR_INVALID_TASK_EXIT,
				       &_default_esf);
	}

	k_thread_abort(_current);
#else
	for (;;) {
		k_cpu_idle();
	}
#endif

	/*
	 * Compiler can't tell that k_thread_abort() won't return and issues a
	 * warning unless we tell it that control never gets this far.
	 */

	CODE_UNREACHABLE;
}

#ifdef CONFIG_MULTITHREADING
static void start_thread(struct k_thread *thread)
{
	int key = irq_lock(); /* protect kernel queues */

	_mark_thread_as_started(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
		if (_must_switch_threads()) {
			_Swap(key);
			return;
		}
	}

	irq_unlock(key);
}
#endif

#ifdef CONFIG_MULTITHREADING
static void schedule_new_thread(struct k_thread *thread, int32_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (delay == 0) {
		start_thread(thread);
	} else {
		int32_t ticks = _TICK_ALIGN + _ms_to_ticks(delay);
		int key = irq_lock();

		_add_thread_timeout(thread, NULL, ticks);
		irq_unlock(key);
	}
#else
	ARG_UNUSED(delay);
	start_thread(thread);
#endif
}
#endif

#ifdef CONFIG_MULTITHREADING
k_tid_t k_thread_spawn(char *stack, size_t stack_size,
			void (*entry)(void *, void *, void*),
			void *p1, void *p2, void *p3,
			int prio, uint32_t options, int32_t delay)
{
	__ASSERT(!_is_in_isr(), "");

	struct k_thread *new_thread = (struct k_thread *)stack;

	_new_thread(stack, stack_size, entry, p1, p2, p3, prio, options);

	schedule_new_thread(new_thread, delay);

	return new_thread;
}
#endif

int k_thread_cancel(k_tid_t tid)
{
	struct k_thread *thread = tid;

	int key = irq_lock();

	if (_has_thread_started(thread) ||
	    !_is_thread_timeout_active(thread)) {
		irq_unlock(key);
		return -EINVAL;
	}

	_abort_thread_timeout(thread);
	_thread_monitor_exit(thread);

	irq_unlock(key);

	return 0;
}

static inline int is_in_any_group(struct _static_thread_data *thread_data,
				  uint32_t groups)
{
	return !!(thread_data->init_groups & groups);
}

void _k_thread_group_op(uint32_t groups, void (*func)(struct k_thread *))
{
	unsigned int  key;

	__ASSERT(!_is_in_isr(), "");

	_sched_lock();

	/* Invoke func() on each static thread in the specified group set. */

	_FOREACH_STATIC_THREAD(thread_data) {
		if (is_in_any_group(thread_data, groups)) {
			key = irq_lock();
			func(thread_data->thread);
			irq_unlock(key);
		}
	}

	/*
	 * If the current thread is still in a ready state, then let the
	 * "unlock scheduler" code determine if any rescheduling is needed.
	 */
	if (_is_thread_ready(_current)) {
		k_sched_unlock();
		return;
	}

	/* The current thread is no longer in a ready state--reschedule. */
	key = irq_lock();
	_sched_unlock_no_reschedule();
	_Swap(key);
}

void _k_thread_single_start(struct k_thread *thread)
{
	_mark_thread_as_started(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}
}

void _k_thread_single_suspend(struct k_thread *thread)
{
	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	}

	_mark_thread_as_suspended(thread);
}

void k_thread_suspend(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_suspend(thread);

	if (thread == _current) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

void _k_thread_single_resume(struct k_thread *thread)
{
	_mark_thread_as_not_suspended(thread);

	if (_is_thread_ready(thread)) {
		_add_thread_to_ready_q(thread);
	}
}

void k_thread_resume(struct k_thread *thread)
{
	unsigned int  key = irq_lock();

	_k_thread_single_resume(thread);

	_reschedule_threads(key);
}

void _k_thread_single_abort(struct k_thread *thread)
{
	if (thread->fn_abort != NULL) {
		thread->fn_abort();
	}

	if (_is_thread_ready(thread)) {
		_remove_thread_from_ready_q(thread);
	} else {
		if (_is_thread_pending(thread)) {
			_unpend_thread(thread);
		}
		if (_is_thread_timeout_active(thread)) {
			_abort_thread_timeout(thread);
		}
	}
	_mark_thread_as_dead(thread);
}

#ifdef CONFIG_MULTITHREADING
void _init_static_threads(void)
{
	unsigned int  key;

	_FOREACH_STATIC_THREAD(thread_data) {
		_task_group_adjust(thread_data);
		_new_thread(
			thread_data->init_stack,
			thread_data->init_stack_size,
			thread_data->init_entry,
			thread_data->init_p1,
			thread_data->init_p2,
			thread_data->init_p3,
			thread_data->init_prio,
			thread_data->init_options);

		thread_data->thread->init_data = thread_data;
	}

	_sched_lock();
#if defined(CONFIG_LEGACY_KERNEL)
	/* Start all (legacy) threads that are part of the EXE task group */
	_k_thread_group_op(K_TASK_GROUP_EXE, _k_thread_single_start);
#endif

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
			schedule_new_thread(thread_data->thread,
					    thread_data->init_delay);
		}
	}
	irq_unlock(key);
	k_sched_unlock();
}
#endif

void _init_thread_base(struct _thread_base *thread_base, int priority,
		       uint32_t initial_state, unsigned int options)
{
	/* k_q_node is initialized upon first insertion in a list */

	thread_base->user_options = (uint8_t)options;
	thread_base->thread_state = (uint8_t)initial_state;

	thread_base->prio = priority;

	thread_base->sched_locked = 0;

	/* swap_data does not need to be initialized */

	_init_thread_timeout(thread_base);
}

uint32_t _k_thread_group_mask_get(struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	return thread_data->init_groups;
}

void _k_thread_group_join(uint32_t groups, struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	thread_data->init_groups |= groups;
}

void _k_thread_group_leave(uint32_t groups, struct k_thread *thread)
{
	struct _static_thread_data *thread_data = thread->init_data;

	thread_data->init_groups &= groups;
}

/* legacy API */
#if defined(CONFIG_LEGACY_KERNEL)
void task_start(ktask_t task)
{
	int key = irq_lock();

	_k_thread_single_start(task);
	_reschedule_threads(key);
}
#endif
