/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/spinlock.h>
#include <wait_q.h>
#include <kthread.h>
#include <priority_q.h>
#include <kswap.h>
#include <ipi.h>
#include <kernel_arch_func.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <stdbool.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/util.h>
#include <metairq.h>
#include <run_q.h>
#include <timeslicing.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* pending_current is owned by timeslicing.c; sleep.c also accesses it */
#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_TIMESLICING)
extern struct k_thread *pending_current;
#endif

struct k_spinlock _sched_spinlock;

/* Storage to "complete" the context switch from an invalid/incomplete thread
 * context (ex: exiting an ISR that aborted _current)
 */
__incoherent struct k_thread _thread_dummy;

static ALWAYS_INLINE void update_cache(int preempt_ok);
static ALWAYS_INLINE void halt_thread(struct k_thread *thread, uint8_t new_state);
static void add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q);

/* Clear the halting bits (_THREAD_ABORTING and _THREAD_SUSPENDING) */
static inline void clear_halting(struct k_thread *thread)
{
	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		barrier_dmem_fence_full(); /* Other cpus spin on this locklessly! */
		thread->base.thread_state &= ~(_THREAD_ABORTING | _THREAD_SUSPENDING);
	}
}

static ALWAYS_INLINE struct k_thread *next_up(void)
{
#ifdef CONFIG_SMP
	if (z_is_thread_halting(_current)) {
		halt_thread(_current, z_is_thread_aborting(_current) ?
				      _THREAD_DEAD : _THREAD_SUSPENDED);
	}
#endif /* CONFIG_SMP */

	struct k_thread *thread = runq_best();

	thread = metairq_preempt_recover(thread);

#ifndef CONFIG_SMP
	/* In uniprocessor mode, we can leave the current thread in
	 * the queue (actually we have to, otherwise the assembly
	 * context switch code for all architectures would be
	 * responsible for putting it back in z_swap and ISR return!),
	 * which makes this choice simple.
	 */
	return (thread != NULL) ? thread : _current_cpu->idle_thread;
#else
	/* Under SMP, the "cache" mechanism for selecting the next
	 * thread doesn't work, so we have more work to do to test
	 * _current against the best choice from the queue.  Here, the
	 * thread selected above represents "the best thread that is
	 * not current".
	 *
	 * Subtle note on "queued": in SMP mode, neither _current nor
	 * metairq_premepted live in the queue, so this isn't exactly the
	 * same thing as "ready", it means "the thread already been
	 * added back to the queue such that we don't want to re-add it".
	 */
	bool queued = z_is_thread_queued(_current);
	bool active = z_is_thread_ready(_current);

	if (thread == NULL) {
		thread = _current_cpu->idle_thread;
	}

	if (active) {
		int32_t cmp = z_sched_prio_cmp(_current, thread);

		/* Ties only switch if state says we yielded */
		if ((cmp > 0) || ((cmp == 0) && !_current_cpu->swap_ok)) {
			thread = _current;
		}

		if (!should_preempt(thread, _current_cpu->swap_ok)) {
			thread = _current;
		}
	}

	if (thread != _current) {
		update_metairq_preempt(thread);
		/*
		 * Put _current back into the queue unless it is ..
		 * 1. not active (i.e., blocked, suspended, dead), or
		 * 2. already queued, or
		 * 3. the idle thread, or
		 * 4. preempted by a MetaIRQ thread
		 */
		if (active && !queued && !z_is_idle_thread_object(_current)
		    && metairq_current_requeue_allowed()) {
			queue_thread(_current);
		}
	}

	/* Take the new _current out of the queue */
	if (z_is_thread_queued(thread)) {
		dequeue_thread(thread);
	}

	_current_cpu->swap_ok = false;
	return thread;
#endif /* CONFIG_SMP */
}

void move_current_to_end_of_prio_q(void)
{
	runq_yield();

	update_cache(1);
}

static ALWAYS_INLINE void update_cache(int preempt_ok)
{
#ifndef CONFIG_SMP
	struct k_thread *thread = next_up();

	if (should_preempt(thread, preempt_ok)) {
#ifdef CONFIG_TIMESLICING
		if (thread != _current) {
			z_time_slice_reset(thread);
		}
#endif /* CONFIG_TIMESLICING */
		update_metairq_preempt(thread);
		_kernel.ready_q.cache = thread;
	} else {
		_kernel.ready_q.cache = _current;
	}

#else
	/* The way this works is that the CPU record keeps its
	 * "cooperative swapping is OK" flag until the next reschedule
	 * call or context switch.  It doesn't need to be tracked per
	 * thread because if the thread gets preempted for whatever
	 * reason the scheduler will make the same decision anyway.
	 */
	_current_cpu->swap_ok = preempt_ok;
#endif /* CONFIG_SMP */
}

