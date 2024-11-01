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

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_TIMESLICING)
extern struct k_thread *pending_current;
#endif

struct k_spinlock _sched_spinlock;

/* Storage to "complete" the context switch from an invalid/incomplete thread
 * context (ex: exiting an ISR that aborted _current)
 */
__incoherent struct k_thread _thread_dummy;

static ALWAYS_INLINE void update_cache(int preempt_ok);
static void halt_thread(struct k_thread *thread, uint8_t new_state);
static void add_to_waitq_locked(struct k_thread *thread, _wait_q_t *wait_q);


BUILD_ASSERT(CONFIG_NUM_COOP_PRIORITIES >= CONFIG_NUM_METAIRQ_PRIORITIES,
	     "You need to provide at least as many CONFIG_NUM_COOP_PRIORITIES as "
	     "CONFIG_NUM_METAIRQ_PRIORITIES as Meta IRQs are just a special class of cooperative "
	     "threads.");

/*
 * Return value same as e.g. memcmp
 * > 0 -> thread 1 priority  > thread 2 priority
 * = 0 -> thread 1 priority == thread 2 priority
 * < 0 -> thread 1 priority  < thread 2 priority
 * Do not rely on the actual value returned aside from the above.
 * (Again, like memcmp.)
 */
int32_t z_sched_prio_cmp(struct k_thread *thread_1,
	struct k_thread *thread_2)
{
	/* `prio` is <32b, so the below cannot overflow. */
	int32_t b1 = thread_1->base.prio;
	int32_t b2 = thread_2->base.prio;

	if (b1 != b2) {
		return b2 - b1;
	}

#ifdef CONFIG_SCHED_DEADLINE
	/* If we assume all deadlines live within the same "half" of
	 * the 32 bit modulus space (this is a documented API rule),
	 * then the latest deadline in the queue minus the earliest is
	 * guaranteed to be (2's complement) non-negative.  We can
	 * leverage that to compare the values without having to check
	 * the current time.
	 */
	uint32_t d1 = thread_1->base.prio_deadline;
	uint32_t d2 = thread_2->base.prio_deadline;

	if (d1 != d2) {
		/* Sooner deadline means higher effective priority.
		 * Doing the calculation with unsigned types and casting
		 * to signed isn't perfect, but at least reduces this
		 * from UB on overflow to impdef.
		 */
		return (int32_t) (d2 - d1);
	}
#endif /* CONFIG_SCHED_DEADLINE */
	return 0;
}

static ALWAYS_INLINE void *thread_runq(struct k_thread *thread)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	int cpu, m = thread->base.cpu_mask;

	/* Edge case: it's legal per the API to "make runnable" a
	 * thread with all CPUs masked off (i.e. one that isn't
	 * actually runnable!).  Sort of a wart in the API and maybe
	 * we should address this in docs/assertions instead to avoid
	 * the extra test.
	 */
	cpu = m == 0 ? 0 : u32_count_trailing_zeros(m);

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

	_priq_run_add(thread_runq(thread), thread);
}

static ALWAYS_INLINE void runq_remove(struct k_thread *thread)
{
	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));

	_priq_run_remove(thread_runq(thread), thread);
}

static ALWAYS_INLINE struct k_thread *runq_best(void)
{
	return _priq_run_best(curr_cpu_runq());
}

/* _current is never in the run queue until context switch on
 * SMP configurations, see z_requeue_current()
 */
static inline bool should_queue_thread(struct k_thread *thread)
{
	return !IS_ENABLED(CONFIG_SMP) || (thread != _current);
}

static ALWAYS_INLINE void queue_thread(struct k_thread *thread)
{
	thread->base.thread_state |= _THREAD_QUEUED;
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
	thread->base.thread_state &= ~_THREAD_QUEUED;
	if (should_queue_thread(thread)) {
		runq_remove(thread);
	}
}

/* Called out of z_swap() when CONFIG_SMP.  The current thread can
 * never live in the run queue until we are inexorably on the context
 * switch path on SMP, otherwise there is a deadlock condition where a
 * set of CPUs pick a cycle of threads to run and wait for them all to
 * context switch forever.
 */
