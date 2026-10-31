/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <kspinlock.h>
#include <ksched.h>
#include <zephyr/spinlock.h>
#include <wait_q.h>
#include <kthread.h>
#include <priority_q.h>
#include <kswap.h>
#include <ipi.h>
#include <ksmp.h>
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

struct k_spinlock _sched_spinlock;  /* The scheduler's spinlock */

/* Storage to "complete" the context switch from an invalid/incomplete thread
 * context (ex: exiting an ISR that aborted _current)
 */
__incoherent struct k_thread _thread_dummy;

static ALWAYS_INLINE void update_cache(int preempt_ok);
static void halt_thread(struct k_thread *thread, uint8_t new_state, k_spinlock_key_t *key);
static void add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q);

/* Uniprocessor selection of the next thread, used to refresh the ready
 * queue cache. The SMP scheduler selects its next thread at the context
 * switch instead, in kernel/smp/sched_smp.c.
 */
static ALWAYS_INLINE struct k_thread *next_up(void)
{
	struct k_thread *thread = runq_best();

	thread = metairq_preempt_recover(thread);

	/* In uniprocessor mode, we can leave the current thread in
	 * the queue (actually we have to, otherwise the assembly
	 * context switch code for all architectures would be
	 * responsible for putting it back in z_swap and ISR return!),
	 * which makes this choice simple.
	 */
	return (thread != NULL) ? thread : _current_cpu->idle_thread;
}

void move_current_to_end_of_prio_q(void)
{
	runq_yield();

	update_cache(1);
}

static ALWAYS_INLINE void update_cache(int preempt_ok)
{
#ifdef CONFIG_SMP
	/* There is no ready queue cache on SMP: the next thread is selected
	 * at the context switch, so only record whether a cooperative swap
	 * is OK until then.
	 */
	z_smp_swap_ok_set(preempt_ok);
#else
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
#endif /* CONFIG_SMP */
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
	Z_SCHED_SPINLOCK {
		if (z_smp_thread_active_elsewhere(thread) == NULL) {
			ready_thread(thread);
		}
	}
}

void z_sched_ready_locked(struct k_thread *thread) ALIAS_OF(ready_thread);

static void unready_thread(struct k_thread *thread)
{
	if (z_is_thread_queued(thread)) {
		/* Clear idle CPU coverage before removing the thread from the run queue. */
		ipi_idle_thread_unreserve(thread);
		dequeue_thread(thread);
	}
	update_cache(thread == _current);
}

/* This routine exists for benchmarking purposes. It is not used in
 * general production code.
 */
void z_unready_thread(struct k_thread *thread)
{
	Z_SCHED_SPINLOCK {
		unready_thread(thread);
	}
}


void z_sched_unready_locked(struct k_thread *thread) ALIAS_OF(unready_thread);

/* This routine only used for testing purposes */
void z_yield_testing_only(void)
{
	Z_SCHED_SPINLOCK {
		move_current_to_end_of_prio_q();
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
	z_metairq_preempted_clear(thread);

	/* If the target is a thread running on another CPU, flag and
	 * poke (note that we might spin to wait, so a true
	 * synchronous IPI is needed here, not deferred!), it will
	 * halt itself in the IPI.  Otherwise it's unscheduled, so we
	 * can clean it up directly.
	 */

	struct _cpu *cpu = z_smp_thread_active_elsewhere(thread);

	if (cpu != NULL) {
		z_smp_halt_remote(thread, cpu, key, terminate);
	} else {
		halt_thread(thread, terminate ? _THREAD_DEAD : _THREAD_SUSPENDED, &key);
		if ((thread == _current) && !arch_is_in_isr()) {
			if (z_is_thread_essential(thread)) {
				z_sched_spinlock_unlock(key);
				k_panic();
				key = z_sched_spinlock_lock();
			}
			z_swap_locked(key);
			__ASSERT(!terminate, "aborted _current back from dead");
		} else {
			z_sched_spinlock_unlock(key);
		}
	}
	/* NOTE: the scheduler lock has been released.  Don't put
	 * logic here, it's likely to be racy/deadlocky even if you
	 * re-take the lock!
	 */
}

static inline bool resched(uint32_t key)
{
	z_smp_swap_ok_set(false);

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

/**
 * Like reschedule(), but the scheduler's spinlock is known to be the lock.
 */
static void reschedule_locked(k_spinlock_key_t key)
{
	return reschedule(&_sched_spinlock, key);
}

void z_sched_lock_reschedule(k_spinlock_key_t key)
{
	update_cache(0);
	reschedule_locked(key);
}

void z_sched_yield(void)
{
	k_spinlock_key_t key = z_sched_spinlock_lock();

	runq_yield();
	update_cache(1);
	z_swap_locked(key);
}

/* The scheduler's spinlock must be held */
static void add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q)
{
	/* A thread must not already be on a wait queue when added to a new one. */
	__ASSERT_NO_MSG(thread->base.pended_on == NULL);

	unready_thread(thread);
	z_mark_thread_as_pending(thread);

	SYS_PORT_TRACING_FUNC(k_thread, sched_pend, thread);

	if (wait_q != NULL) {
		thread->base.pended_on = wait_q;
		_priq_wait_add(&wait_q->waitq, thread);
	}
}

void z_sched_add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q)
	ALIAS_OF(add_to_waitq_locked);