/**
 * Returns pointer to _cpu if the thread is currently running on
 * another CPU.
 */
static struct _cpu *thread_active_elsewhere(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	int thread_cpu_id = thread->base.cpu;
	struct _cpu *thread_cpu;

	__ASSERT_NO_MSG((thread_cpu_id >= 0) &&
			(thread_cpu_id < arch_num_cpus()));

	thread_cpu = &_kernel.cpus[thread_cpu_id];
	if ((thread_cpu->current == thread) && (thread_cpu != _current_cpu)) {
		return thread_cpu;
	}

#endif /* CONFIG_SMP */
	ARG_UNUSED(thread);
	return NULL;
}

static inline void ready_thread(struct k_thread *thread)
{
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(sys_cache_is_mem_coherent(thread));
#endif /* CONFIG_KERNEL_COHERENCE */

	/* If thread is queued already, do not try and added it to the
	 * run queue again
	 */
	if (!z_is_thread_queued(thread) && z_is_thread_ready(thread)) {
		SYS_PORT_TRACING_OBJ_FUNC(k_thread, sched_ready, thread);

		queue_thread(thread);
		update_cache(0);

		flag_ipi(ipi_mask_create(thread));
	}
}

void z_ready_thread(struct k_thread *thread)
{
	K_SPINLOCK(&_sched_spinlock) {
		if (thread_active_elsewhere(thread) == NULL) {
			ready_thread(thread);
		}
	}
}

void z_sched_ready_locked(struct k_thread *thread)
{
	ready_thread(thread);
}

static void unready_thread(struct k_thread *thread)
{
	if (z_is_thread_queued(thread)) {
		dequeue_thread(thread);
	}
	update_cache(thread == _current);
}

/* This routine exists for benchmarking purposes. It is not used in
 * general production code.
 */
void z_unready_thread(struct k_thread *thread)
{
	K_SPINLOCK(&_sched_spinlock) {
		unready_thread(thread);
	}
}


void z_sched_unready_locked(struct k_thread *thread)
{
	unready_thread(thread);
}

/* This routine only used for testing purposes */
void z_yield_testing_only(void)
{
	K_SPINLOCK(&_sched_spinlock) {
		move_current_to_end_of_prio_q();
	}
}

/* Spins in ISR context, waiting for a thread known to be running on
 * another CPU to catch the IPI we sent and halt.  Note that we check
 * for ourselves being asynchronously halted first to prevent simple
 * deadlocks (but not complex ones involving cycles of 3+ threads!).
 * Acts to release the provided lock before returning.
 */
static void thread_halt_spin(struct k_thread *thread, k_spinlock_key_t key)
{
	if (z_is_thread_halting(_current)) {
		halt_thread(_current,
			    z_is_thread_aborting(_current) ? _THREAD_DEAD : _THREAD_SUSPENDED);
	}
	k_spin_unlock(&_sched_spinlock, key);
	while (z_is_thread_halting(thread)) {
		unsigned int k = arch_irq_lock();

		arch_spin_relax(); /* Requires interrupts be masked */
		arch_irq_unlock(k);
	}
}

/* Shared handler for k_thread_{suspend,abort}().  Called with the
 * scheduler lock held and the key passed (which it may
 * release/reacquire!) which will be released before a possible return
 * (aborting _current will not return, obviously), which may be after
 * a context switch.
 */