void z_requeue_current(struct k_thread *thread)
{
	if (z_is_thread_queued(thread)) {
		runq_add(thread);
	}
	signal_pending_ipi();
}

/* Return true if the thread is aborting, else false */
static inline bool is_aborting(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_ABORTING) != 0U;
}

/* Return true if the thread is aborting or suspending, else false */
static inline bool is_halting(struct k_thread *thread)
{
	return (thread->base.thread_state &
		(_THREAD_ABORTING | _THREAD_SUSPENDING)) != 0U;
}

/* Clear the halting bits (_THREAD_ABORTING and _THREAD_SUSPENDING) */
static inline void clear_halting(struct k_thread *thread)
{
	barrier_dmem_fence_full(); /* Other cpus spin on this locklessly! */
	thread->base.thread_state &= ~(_THREAD_ABORTING | _THREAD_SUSPENDING);
}

static ALWAYS_INLINE struct k_thread *next_up(void)
{
#ifdef CONFIG_SMP
	if (is_halting(_current)) {
		halt_thread(_current, is_aborting(_current) ?
				      _THREAD_DEAD : _THREAD_SUSPENDED);
	}
#endif /* CONFIG_SMP */

	struct k_thread *thread = runq_best();

#if (CONFIG_NUM_METAIRQ_PRIORITIES > 0) &&                                                         \
	(CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES)
	/* MetaIRQs must always attempt to return back to a
	 * cooperative thread they preempted and not whatever happens
	 * to be highest priority now. The cooperative thread was
	 * promised it wouldn't be preempted (by non-metairq threads)!
	 */
	struct k_thread *mirqp = _current_cpu->metairq_preempted;

	if (mirqp != NULL && (thread == NULL || !thread_is_metairq(thread))) {
		if (!z_is_thread_prevented_from_running(mirqp)) {
			thread = mirqp;
		} else {
			_current_cpu->metairq_preempted = NULL;
		}
	}
#endif
/* CONFIG_NUM_METAIRQ_PRIORITIES > 0 &&
 * CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES
 */

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
	 * Subtle note on "queued": in SMP mode, _current does not
	 * live in the queue, so this isn't exactly the same thing as
	 * "ready", it means "is _current already added back to the
	 * queue such that we don't want to re-add it".
	 */
	bool queued = z_is_thread_queued(_current);
	bool active = !z_is_thread_prevented_from_running(_current);

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

	/* Put _current back into the queue */
	if ((thread != _current) && active &&
		!z_is_idle_thread_object(_current) && !queued) {
		queue_thread(_current);
	}

	/* Take the new _current out of the queue */
	if (z_is_thread_queued(thread)) {
		dequeue_thread(thread);
	}

	_current_cpu->swap_ok = false;
	return thread;
#endif /* CONFIG_SMP */
}

void move_thread_to_end_of_prio_q(struct k_thread *thread)
{
	if (z_is_thread_queued(thread)) {
		dequeue_thread(thread);
	}
	queue_thread(thread);
	update_cache(thread == _current);
}

/* Track cooperative threads preempted by metairqs so we can return to
 * them specifically.  Called at the moment a new thread has been
 * selected to run.
 */
static void update_metairq_preempt(struct k_thread *thread)
{
#if (CONFIG_NUM_METAIRQ_PRIORITIES > 0) &&                                                         \
	(CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES)
	if (thread_is_metairq(thread) && !thread_is_metairq(_current) &&
	    !thread_is_preemptible(_current)) {
		/* Record new preemption */
		_current_cpu->metairq_preempted = _current;
	} else if (!thread_is_metairq(thread) && !z_is_idle_thread_object(thread)) {
		/* Returning from existing preemption */
		_current_cpu->metairq_preempted = NULL;
	}
#else
	ARG_UNUSED(thread);
#endif
/* CONFIG_NUM_METAIRQ_PRIORITIES > 0 &&
 * CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES
 */
}

