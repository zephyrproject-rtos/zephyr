/*
 * Copyright (c) Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_RUN_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_RUN_Q_H_

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/spinlock.h>
#include <wait_q.h>
#include <kthread.h>
#include <priority_q.h>
#include <ipi.h>


#ifdef IAR_SUPPRESS_ALWAYS_INLINE_WARNING_FLAG
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_ALWAYS_INLINE)
#endif
static ALWAYS_INLINE void *thread_runq(struct k_thread *thread)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	uint16_t cpu_mask = thread->base.cpu_mask;
	int cpu;

	/* Edge case: it's legal per the API to "make runnable" a
	 * thread with all CPUs masked off (i.e. one that isn't
	 * actually runnable!).  Sort of a wart in the API and maybe
	 * we should address this in docs/assertions instead to avoid
	 * the extra test.
	 */
	if (cpu_mask == 0U) {
		cpu = 0;
	} else {
		cpu = u32_count_trailing_zeros(cpu_mask);
	}

	return &_kernel.cpus[cpu].ready_q.runq;
#else
	ARG_UNUSED(thread);
	return &_kernel.ready_q.runq;
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}

static ALWAYS_INLINE void *curr_cpu_runq(void)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	return &arch_curr_cpu()->ready_q.runq;
#else
	return &_kernel.ready_q.runq;
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}

static ALWAYS_INLINE void runq_add(struct k_thread *thread)
{
	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));
	__ASSERT_NO_MSG(!is_thread_dummy(thread));

	_priq_run_add(thread_runq(thread), thread);
}

static ALWAYS_INLINE void runq_remove(struct k_thread *thread)
{
	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));
	__ASSERT_NO_MSG(!is_thread_dummy(thread));

	_priq_run_remove(thread_runq(thread), thread);
}

static ALWAYS_INLINE void runq_yield(void)
{
	_priq_run_yield(curr_cpu_runq());
}

static ALWAYS_INLINE struct k_thread *runq_best(void)
{
	return _priq_run_best(curr_cpu_runq());
}

/*
 * _current is never in the run queue until context switch on
 * SMP configurations.
 */
static inline bool should_queue_thread(struct k_thread *thread)
{
	return !IS_ENABLED(CONFIG_SMP) || (thread != _current);
}

static ALWAYS_INLINE void queue_thread(struct k_thread *thread)
{
	z_mark_thread_as_queued(thread);
	if (should_queue_thread(thread)) {
		runq_add(thread);
	}
#ifdef CONFIG_SMP
	if (thread == _current) {
		/* add current to end of queue means "yield" */
		_current_cpu->swap_ok = true;
	}
#endif /* CONFIG_SMP */
}

static ALWAYS_INLINE void dequeue_thread(struct k_thread *thread)
{
	z_mark_thread_as_not_queued(thread);
	if (should_queue_thread(thread)) {
		runq_remove(thread);
	}
}
#ifdef IAR_SUPPRESS_ALWAYS_INLINE_WARNING_FLAG
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_ALWAYS_INLINE)
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_RUN_Q_H_ */
