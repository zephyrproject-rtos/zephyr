/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Hooks through which the common kernel code reaches the SMP support in
 * kernel/smp/. Each hook has a uniprocessor variant at the end of this file
 * that compiles to nothing, so the callers carry no SMP conditionals.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSMP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSMP_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <wait_q.h>

#ifdef CONFIG_SMP

/* Architecture SMP setup, run between PRE_KERNEL_1 and PRE_KERNEL_2. */
static inline void z_smp_arch_init(void)
{
	(void)arch_smp_init();
}

/* Start the secondary CPUs once the static threads are ready. */
void z_smp_init(void);

/* Initialize the per-CPU state that only exists on SMP. */
static inline void z_smp_cpu_init(struct _cpu *cpu)
{
#ifdef CONFIG_SCHED_IPI_SUPPORTED
	sys_dlist_init(&cpu->ipi_workq);
#else
	ARG_UNUSED(cpu);
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
}

/* Initialize the thread base fields that only exist on SMP. */
static inline void z_smp_thread_base_init(struct _thread_base *base)
{
	base->is_idle = 0;

	/* Pretend that the thread was last executing on CPU0 to prevent
	 * out-of-bounds memory accesses to the _kernel.cpus[] array.
	 */
	base->cpu = 0;
}

static inline void z_smp_thread_mark_idle(struct k_thread *thread)
{
	thread->base.is_idle = 1U;
}

/* Wait queue on which the thread suspending @p thread waits while
 * @p thread runs on another CPU.
 */
static inline void z_smp_halt_queue_init(struct k_thread *thread)
{
	z_waitq_init(&thread->halt_queue);
}

/* Wake the threads waiting on the halt queue. Scheduler spinlock held. */
void z_smp_halt_queue_wake(struct k_thread *thread);

/* Return the CPU on which @p thread is currently running, or NULL when it
 * is not running on another CPU.
 */
static inline struct _cpu *z_smp_thread_active_elsewhere(struct k_thread *thread)
{
	int thread_cpu_id = thread->base.cpu;
	struct _cpu *thread_cpu;

	__ASSERT_NO_MSG((thread_cpu_id >= 0) && (thread_cpu_id < arch_num_cpus()));

	thread_cpu = &_kernel.cpus[thread_cpu_id];
	if ((thread_cpu->current == thread) && (thread_cpu != _current_cpu)) {
		return thread_cpu;
	}

	return NULL;
}

/* Halt @p thread, which runs on @p cpu: flag it, poke the CPU and wait
 * until the thread has halted itself. Releases the scheduler spinlock
 * @p key before returning.
 */
void z_smp_halt_remote(struct k_thread *thread, struct _cpu *cpu, k_spinlock_key_t key,
		       bool terminate);

/* Clear the halting bits (_THREAD_ABORTING and _THREAD_SUSPENDING). */
static inline void z_smp_clear_halting(struct k_thread *thread)
{
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		barrier_dmem_fence_full(); /* Other cpus spin on this locklessly! */
		thread->base.thread_state &= ~(_THREAD_ABORTING | _THREAD_SUSPENDING);
	}
}

/* Record whether _current may be switched out cooperatively. The CPU
 * record keeps the flag until the next reschedule call or context switch.
 * It doesn't need to be tracked per thread because if the thread gets
 * preempted for whatever reason the scheduler will make the same decision
 * anyway.
 */
static inline void z_smp_swap_ok_set(bool ok)
{
	_current_cpu->swap_ok = ok;
}

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either z_swap() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void z_smp_release_global_lock(struct k_thread *thread);

/* Select the thread to switch to. Scheduler spinlock held. */
struct k_thread *z_swap_next_thread(void);

/* Bind @p new_thread to this CPU before z_swap() switches to it. */
static inline void z_smp_swap_prepare(struct k_thread *new_thread, bool release_global_lock)
{
	new_thread->base.cpu = arch_curr_cpu()->id;

	if (release_global_lock) {
		z_smp_release_global_lock(new_thread);
	}
}

/* Spin, with the scheduler lock held (!), on a thread that is known
 * (!!) to have released the lock and be on a path where it will
 * deterministically (!!!) reach arch_switch() in very small constant
 * time.
 *
 * This exists to treat an unavoidable SMP race when threads swap --
 * their thread record is in the queue (and visible to other CPUs)
 * before arch_switch() finishes saving state.  We must spin for the
 * switch handle before entering a new thread.  See docs on
 * arch_switch().
 *
 * Stated differently: there's a chicken and egg bug with the question
 * of "is a thread running or not?".  The thread needs to mark itself
 * "not running" from its own context, but at that moment it obviously
 * is still running until it reaches arch_switch()!  Locking can't
 * treat this because the scheduler lock can't be released by the
 * switched-to thread, which is going to (obviously) be running its
 * own code and doesn't know it was switched out.
 */
static inline void z_sched_switch_spin(struct k_thread *thread)
{
	volatile void **shp = (void *)&thread->switch_handle;

	while (*shp == NULL) {
		arch_spin_relax();
	}
	/* Read barrier: don't allow any subsequent loads in the
	 * calling code to reorder before we saw switch_handle go
	 * non-null.
	 */
	barrier_dmem_fence_full();
}

/* Active threads must have a NULL switch handle, and it must be seen
 * before the scheduler lock is released.
 */
static inline void z_smp_switch_handle_clear(struct k_thread *thread)
{
	thread->switch_handle = NULL;
	barrier_dmem_fence_full(); /* write barrier */
}

/* Put the outgoing @p old_thread back in the run queue once the switch
 * away from it is certain, and send the pending IPIs.
 */
void z_smp_switch_requeue(struct k_thread *old_thread);

#else /* !CONFIG_SMP */

static inline void z_smp_arch_init(void)
{
}

static inline void z_smp_init(void)
{
}

static inline void z_smp_cpu_init(struct _cpu *cpu)
{
	ARG_UNUSED(cpu);
}

static inline void z_smp_thread_base_init(struct _thread_base *base)
{
	ARG_UNUSED(base);
}

static inline void z_smp_thread_mark_idle(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_smp_halt_queue_init(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_smp_halt_queue_wake(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline struct _cpu *z_smp_thread_active_elsewhere(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return NULL;
}

static inline void z_smp_halt_remote(struct k_thread *thread, struct _cpu *cpu,
				     k_spinlock_key_t key, bool terminate)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(cpu);
	ARG_UNUSED(key);
	ARG_UNUSED(terminate);
}

static inline void z_smp_clear_halting(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_smp_swap_ok_set(bool ok)
{
	ARG_UNUSED(ok);
}

static inline void z_smp_swap_prepare(struct k_thread *new_thread, bool release_global_lock)
{
	ARG_UNUSED(new_thread);
	ARG_UNUSED(release_global_lock);
}

static inline void z_sched_switch_spin(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_smp_switch_handle_clear(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline void z_smp_switch_requeue(struct k_thread *old_thread)
{
	ARG_UNUSED(old_thread);
}

#endif /* CONFIG_SMP */

#endif /* ZEPHYR_KERNEL_INCLUDE_KSMP_H_ */