static ALWAYS_INLINE void update_cache(int preempt_ok)
{
#ifndef CONFIG_SMP
	struct k_thread *thread = next_up();

	if (should_preempt(thread, preempt_ok)) {
#ifdef CONFIG_TIMESLICING
		if (thread != _current) {
			z_reset_time_slice(thread);
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

static struct _cpu *thread_active_elsewhere(struct k_thread *thread)
{
	/* Returns pointer to _cpu if the thread is currently running on
	 * another CPU. There are more scalable designs to answer this
	 * question in constant time, but this is fine for now.
	 */
#ifdef CONFIG_SMP
	int currcpu = _current_cpu->id;

	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		if ((i != currcpu) &&
		    (_kernel.cpus[i].current == thread)) {
			return &_kernel.cpus[i];
		}
	}
#endif /* CONFIG_SMP */
	ARG_UNUSED(thread);
	return NULL;
}

static void ready_thread(struct k_thread *thread)
{
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(thread));
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

void z_move_thread_to_end_of_prio_q(struct k_thread *thread)
{
	K_SPINLOCK(&_sched_spinlock) {
		move_thread_to_end_of_prio_q(thread);
	}
}

void z_sched_start(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	if (z_has_thread_started(thread)) {
		k_spin_unlock(&_sched_spinlock, key);
		return;
	}

	z_mark_thread_as_started(thread);
	ready_thread(thread);
	z_reschedule(&_sched_spinlock, key);
}

/* Spins in ISR context, waiting for a thread known to be running on
 * another CPU to catch the IPI we sent and halt.  Note that we check
 * for ourselves being asynchronously halted first to prevent simple
 * deadlocks (but not complex ones involving cycles of 3+ threads!).
 * Acts to release the provided lock before returning.
 */
static void thread_halt_spin(struct k_thread *thread, k_spinlock_key_t key)
{
	if (is_halting(_current)) {
		halt_thread(_current,
			    is_aborting(_current) ? _THREAD_DEAD : _THREAD_SUSPENDED);
	}
	k_spin_unlock(&_sched_spinlock, key);
	while (is_halting(thread)) {
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
static void z_thread_halt(struct k_thread *thread, k_spinlock_key_t key,
			  bool terminate)
{
	_wait_q_t *wq = &thread->join_queue;
#ifdef CONFIG_SMP
	wq = terminate ? wq : &thread->halt_queue;
#endif

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
#endif
#endif
		if (arch_is_in_isr()) {
			thread_halt_spin(thread, key);
		} else  {
			add_to_waitq_locked(_current, wq);
			z_swap(&_sched_spinlock, key);
		}
	} else {
		halt_thread(thread, terminate ? _THREAD_DEAD : _THREAD_SUSPENDED);
		if ((thread == _current) && !arch_is_in_isr()) {
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


void z_impl_k_thread_suspend(k_tid_t thread)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, suspend, thread);

	(void)z_abort_thread_timeout(thread);

	k_spinlock_key_t  key = k_spin_lock(&_sched_spinlock);

	if ((thread->base.thread_state & _THREAD_SUSPENDED) != 0U) {

		/* The target thread is already suspended. Nothing to do. */

		k_spin_unlock(&_sched_spinlock, key);
		return;
	}

	z_thread_halt(thread, key, false);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, suspend, thread);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_suspend(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_suspend(thread);
}
#include <zephyr/syscalls/k_thread_suspend_mrsh.c>
#endif /* CONFIG_USERSPACE */

void z_impl_k_thread_resume(k_tid_t thread)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, resume, thread);

	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	/* Do not try to resume a thread that was not suspended */
	if (!z_is_thread_suspended(thread)) {
		k_spin_unlock(&_sched_spinlock, key);
		return;
	}

	z_mark_thread_as_not_suspended(thread);
	ready_thread(thread);

	z_reschedule(&_sched_spinlock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, resume, thread);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_resume(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_thread_resume(thread);
}
#include <zephyr/syscalls/k_thread_resume_mrsh.c>
#endif /* CONFIG_USERSPACE */

static void unready_thread(struct k_thread *thread)
{
	if (z_is_thread_queued(thread)) {
		dequeue_thread(thread);
	}
	update_cache(thread == _current);
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
	__ASSERT_NO_MSG(wait_q == NULL || arch_mem_coherent(wait_q));
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

void z_sched_wake_thread(struct k_thread *thread, bool is_timeout)
{
	K_SPINLOCK(&_sched_spinlock) {
		bool killed = (thread->base.thread_state &
				(_THREAD_DEAD | _THREAD_ABORTING));

#ifdef CONFIG_EVENTS
		bool do_nothing = thread->no_wake_on_timeout && is_timeout;

		thread->no_wake_on_timeout = false;

		if (do_nothing) {
			continue;
		}
#endif /* CONFIG_EVENTS */

		if (!killed) {
			/* The thread is not being killed */
			if (thread->base.pended_on != NULL) {
				unpend_thread_no_timeout(thread);
			}
			z_mark_thread_as_started(thread);
			if (is_timeout) {
				z_mark_thread_as_not_suspended(thread);
			}
			ready_thread(thread);
		}
	}

}

#ifdef CONFIG_SYS_CLOCK_EXISTS
/* Timeout handler for *_thread_timeout() APIs */
void z_thread_timeout(struct _timeout *timeout)
{
	struct k_thread *thread = CONTAINER_OF(timeout,
					       struct k_thread, base.timeout);

	z_sched_wake_thread(thread, true);
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
	(void)z_abort_thread_timeout(thread);
}

/* Priority set utility that does no rescheduling, it just changes the
 * run queue state, returning true if a reschedule is needed later.
 */
bool z_thread_prio_set(struct k_thread *thread, int prio)
{
	bool need_sched = 0;
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
		} else {
			thread->base.prio = prio;
		}
	}

	SYS_PORT_TRACING_OBJ_FUNC(k_thread, sched_priority_set, thread, prio);

	return need_sched;
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

void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key)
{
	if (resched(key.key) && need_swap()) {
		z_swap(lock, key);
	} else {
		k_spin_unlock(lock, key);
		signal_pending_ipi();
	}
}

void z_reschedule_irqlock(uint32_t key)
{
	if (resched(key) && need_swap()) {
		z_swap_irqlock(key);
	} else {
		irq_unlock(key);
		signal_pending_ipi();
	}
}

void k_sched_lock(void)
{
	K_SPINLOCK(&_sched_spinlock) {
		SYS_PORT_TRACING_FUNC(k_thread, sched_lock);

		z_sched_lock();
	}
}

void k_sched_unlock(void)
{
	K_SPINLOCK(&_sched_spinlock) {
		__ASSERT(_current->base.sched_locked != 0U, "");
		__ASSERT(!arch_is_in_isr(), "");

		++_current->base.sched_locked;
		update_cache(0);
	}

	LOG_DBG("scheduler unlocked (%p:%d)",
		_current, _current->base.sched_locked);

	SYS_PORT_TRACING_FUNC(k_thread, sched_unlock);

	z_reschedule_unlocked();
}

struct k_thread *z_swap_next_thread(void)
{
#ifdef CONFIG_SMP
	struct k_thread *ret = next_up();

	if (ret == _current) {
		/* When not swapping, have to signal IPIs here.  In
		 * the context switch case it must happen later, after
		 * _current gets requeued.
		 */
		signal_pending_ipi();
	}
	return ret;
#else
	return _kernel.ready_q.cache;
#endif /* CONFIG_SMP */
}

#ifdef CONFIG_USE_SWITCH
/* Just a wrapper around arch_current_thread_set(xxx) with tracing */
static inline void set_current(struct k_thread *new_thread)
{
	z_thread_mark_switched_out();
	arch_current_thread_set(new_thread);
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
 * @retval Handle for the next thread to execute, or @p interrupted when
 *         no new thread is to be scheduled.
 */
void *z_get_next_switch_handle(void *interrupted)
{
	z_check_stack_sentinel();

#ifdef CONFIG_SMP
	void *ret = NULL;

	K_SPINLOCK(&_sched_spinlock) {
		struct k_thread *old_thread = _current, *new_thread;

		if (IS_ENABLED(CONFIG_SMP)) {
			old_thread->switch_handle = NULL;
		}
		new_thread = next_up();

		z_sched_usage_switch(new_thread);

		if (old_thread != new_thread) {
			uint8_t  cpu_id;

			update_metairq_preempt(new_thread);
			z_sched_switch_spin(new_thread);
			arch_cohere_stacks(old_thread, interrupted, new_thread);

			_current_cpu->swap_ok = 0;
			cpu_id = arch_curr_cpu()->id;
			new_thread->base.cpu = cpu_id;
			set_current(new_thread);

#ifdef CONFIG_TIMESLICING
			z_reset_time_slice(new_thread);
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
		if (IS_ENABLED(CONFIG_SMP)) {
			/* Active threads MUST have a null here */
			new_thread->switch_handle = NULL;
		}
	}
	signal_pending_ipi();
	return ret;
#else
	z_sched_usage_switch(_kernel.ready_q.cache);
	_current->switch_handle = interrupted;
	set_current(_kernel.ready_q.cache);
	return _current->switch_handle;
#endif /* CONFIG_SMP */
}
#endif /* CONFIG_USE_SWITCH */

int z_unpend_all(_wait_q_t *wait_q)
{
	int need_sched = 0;
	struct k_thread *thread;

	for (thread = z_waitq_head(wait_q); thread != NULL; thread = z_waitq_head(wait_q)) {
		z_unpend_thread(thread);
		z_ready_thread(thread);
		need_sched = 1;
	}

	return need_sched;
}

void init_ready_q(struct _ready_q *ready_q)
{
	_priq_run_init(&ready_q->runq);
}

void z_sched_init(void)
{
#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		init_ready_q(&_kernel.cpus[i].ready_q);
	}
#else
	init_ready_q(&_kernel.ready_q);
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */
}

void z_impl_k_thread_priority_set(k_tid_t thread, int prio)
{
	/*
	 * Use NULL, since we cannot know what the entry point is (we do not
	 * keep track of it) and idle cannot change its priority.
	 */
	Z_ASSERT_VALID_PRIO(prio, NULL);

	bool need_sched = z_thread_prio_set((struct k_thread *)thread, prio);

	if ((need_sched) && (IS_ENABLED(CONFIG_SMP) ||
			     (_current->base.sched_locked == 0U))) {
		z_reschedule_unlocked();
	}
}

#ifdef CONFIG_USERSPACE
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
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_SCHED_DEADLINE
void z_impl_k_thread_deadline_set(k_tid_t tid, int deadline)
{

	deadline = CLAMP(deadline, 0, INT_MAX);

	struct k_thread *thread = tid;
	int32_t newdl = k_cycle_get_32() + deadline;

	/* The prio_deadline field changes the sorting order, so can't
	 * change it while the thread is in the run queue (dlists
	 * actually are benign as long as we requeue it before we
	 * release the lock, but an rbtree will blow up if we break
	 * sorting!)
	 */
	K_SPINLOCK(&_sched_spinlock) {
		if (z_is_thread_queued(thread)) {
			dequeue_thread(thread);
			thread->base.prio_deadline = newdl;
			queue_thread(thread);
		} else {
			thread->base.prio_deadline = newdl;
		}
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	K_OOPS(K_SYSCALL_VERIFY_MSG(deadline > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	z_impl_k_thread_deadline_set((k_tid_t)thread, deadline);
}
#include <zephyr/syscalls/k_thread_deadline_set_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_SCHED_DEADLINE */

bool k_can_yield(void)
{
	return !(k_is_pre_kernel() || k_is_in_isr() ||
		 z_is_idle_thread_object(_current));
}

void z_impl_k_yield(void)
{
	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_FUNC(k_thread, yield);

	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	if (!IS_ENABLED(CONFIG_SMP) ||
	    z_is_thread_queued(_current)) {
		dequeue_thread(_current);
	}
	queue_thread(_current);
	update_cache(1);
	z_swap(&_sched_spinlock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_yield(void)
{
	z_impl_k_yield();
}
#include <zephyr/syscalls/k_yield_mrsh.c>
#endif /* CONFIG_USERSPACE */

static int32_t z_tick_sleep(k_ticks_t ticks)
{
	uint32_t expected_wakeup_ticks;

	__ASSERT(!arch_is_in_isr(), "");

	LOG_DBG("thread %p for %lu ticks", _current, (unsigned long)ticks);

	/* wait of 0 ms is treated as a 'yield' */
	if (ticks == 0) {
		k_yield();
		return 0;
	}

	if (Z_TICK_ABS(ticks) <= 0) {
		expected_wakeup_ticks = ticks + sys_clock_tick_get_32();
	} else {
		expected_wakeup_ticks = Z_TICK_ABS(ticks);
	}

	k_timeout_t timeout = Z_TIMEOUT_TICKS(ticks);
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif /* CONFIG_TIMESLICING && CONFIG_SWAP_NONATOMIC */
	unready_thread(_current);
	z_add_thread_timeout(_current, timeout);
	z_mark_thread_as_suspended(_current);

	(void)z_swap(&_sched_spinlock, key);

	__ASSERT(!z_is_thread_state_set(_current, _THREAD_SUSPENDED), "");

	ticks = (k_ticks_t)expected_wakeup_ticks - sys_clock_tick_get_32();
	if (ticks > 0) {
		return ticks;
	}

	return 0;
}

int32_t z_impl_k_sleep(k_timeout_t timeout)
{
	k_ticks_t ticks;

	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, sleep, timeout);

	/* in case of K_FOREVER, we suspend */
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {

		k_thread_suspend(_current);
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, (int32_t) K_TICKS_FOREVER);

		return (int32_t) K_TICKS_FOREVER;
	}

	ticks = timeout.ticks;

	ticks = z_tick_sleep(ticks);

	int32_t ret = k_ticks_to_ms_ceil64(ticks);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_sleep(k_timeout_t timeout)
{
	return z_impl_k_sleep(timeout);
}
#include <zephyr/syscalls/k_sleep_mrsh.c>
#endif /* CONFIG_USERSPACE */

int32_t z_impl_k_usleep(int32_t us)
{
	int32_t ticks;

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, usleep, us);

	ticks = k_us_to_ticks_ceil64(us);
	ticks = z_tick_sleep(ticks);

	int32_t ret = k_ticks_to_us_ceil64(ticks);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, usleep, us, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_usleep(int32_t us)
{
	return z_impl_k_usleep(us);
}
#include <zephyr/syscalls/k_usleep_mrsh.c>
#endif /* CONFIG_USERSPACE */

void z_impl_k_wakeup(k_tid_t thread)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_thread, wakeup, thread);

	if (z_is_thread_pending(thread)) {
		return;
	}

	if (z_abort_thread_timeout(thread) < 0) {
		/* Might have just been sleeping forever */
		if (thread->base.thread_state != _THREAD_SUSPENDED) {
			return;
		}
	}

	k_spinlock_key_t  key = k_spin_lock(&_sched_spinlock);

	z_mark_thread_as_not_suspended(thread);

	if (thread_active_elsewhere(thread) == NULL) {
		ready_thread(thread);
	}

	if (arch_is_in_isr()) {
		k_spin_unlock(&_sched_spinlock, key);
	} else {
		z_reschedule(&_sched_spinlock, key);
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_wakeup(k_tid_t thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_wakeup(thread);
}
#include <zephyr/syscalls/k_wakeup_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_tid_t z_impl_k_sched_current_thread_query(void)
{
	return arch_current_thread();
}

#ifdef CONFIG_USERSPACE
static inline k_tid_t z_vrfy_k_sched_current_thread_query(void)
{
	return z_impl_k_sched_current_thread_query();
}
#include <zephyr/syscalls/k_sched_current_thread_query_mrsh.c>
#endif /* CONFIG_USERSPACE */

static inline void unpend_all(_wait_q_t *wait_q)
{
	struct k_thread *thread;

	for (thread = z_waitq_head(wait_q); thread != NULL; thread = z_waitq_head(wait_q)) {
		unpend_thread_no_timeout(thread);
		(void)z_abort_thread_timeout(thread);
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
static void halt_thread(struct k_thread *thread, uint8_t new_state)
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
			(void)z_abort_thread_timeout(thread);
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

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
		arch_float_disable(thread);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

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
		 */
		if (dummify && !IS_ENABLED(CONFIG_ARCH_POSIX)) {
#ifdef CONFIG_USE_SWITCH
			_current->switch_handle = _current;
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

void z_thread_abort(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

	if (z_is_thread_essential(thread)) {
		k_spin_unlock(&_sched_spinlock, key);
		__ASSERT(false, "aborting essential thread %p", thread);
		k_panic();
		return;
	}

	if ((thread->base.thread_state & _THREAD_DEAD) != 0U) {
		k_spin_unlock(&_sched_spinlock, key);
		return;
	}

	z_thread_halt(thread, key, true);
}

#if !defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void z_impl_k_thread_abort(k_tid_t thread)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, abort, thread);

	z_thread_abort(thread);

	__ASSERT_NO_MSG((thread->base.thread_state & _THREAD_DEAD) != 0);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, abort, thread);
}
#endif /* !CONFIG_ARCH_HAS_THREAD_ABORT */

int z_impl_k_thread_join(struct k_thread *thread, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_thread, join, thread, timeout);

	if ((thread->base.thread_state & _THREAD_DEAD) != 0U) {
		z_sched_switch_spin(thread);
		ret = 0;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		ret = -EBUSY;
	} else if ((thread == _current) ||
		   (thread->base.pended_on == &_current->join_queue)) {
		ret = -EDEADLK;
	} else {
		__ASSERT(!arch_is_in_isr(), "cannot join in ISR");
		add_to_waitq_locked(_current, &thread->join_queue);
		add_thread_timeout(_current, timeout);

		SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_thread, join, thread, timeout);
		ret = z_swap(&_sched_spinlock, key);
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, join, thread, timeout, ret);

		return ret;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_thread, join, thread, timeout, ret);

	k_spin_unlock(&_sched_spinlock, key);
	return ret;
}

#ifdef CONFIG_USERSPACE
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
		K_OOPS(K_SYSCALL_VERIFY_MSG(ret, "access denied"));
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
#endif /* CONFIG_USERSPACE */

/*
 * future scheduler.h API implementations
 */
bool z_sched_wake(_wait_q_t *wait_q, int swap_retval, void *swap_data)
{
	struct k_thread *thread;
	bool ret = false;

	K_SPINLOCK(&_sched_spinlock) {
		thread = _priq_wait_best(&wait_q->waitq);

		if (thread != NULL) {
			z_thread_return_value_set_with_data(thread,
							    swap_retval,
							    swap_data);
			unpend_thread_no_timeout(thread);
			(void)z_abort_thread_timeout(thread);
			ready_thread(thread);
			ret = true;
		}
	}

	return ret;
}

int z_sched_wait(struct k_spinlock *lock, k_spinlock_key_t key,
		 _wait_q_t *wait_q, k_timeout_t timeout, void **data)
{
	int ret = z_pend_curr(lock, key, wait_q, timeout);

	if (data != NULL) {
		*data = _current->base.swap_data;
	}
	return ret;
}

int z_sched_waitq_walk(_wait_q_t  *wait_q,
		       int (*func)(struct k_thread *, void *), void *data)
{
	struct k_thread *thread;
	int  status = 0;

	K_SPINLOCK(&_sched_spinlock) {
		_WAIT_Q_FOR_EACH(wait_q, thread) {

			/*
			 * Invoke the callback function on each waiting thread
			 * for as long as there are both waiting threads AND
			 * it returns 0.
			 */

			status = func(thread, data);
			if (status != 0) {
				break;
			}
		}
	}

	return status;
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