void z_thread_halt(struct k_thread *thread, k_spinlock_key_t key,
					bool terminate)
{
	_wait_q_t *wq = &thread->join_queue;
#ifdef CONFIG_SMP
	wq = terminate ? wq : &thread->halt_queue;
#endif

	z_metairq_preempted_clear(thread);

	/* If the target is a thread running on another CPU, flag and
	 * poke (note that we might spin to wait, so a true
	 * synchronous IPI is needed here, not deferred!), it will
	 * halt itself in the IPI.  Otherwise it's unscheduled, so we
	 * can clean it up directly.
	 */

	struct _cpu *cpu = thread_active_elsewhere(thread);

	if (cpu != NULL) {
		thread->base.thread_state |= (terminate ? _THREAD_ABORTING
					      : _THREAD_SUSPENDING);
#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
		arch_sched_directed_ipi(IPI_CPU_MASK(cpu->id));
#else
		arch_sched_broadcast_ipi();
#endif /* CONFIG_ARCH_HAS_DIRECTED_IPIS */
#endif /* CONFIG_SMP && CONFIG_SCHED_IPI_SUPPORTED */
		if (arch_is_in_isr()) {
			thread_halt_spin(thread, key);
		} else  {
			add_to_waitq_locked(_current, wq);
			z_swap(&_sched_spinlock, key);
		}
	} else {
		halt_thread(thread, terminate ? _THREAD_DEAD : _THREAD_SUSPENDED);
		if ((thread == _current) && !arch_is_in_isr()) {
			if (z_is_thread_essential(thread)) {
				k_spin_unlock(&_sched_spinlock, key);
				k_panic();
				key = k_spin_lock(&_sched_spinlock);
			}
			z_swap(&_sched_spinlock, key);
			__ASSERT(!terminate, "aborted _current back from dead");
		} else {
			k_spin_unlock(&_sched_spinlock, key);
		}
	}
	/* NOTE: the scheduler lock has been released.  Don't put
	 * logic here, it's likely to be racy/deadlocky even if you
	 * re-take the lock!
	 */
}

static inline bool resched(uint32_t key)
{
#ifdef CONFIG_SMP
	_current_cpu->swap_ok = 0;
#endif /* CONFIG_SMP */

	return arch_irq_unlocked(key) && !arch_is_in_isr();
}

/*
 * Check if the next ready thread is the same as the current thread
 * and save the trip if true.
 */
static inline bool need_swap(void)
{
	/* the SMP case will be handled in C based z_swap() */
#ifdef CONFIG_SMP
	return true;
#else
	struct k_thread *new_thread;

	/* Check if the next ready thread is the same as the current thread */
	new_thread = _kernel.ready_q.cache;
	return new_thread != _current;
#endif /* CONFIG_SMP */
}

static void reschedule(struct k_spinlock *lock, k_spinlock_key_t key)
{
	if (resched(key.key) && need_swap()) {
		z_swap(lock, key);
	} else {
		signal_pending_ipi();
		k_spin_unlock(lock, key);
	}
}

void z_sched_lock_reschedule(k_spinlock_key_t key)
{
	update_cache(0);
	reschedule(&_sched_spinlock, key);
}

void z_sched_yield(void)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	runq_yield();
	update_cache(1);
	z_swap(&_sched_spinlock, key);
}

/* _sched_spinlock must be held */
static void add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q)
{
	unready_thread(thread);
	z_mark_thread_as_pending(thread);

	SYS_PORT_TRACING_FUNC(k_thread, sched_pend, thread);

	if (wait_q != NULL) {
		thread->base.pended_on = wait_q;
		_priq_wait_add(&wait_q->waitq, thread);
	}
}

void z_sched_add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q)
{
	add_to_waitq_locked(thread, wait_q);
}

static void add_thread_timeout(struct k_thread *thread, k_timeout_t timeout)
{
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		z_add_thread_timeout(thread, timeout);
	}
}

static void pend_locked(struct k_thread *thread, _wait_q_t *wait_q,
			k_timeout_t timeout)
{
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(wait_q == NULL || sys_cache_is_mem_coherent(wait_q));
#endif /* CONFIG_KERNEL_COHERENCE */
	add_to_waitq_locked(thread, wait_q);
	add_thread_timeout(thread, timeout);
}

void z_pend_thread(struct k_thread *thread, _wait_q_t *wait_q,
		   k_timeout_t timeout)
{
	__ASSERT_NO_MSG(thread == _current || is_thread_dummy(thread));
	K_SPINLOCK(&_sched_spinlock) {
		pend_locked(thread, wait_q, timeout);
	}
}

void z_unpend_thread_no_timeout(struct k_thread *thread)
{
	K_SPINLOCK(&_sched_spinlock) {
		if (thread->base.pended_on != NULL) {
			unpend_thread_no_timeout(thread);
		}
	}
}