static void pend_locked(struct k_thread *thread, _wait_q_t *wait_q,
			k_timeout_t timeout)
{
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(wait_q == NULL || sys_cache_is_mem_coherent(wait_q));
#endif /* CONFIG_KERNEL_COHERENCE */
	add_to_waitq_locked(thread, wait_q);
	z_add_thread_timeout(thread, timeout);
}

void z_pend_thread(struct k_thread *thread, _wait_q_t *wait_q,
		   k_timeout_t timeout)
{
	__ASSERT_NO_MSG(thread == _current || is_thread_dummy(thread));
	Z_SCHED_SPINLOCK {
		pend_locked(thread, wait_q, timeout);
	}
}

void z_unpend_thread_no_timeout(struct k_thread *thread)
{
	Z_SCHED_SPINLOCK {
		if (thread->base.pended_on != NULL) {
			unpend_thread_no_timeout(thread);
		}
	}
}

void z_sched_wake_thread_locked(struct k_thread *thread)
{
	/* No K_SPINLOCK: caller must hold the scheduler's spinlock when calling */
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

	Z_SCHED_SPINLOCK {
		/* A concurrent waker (e.g. a sem give on another CPU) may
		 * have unpended and readied the thread, after which the
		 * thread could run and re-pend elsewhere -- possibly with no
		 * timeout. Such a waker aborts this timeout, flagging it
		 * superseded; bail so we don't wake the thread from its new
		 * wait.
		 */
		if (!z_timeout_inflight_superseded(timeout)) {
			z_sched_wake_thread_locked(thread);
		}
	}
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */

int z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, k_timeout_t timeout)
{
	/* A blocking pend from ISR context is a programming error with no
	 * safe recovery: it would sleep whatever thread was interrupted and,
	 * on CONFIG_SWAP_NONATOMIC, corrupt the scheduler. Refuse it fatally
	 * in every build rather than degrading into the corruption traced in
	 * #111518. Placed first so the refusal performs no side effect.
	 */
	if (arch_is_in_isr()) {
		__ASSERT(false, "blocking pend from ISR context");
		k_panic();
	}

#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif /* CONFIG_TIMESLICING && CONFIG_SWAP_NONATOMIC */
	__ASSERT_NO_MSG((sizeof(struct k_spinlock) == 0) || !z_is_sched_spinlock(lock));

	/* We do a "lock swap" prior to calling z_swap(), such that
	 * the caller's lock gets released as desired.  But we ensure
	 * that we hold the scheduler lock and leave local interrupts
	 * masked until we reach the context switch.  z_swap() itself
	 * has similar code; the duplication is because it's a legacy
	 * API that doesn't expect to be called with scheduler lock
	 * held.
	 */
	(void) z_sched_spinlock_lock();
	pend_locked(_current, wait_q, timeout);
	k_spin_release(lock);
	return z_swap_locked(key);
}

struct k_thread *z_unpend1_no_timeout(_wait_q_t *wait_q)
{
	struct k_thread *thread = NULL;

	Z_SCHED_SPINLOCK {
		thread = _priq_wait_best(&wait_q->waitq);

		if (thread != NULL) {
			unpend_thread_no_timeout(thread);
		}
	}

	return thread;
}

void z_unpend_thread(struct k_thread *thread)
{
	k_spinlock_key_t key = z_sched_spinlock_lock();

	if (thread->base.pended_on != NULL) {
		unpend_thread_no_timeout(thread);
	}
	while (z_try_abort_thread_timeout(thread) == -EAGAIN) {
		z_sched_spinlock_unlock(key);
		key = z_sched_spinlock_lock();
	}
	z_sched_spinlock_unlock(key);
}

/* Priority set utility that does no rescheduling, it just changes the
 * run queue state, returning true if a reschedule is needed later.
 */