void z_sched_wake_thread_locked(struct k_thread *thread)
{
	/* No K_SPINLOCK: caller must hold _sched_spinlock when calling */
	bool killed = (thread->base.thread_state &
			(_THREAD_DEAD | _THREAD_ABORTING));

	if (!killed) {
		/* The thread is not being killed */
		if (thread->base.pended_on != NULL) {
			unpend_thread_no_timeout(thread);
		}
		z_mark_thread_as_not_sleeping(thread);
		ready_thread(thread);
	}
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
/* Timeout handler for *_thread_timeout() APIs */
void z_thread_timeout(struct _timeout *timeout)
{
	struct k_thread *thread = CONTAINER_OF(timeout,
					       struct k_thread, base.timeout);

	K_SPINLOCK(&_sched_spinlock) {
		z_sched_wake_thread_locked(thread);
	}
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */

int z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, k_timeout_t timeout)
{
#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif /* CONFIG_TIMESLICING && CONFIG_SWAP_NONATOMIC */
	__ASSERT_NO_MSG(sizeof(_sched_spinlock) == 0 || lock != &_sched_spinlock);

	/* We do a "lock swap" prior to calling z_swap(), such that
	 * the caller's lock gets released as desired.  But we ensure
	 * that we hold the scheduler lock and leave local interrupts
	 * masked until we reach the context switch.  z_swap() itself
	 * has similar code; the duplication is because it's a legacy
	 * API that doesn't expect to be called with scheduler lock
	 * held.
	 */
	(void) k_spin_lock(&_sched_spinlock);
	pend_locked(_current, wait_q, timeout);
	k_spin_release(lock);
	return z_swap(&_sched_spinlock, key);
}

struct k_thread *z_unpend1_no_timeout(_wait_q_t *wait_q)
{
	struct k_thread *thread = NULL;

	K_SPINLOCK(&_sched_spinlock) {
		thread = _priq_wait_best(&wait_q->waitq);

		if (thread != NULL) {
			unpend_thread_no_timeout(thread);
		}
	}

	return thread;
}

void z_unpend_thread(struct k_thread *thread)
{
	z_unpend_thread_no_timeout(thread);
	z_abort_thread_timeout(thread);
}

/* Priority set utility that does no rescheduling, it just changes the
 * run queue state, returning true if a reschedule is needed later.
 */
bool z_thread_prio_set(struct k_thread *thread, int prio)
{
	bool need_sched = false;
	int old_prio = thread->base.prio;

	K_SPINLOCK(&_sched_spinlock) {
		need_sched = z_is_thread_ready(thread);

		if (need_sched) {
			if (!IS_ENABLED(CONFIG_SMP) || z_is_thread_queued(thread)) {
				dequeue_thread(thread);
				thread->base.prio = prio;
				queue_thread(thread);

				if (old_prio > prio) {
					flag_ipi(ipi_mask_create(thread));
				}
			} else {
				/*
				 * This is a running thread on SMP. Update its
				 * priority, but do not requeue it. An IPI is
				 * needed if the priority is both being lowered
				 * and it is running on another CPU.
				 */

				thread->base.prio = prio;

				struct _cpu *cpu;

				cpu = thread_active_elsewhere(thread);
				if ((cpu != NULL) && (old_prio < prio)) {
					flag_ipi(IPI_CPU_MASK(cpu->id));
				}
			}

			update_cache(1);
		} else if (z_is_thread_pending(thread)) {
			/* Thread is pending, remove it from the waitq
			 * and reinsert it with the new priority to avoid
			 * violating waitq ordering and rb assumptions.
			 */
			_wait_q_t *wait_q = pended_on_thread(thread);

			_priq_wait_remove(&wait_q->waitq, thread);
			thread->base.prio = prio;
			_priq_wait_add(&wait_q->waitq, thread);
		} else {
			thread->base.prio = prio;
		}
	}

	SYS_PORT_TRACING_OBJ_FUNC(k_thread, sched_priority_set, thread, prio);

	return need_sched;
}

void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key)
{
	reschedule(lock, key);
}

void z_reschedule_irqlock(uint32_t key)
{
	if (resched(key) && need_swap()) {
		z_swap_irqlock(key);
	} else {
		/* TODO: We only hold the IRQ lock here, not _sched_spinlock,
		 * violating the locking requirement documented in
		 * signal_pending_ipi(). This can result in added delayed
		 * rescheduling.
		 */
		signal_pending_ipi();
		irq_unlock(key);
	}
}

struct k_thread *z_swap_next_thread(void)
{
	struct k_thread *ret = next_up();

	if (ret == _current) {
		/* When not swapping, have to signal IPIs here.  In
		 * the context switch case it must happen later, after
		 * _current gets requeued.
		 */
		signal_pending_ipi();
	}
	return ret;
}

#ifdef CONFIG_USE_SWITCH
/* Just a wrapper around z_current_thread_set(xxx) with tracing */
static inline void set_current(struct k_thread *new_thread)
{
	/* If the new thread is the same as the current thread, we
	 * don't need to do anything.
	 */
	if (IS_ENABLED(CONFIG_INSTRUMENT_THREAD_SWITCHING) && new_thread != _current) {
		z_thread_mark_switched_out();
	}
	z_current_thread_set(new_thread);
}

/**
 * @brief Determine next thread to execute upon completion of an interrupt
 *
 * Thread preemption is performed by context switching after the completion
 * of a non-recursed interrupt. This function determines which thread to
 * switch to if any. This function accepts as @p interrupted either:
 *
 * - The handle for the interrupted thread in which case the thread's context
 *   must already be fully saved and ready to be picked up by a different CPU.
 *
 * - NULL if more work is required to fully save the thread's state after
 *   it is known that a new thread is to be scheduled. It is up to the caller
 *   to store the handle resulting from the thread that is being switched out
 *   in that thread's "switch_handle" field after its
 *   context has fully been saved, following the same requirements as with
 *   the @ref arch_switch() function.
 *
 * If a new thread needs to be scheduled then its handle is returned.
 * Otherwise the same value provided as @p interrupted is returned back.
 * Those handles are the same opaque types used by the @ref arch_switch()
 * function.
 *
 * @warning
 * The _current value may have changed after this call and not refer
 * to the interrupted thread anymore. It might be necessary to make a local
 * copy before calling this function.
 *
 * @param interrupted Handle for the thread that was interrupted or NULL.
 * @return Handle for the next thread to execute, or @p interrupted when
 *         no new thread is to be scheduled.
 */
void *z_get_next_switch_handle(void *interrupted)
{
	z_check_stack_sentinel();

#ifdef CONFIG_SMP
	void *ret = NULL;

	K_SPINLOCK(&_sched_spinlock) {
		struct k_thread *old_thread = _current, *new_thread;

		__ASSERT(old_thread->switch_handle == NULL || is_thread_dummy(old_thread),
			"old thread handle should be null.");

		new_thread = next_up();

		z_sched_usage_switch(new_thread);

		if (old_thread != new_thread) {
			uint8_t  cpu_id;

			z_sched_switch_spin(new_thread);
			arch_cohere_stacks(old_thread, interrupted, new_thread);

			_current_cpu->swap_ok = 0;
			cpu_id = arch_curr_cpu()->id;
			new_thread->base.cpu = cpu_id;
			set_current(new_thread);

#ifdef CONFIG_TIMESLICING
			z_time_slice_reset(new_thread);
#endif /* CONFIG_TIMESLICING */

#ifdef CONFIG_SPIN_VALIDATE
			/* Changed _current!  Update the spinlock
			 * bookkeeping so the validation doesn't get
			 * confused when the "wrong" thread tries to
			 * release the lock.
			 */
			z_spin_lock_set_owner(&_sched_spinlock);
#endif /* CONFIG_SPIN_VALIDATE */

			/* A queued (runnable) old/current thread
			 * needs to be added back to the run queue
			 * here, and atomically with its switch handle
			 * being set below.  This is safe now, as we
			 * will not return into it.
			 */
			if (z_is_thread_queued(old_thread)) {
#ifdef CONFIG_SCHED_IPI_CASCADE
				if ((new_thread->base.cpu_mask != -1) &&
				    (old_thread->base.cpu_mask != BIT(cpu_id))) {
					flag_ipi(ipi_mask_create(old_thread));
				}
#endif
				runq_add(old_thread);
			}
		}
		old_thread->switch_handle = interrupted;
		ret = new_thread->switch_handle;
		/* Active threads MUST have a null here */
		new_thread->switch_handle = NULL;

		/* Check for IPIs under the lock to avoid silently consuming a
		 * rescheduling IPI flagged by another CPU for ourselves.
		 */
		signal_pending_ipi();
	}
	return ret;
#else
	z_sched_usage_switch(_kernel.ready_q.cache);
	_current->switch_handle = interrupted;
	set_current(_kernel.ready_q.cache);
	return _current->switch_handle;
#endif /* CONFIG_SMP */
}
#endif /* CONFIG_USE_SWITCH */

int z_unpend_all_locked(_wait_q_t *wait_q)
{
	int need_sched = 0;
	struct k_thread *thread;

	__ASSERT(z_spin_is_locked(&_sched_spinlock), "sched lock not held");

	for (thread = z_waitq_head(wait_q); thread != NULL; thread = z_waitq_head(wait_q)) {
		unpend_thread_no_timeout(thread);
		z_abort_thread_timeout(thread);
		ready_thread(thread);
		need_sched = 1;
	}

	return need_sched;
}