bool z_thread_prio_set(struct k_thread *thread, int prio)
{
	bool need_sched = false;
	int old_prio = thread->base.prio;

	Z_SCHED_SPINLOCK {
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

				cpu = z_smp_thread_active_elsewhere(thread);
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

void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key) ALIAS_OF(reschedule);

void z_reschedule_locked(k_spinlock_key_t key) ALIAS_OF(reschedule_locked);

void z_reschedule_irqlock(uint32_t key)
{
	if (resched(key) && need_swap()) {
		z_swap_irqlock(key);
	} else {
		signal_pending_ipi();
		irq_unlock(key);
	}
}

#if defined(CONFIG_USE_SWITCH) && !defined(CONFIG_SMP)
void *z_get_next_switch_handle(void *interrupted)
{
	z_check_stack_sentinel();

	struct k_thread *next = _kernel.ready_q.cache;

	z_sched_usage_switch(next);
	_current->switch_handle = interrupted;
	z_sched_set_current(next);
	return next->switch_handle;
}
#endif /* CONFIG_USE_SWITCH && !CONFIG_SMP */

int z_unpend_all(_wait_q_t *wait_q)
{
	int need_sched = 0;
	struct k_thread *thread;

	Z_SCHED_SPINLOCK {
		for (thread = z_waitq_head_locked(wait_q);
		     thread != NULL;
		     thread = z_waitq_head_locked(wait_q)) {
			unpend_thread_no_timeout(thread);
			/* Abort the timeout and ready the thread unconditionally. If
			 * the timeout handler is in flight on another CPU, the abort
			 * flags it superseded and z_thread_timeout() bails when it
			 * runs -- so it will NOT ready the thread, we must.
			 */
			(void)z_try_abort_thread_timeout(thread);
			ready_thread(thread);
			need_sched = 1;
		}
	}

	return need_sched;
}

static inline void unpend_all(_wait_q_t *wait_q)
{
	struct k_thread *thread;

	for (thread = z_waitq_head_locked(wait_q);
	     thread != NULL;
	     thread = z_waitq_head_locked(wait_q)) {
		unpend_thread_no_timeout(thread);
		arch_thread_return_value_set(thread, 0);
		/* See z_unpend_all(): the in-flight handler bails on the
		 * superseded mark, so we ready the thread unconditionally.
		 */
		(void)z_try_abort_thread_timeout(thread);
		ready_thread(thread);
	}
}

void z_sched_unpend_all_locked(_wait_q_t *wait_q) ALIAS_OF(unpend_all);

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
 * @param key Pointer to the scheduler spinlock key held by the caller
 */
static void halt_thread(struct k_thread *thread, uint8_t new_state, k_spinlock_key_t *key)
{
	bool dummify = false;

	/* We hold the lock, and the thread is known not to be running
	 * anywhere.
	 */
	if ((thread->base.thread_state & new_state) == 0U) {
		thread->base.thread_state |= new_state;
		if (z_is_thread_queued(thread)) {
			/* Clear idle CPU coverage before removing the thread from the run queue. */
			ipi_idle_thread_unreserve(thread);
			dequeue_thread(thread);
		}

		if (new_state == _THREAD_DEAD) {
			if (thread->base.pended_on != NULL) {
				unpend_thread_no_timeout(thread);
			}
			/* Wait for any in-flight handler to complete before
			 * we proceed: the caller may free this thread's
			 * storage (dynamic threads), and an in-flight handler
			 * would UAF when it eventually runs z_thread_timeout()
			 * and dereferences thread->base. _THREAD_DEAD is
			 * already set, so once the handler runs it bails via
			 * z_sched_wake_thread_locked()'s killed check.
			 *
			 * NULL key is the next_up() self-halt path on a
			 * running _current that has no linked timeout; there
			 * is nothing to do here. That path is always reached
			 * via z_thread_halt()'s IF branch, which spins on
			 * z_try_abort_thread_timeout(thread) outside any lock
			 * after the halt-queue wait completes, closing any
			 * remaining in-flight window before the caller of
			 * z_thread_halt() returns.
			 */
			if (key != NULL) {
				while (z_try_abort_thread_timeout(thread) == -EAGAIN) {
					z_sched_spinlock_unlock(*key);
					*key = z_sched_spinlock_lock();
				}
			}
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
		z_smp_halt_queue_wake(thread);
		update_cache(1);

		if (new_state == _THREAD_SUSPENDED) {
			z_smp_clear_halting(thread);
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
#ifdef CONFIG_SPIN_VALIDATE
			/* On arches where exceptions run as ISRs (e.g. Xtensa),
			 * the dying thread's lock tracking is never cleared via the
			 * normal abort path.  Reset it here before _thread_dummy
			 * takes over.  Sentinel-gated so genuine bugs still assert.
			 */
			if (thread->base.swap_data ==
			    (void *)&z_spinlock_abort_sentinel) {
				z_spin_validate_reset(true);
			}
#endif
			z_dummy_thread_init(&_thread_dummy);

		}

		/* Finally update the halting thread state, on which
		 * other CPUs might be spinning (see
		 * thread_halt_spin()).
		 */
		z_smp_clear_halting(thread);
	}
}

void z_sched_halt_thread_locked(struct k_thread *thread, uint8_t new_state, k_spinlock_key_t *key)
	ALIAS_OF(halt_thread);

void z_thread_suspend_current(struct k_thread *thread)
{
	k_spinlock_key_t key = z_sched_spinlock_lock();

	z_mark_thread_as_suspended(thread);
	z_metairq_preempted_clear(thread);
	dequeue_thread(thread);
	update_cache(1);
	z_swap_locked(key);
}