int z_unpend_all(_wait_q_t *wait_q)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);
	int need_sched = z_unpend_all_locked(wait_q);

	k_spin_unlock(&_sched_spinlock, key);
	return need_sched;
}

static inline void unpend_all(_wait_q_t *wait_q)
{
	struct k_thread *thread;

	for (thread = z_waitq_head(wait_q); thread != NULL; thread = z_waitq_head(wait_q)) {
		unpend_thread_no_timeout(thread);
		z_abort_thread_timeout(thread);
		arch_thread_return_value_set(thread, 0);
		ready_thread(thread);
	}
}

#ifdef CONFIG_THREAD_ABORT_HOOK
extern void thread_abort_hook(struct k_thread *thread);
#endif /* CONFIG_THREAD_ABORT_HOOK */

/**
 * @brief Dequeues the specified thread
 *
 * Dequeues the specified thread and move it into the specified new state.
 *
 * @param thread Identify the thread to halt
 * @param new_state New thread state (_THREAD_DEAD or _THREAD_SUSPENDED)
 */
static ALWAYS_INLINE void halt_thread(struct k_thread *thread, uint8_t new_state)
{
	bool dummify = false;

	/* We hold the lock, and the thread is known not to be running
	 * anywhere.
	 */
	if ((thread->base.thread_state & new_state) == 0U) {
		thread->base.thread_state |= new_state;
		if (z_is_thread_queued(thread)) {
			dequeue_thread(thread);
		}

		if (new_state == _THREAD_DEAD) {
			if (thread->base.pended_on != NULL) {
				unpend_thread_no_timeout(thread);
			}
			z_abort_thread_timeout(thread);
			unpend_all(&thread->join_queue);

			/* Edge case: aborting _current from within an
			 * ISR that preempted it requires clearing the
			 * _current pointer so the upcoming context
			 * switch doesn't clobber the now-freed
			 * memory
			 */
			if (thread == _current && arch_is_in_isr()) {
				dummify = true;
			}
		}
#ifdef CONFIG_SMP
		unpend_all(&thread->halt_queue);
#endif /* CONFIG_SMP */
		update_cache(1);

		if (new_state == _THREAD_SUSPENDED) {
			clear_halting(thread);
			return;
		}

		arch_coprocessors_disable(thread);

		SYS_PORT_TRACING_FUNC(k_thread, sched_abort, thread);

		z_thread_monitor_exit(thread);
#ifdef CONFIG_THREAD_ABORT_HOOK
		thread_abort_hook(thread);
#endif /* CONFIG_THREAD_ABORT_HOOK */

#ifdef CONFIG_OBJ_CORE_THREAD
#ifdef CONFIG_OBJ_CORE_STATS_THREAD
		k_obj_core_stats_deregister(K_OBJ_CORE(thread));
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */
		k_obj_core_unlink(K_OBJ_CORE(thread));
#endif /* CONFIG_OBJ_CORE_THREAD */

#ifdef CONFIG_USERSPACE
		z_mem_domain_exit_thread(thread);
		k_thread_perms_all_clear(thread);
		k_object_uninit(thread->stack_obj);
		k_object_uninit(thread);
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_THREAD_ABORT_NEED_CLEANUP
		k_thread_abort_cleanup(thread);
#endif /* CONFIG_THREAD_ABORT_NEED_CLEANUP */

		/* Do this "set _current to dummy" step last so that
		 * subsystems above can rely on _current being
		 * unchanged.  Disabled for posix as that arch
		 * continues to use the _current pointer in its swap
		 * code.  Note that we must leave a non-null switch
		 * handle for any threads spinning in join() (this can
		 * never be used, as our thread is flagged dead, but
		 * it must not be NULL otherwise join can deadlock).
		 * Use 1 as a clearly invalid but non-NULL value.
		 */
		if (dummify && !IS_ENABLED(CONFIG_ARCH_POSIX)) {
#ifdef CONFIG_USE_SWITCH
			_current->switch_handle = (void *)1;
#endif
			z_dummy_thread_init(&_thread_dummy);

		}

		/* Finally update the halting thread state, on which
		 * other CPUs might be spinning (see
		 * thread_halt_spin()).
		 */
		clear_halting(thread);
	}
}

void z_thread_suspend_current(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	z_mark_thread_as_suspended(thread);
	z_metairq_preempted_clear(thread);
	dequeue_thread(thread);
	update_cache(1);
	z_swap(&_sched_spinlock, key);
}
