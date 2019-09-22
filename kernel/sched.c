/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <ksched.h>
#include <spinlock.h>
#include <sched_priq.h>
#include <wait_q.h>
#include <kswap.h>
#include <kernel_arch_func.h>
#include <syscall_handler.h>
#include <drivers/timer/system_timer.h>
#include <stdbool.h>
#include <kernel_internal.h>

#if defined(CONFIG_SCHED_DUMB)
#define _priq_run_add		z_priq_dumb_add
#define _priq_run_remove	z_priq_dumb_remove
# if defined(CONFIG_SCHED_CPU_MASK)
#  define _priq_run_best	_priq_dumb_mask_best
# else
#  define _priq_run_best	z_priq_dumb_best
# endif
#elif defined(CONFIG_SCHED_SCALABLE)
#define _priq_run_add		z_priq_rb_add
#define _priq_run_remove	z_priq_rb_remove
#define _priq_run_best		z_priq_rb_best
#elif defined(CONFIG_SCHED_MULTIQ)
#define _priq_run_add		z_priq_mq_add
#define _priq_run_remove	z_priq_mq_remove
#define _priq_run_best		z_priq_mq_best
#endif

#if defined(CONFIG_WAITQ_SCALABLE)
#define z_priq_wait_add		z_priq_rb_add
#define _priq_wait_remove	z_priq_rb_remove
#define _priq_wait_best		z_priq_rb_best
#elif defined(CONFIG_WAITQ_DUMB)
#define z_priq_wait_add		z_priq_dumb_add
#define _priq_wait_remove	z_priq_dumb_remove
#define _priq_wait_best		z_priq_dumb_best
#endif

/* the only struct z_kernel instance */
struct z_kernel _kernel;

static struct k_spinlock sched_spinlock;

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			!__i.key;					\
			k_spin_unlock(lck, __key), __i.key = 1)

static inline int is_preempt(struct k_thread *thread)
{
#ifdef CONFIG_PREEMPT_ENABLED
	/* explanation in kernel_struct.h */
	return thread->base.preempt <= _PREEMPT_THRESHOLD;
#else
	return 0;
#endif
}

static inline int is_metairq(struct k_thread *thread)
{
#if CONFIG_NUM_METAIRQ_PRIORITIES > 0
	return (thread->base.prio - K_HIGHEST_THREAD_PRIO)
		< CONFIG_NUM_METAIRQ_PRIORITIES;
#else
	return 0;
#endif
}

#if CONFIG_ASSERT
static inline bool is_thread_dummy(struct k_thread *thread)
{
	return (thread->base.thread_state & _THREAD_DUMMY) != 0U;
}
#endif

bool z_is_t1_higher_prio_than_t2(struct k_thread *t1, struct k_thread *t2)
{
	if (t1->base.prio < t2->base.prio) {
		return true;
	}

#ifdef CONFIG_SCHED_DEADLINE
	/* Note that we don't care about wraparound conditions.  The
	 * expectation is that the application will have arranged to
	 * block the threads, change their priorities or reset their
	 * deadlines when the job is complete.  Letting the deadlines
	 * go negative is fine and in fact prevents aliasing bugs.
	 */
	if (t1->base.prio == t2->base.prio) {
		int now = (int) k_cycle_get_32();
		int dt1 = t1->base.prio_deadline - now;
		int dt2 = t2->base.prio_deadline - now;

		return dt1 < dt2;
	}
#endif

	return false;
}

static ALWAYS_INLINE bool should_preempt(struct k_thread *th, int preempt_ok)
{
	/* Preemption is OK if it's being explicitly allowed by
	 * software state (e.g. the thread called k_yield())
	 */
	if (preempt_ok != 0) {
		return true;
	}

	__ASSERT(_current != NULL, "");

	/* Or if we're pended/suspended/dummy (duh) */
	if (z_is_thread_prevented_from_running(_current)) {
		return true;
	}

	/* Edge case on ARM where a thread can be pended out of an
	 * interrupt handler before the "synchronous" swap starts
	 * context switching.  Platforms with atomic swap can never
	 * hit this.
	 */
	if (IS_ENABLED(CONFIG_SWAP_NONATOMIC)
	    && z_is_thread_timeout_active(th)) {
		return true;
	}

	/* Otherwise we have to be running a preemptible thread or
	 * switching to a metairq
	 */
	if (is_preempt(_current) || is_metairq(th)) {
		return true;
	}

	/* The idle threads can look "cooperative" if there are no
	 * preemptible priorities (this is sort of an API glitch).
	 * They must always be preemptible.
	 */
	if (!IS_ENABLED(CONFIG_PREEMPT_ENABLED) &&
	    z_is_idle_thread_object(_current)) {
		return true;
	}

	return false;
}

#ifdef CONFIG_SCHED_CPU_MASK
static ALWAYS_INLINE struct k_thread *_priq_dumb_mask_best(sys_dlist_t *pq)
{
	/* With masks enabled we need to be prepared to walk the list
	 * looking for one we can run
	 */
	struct k_thread *t;

	SYS_DLIST_FOR_EACH_CONTAINER(pq, t, base.qnode_dlist) {
		if ((t->base.cpu_mask & BIT(_current_cpu->id)) != 0) {
			return t;
		}
	}
	return NULL;
}
#endif

static ALWAYS_INLINE struct k_thread *next_up(void)
{
#ifndef CONFIG_SMP
	/* In uniprocessor mode, we can leave the current thread in
	 * the queue (actually we have to, otherwise the assembly
	 * context switch code for all architectures would be
	 * responsible for putting it back in z_swap and ISR return!),
	 * which makes this choice simple.
	 */
	struct k_thread *th = _priq_run_best(&_kernel.ready_q.runq);

	return th ? th : _current_cpu->idle_thread;
#else

	/* Under SMP, the "cache" mechanism for selecting the next
	 * thread doesn't work, so we have more work to do to test
	 * _current against the best choice from the queue.
	 *
	 * Subtle note on "queued": in SMP mode, _current does not
	 * live in the queue, so this isn't exactly the same thing as
	 * "ready", it means "is _current already added back to the
	 * queue such that we don't want to re-add it".
	 */
	int queued = z_is_thread_queued(_current);
	int active = !z_is_thread_prevented_from_running(_current);

	/* Choose the best thread that is not current */
	struct k_thread *th = _priq_run_best(&_kernel.ready_q.runq);
	if (th == NULL) {
		th = _current_cpu->idle_thread;
	}

	if (active) {
		if (!queued &&
		    !z_is_t1_higher_prio_than_t2(th, _current)) {
			th = _current;
		}

		if (!should_preempt(th, _current_cpu->swap_ok)) {
			th = _current;
		}
	}

	/* Put _current back into the queue */
	if (th != _current && active && !z_is_idle_thread_object(_current) &&
	    !queued) {
		_priq_run_add(&_kernel.ready_q.runq, _current);
		z_mark_thread_as_queued(_current);
	}

	/* Take the new _current out of the queue */
	if (z_is_thread_queued(th)) {
		_priq_run_remove(&_kernel.ready_q.runq, th);
	}
	z_mark_thread_as_not_queued(th);

	return th;
#endif
}

#ifdef CONFIG_TIMESLICING

static int slice_time;
static int slice_max_prio;

#ifdef CONFIG_SWAP_NONATOMIC
/* If z_swap() isn't atomic, then it's possible for a timer interrupt
 * to try to timeslice away _current after it has already pended
 * itself but before the corresponding context switch.  Treat that as
 * a noop condition in z_time_slice().
 */
static struct k_thread *pending_current;
#endif

void z_reset_time_slice(void)
{
	/* Add the elapsed time since the last announced tick to the
	 * slice count, as we'll see those "expired" ticks arrive in a
	 * FUTURE z_time_slice() call.
	 */
	if (slice_time != 0) {
		_current_cpu->slice_ticks = slice_time + z_clock_elapsed();
		z_set_timeout_expiry(slice_time, false);
	}
}

void k_sched_time_slice_set(s32_t slice, int prio)
{
	LOCKED(&sched_spinlock) {
		_current_cpu->slice_ticks = 0;
		slice_time = z_ms_to_ticks(slice);
		slice_max_prio = prio;
		z_reset_time_slice();
	}
}

static inline int sliceable(struct k_thread *t)
{
	return is_preempt(t)
		&& !z_is_prio_higher(t->base.prio, slice_max_prio)
		&& !z_is_idle_thread_object(t)
		&& !z_is_thread_timeout_active(t);
}

/* Called out of each timer interrupt */
void z_time_slice(int ticks)
{
#ifdef CONFIG_SWAP_NONATOMIC
	if (pending_current == _current) {
		z_reset_time_slice();
		return;
	}
	pending_current = NULL;
#endif

	if (slice_time && sliceable(_current)) {
		if (ticks >= _current_cpu->slice_ticks) {
			z_move_thread_to_end_of_prio_q(_current);
			z_reset_time_slice();
		} else {
			_current_cpu->slice_ticks -= ticks;
		}
	} else {
		_current_cpu->slice_ticks = 0;
	}
}
#endif

static void update_cache(int preempt_ok)
{
#ifndef CONFIG_SMP
	struct k_thread *th = next_up();

	if (should_preempt(th, preempt_ok)) {
#ifdef CONFIG_TIMESLICING
		if (th != _current) {
			z_reset_time_slice();
		}
#endif
		_kernel.ready_q.cache = th;
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
#endif
}

void z_add_thread_to_ready_q(struct k_thread *thread)
{
	LOCKED(&sched_spinlock) {
		_priq_run_add(&_kernel.ready_q.runq, thread);
		z_mark_thread_as_queued(thread);
		update_cache(0);
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
		z_arch_sched_ipi();
#endif
	}
}

void z_move_thread_to_end_of_prio_q(struct k_thread *thread)
{
	LOCKED(&sched_spinlock) {
		if (z_is_thread_queued(thread)) {
			_priq_run_remove(&_kernel.ready_q.runq, thread);
		}
		_priq_run_add(&_kernel.ready_q.runq, thread);
		z_mark_thread_as_queued(thread);
		update_cache(thread == _current);
	}
}

void z_remove_thread_from_ready_q(struct k_thread *thread)
{
	LOCKED(&sched_spinlock) {
		if (z_is_thread_queued(thread)) {
			_priq_run_remove(&_kernel.ready_q.runq, thread);
			z_mark_thread_as_not_queued(thread);
		}
		update_cache(thread == _current);
	}
}

static void pend(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	z_remove_thread_from_ready_q(thread);
	z_mark_thread_as_pending(thread);

	if (wait_q != NULL) {
		thread->base.pended_on = wait_q;
		z_priq_wait_add(&wait_q->waitq, thread);
	}

	if (timeout != K_FOREVER) {
		s32_t ticks = _TICK_ALIGN + z_ms_to_ticks(timeout);

		z_add_thread_timeout(thread, ticks);
	}

	sys_trace_thread_pend(thread);
}

void z_pend_thread(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	__ASSERT_NO_MSG(thread == _current || is_thread_dummy(thread));
	pend(thread, wait_q, timeout);
}

static _wait_q_t *pended_on(struct k_thread *thread)
{
	__ASSERT_NO_MSG(thread->base.pended_on);

	return thread->base.pended_on;
}

ALWAYS_INLINE struct k_thread *z_find_first_thread_to_unpend(_wait_q_t *wait_q,
						     struct k_thread *from)
{
	ARG_UNUSED(from);

	struct k_thread *ret = NULL;

	LOCKED(&sched_spinlock) {
		ret = _priq_wait_best(&wait_q->waitq);
	}

	return ret;
}

ALWAYS_INLINE void z_unpend_thread_no_timeout(struct k_thread *thread)
{
	LOCKED(&sched_spinlock) {
		_priq_wait_remove(&pended_on(thread)->waitq, thread);
		z_mark_thread_as_not_pending(thread);
	}

	thread->base.pended_on = NULL;
}

#ifdef CONFIG_SYS_CLOCK_EXISTS
/* Timeout handler for *_thread_timeout() APIs */
void z_thread_timeout(struct _timeout *to)
{
	struct k_thread *th = CONTAINER_OF(to, struct k_thread, base.timeout);

	if (th->base.pended_on != NULL) {
		z_unpend_thread_no_timeout(th);
	}
	z_mark_thread_as_started(th);
	z_mark_thread_as_not_suspended(th);
	z_ready_thread(th);
}
#endif

int z_pend_curr_irqlock(u32_t key, _wait_q_t *wait_q, s32_t timeout)
{
	pend(_current, wait_q, timeout);

#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;

	int ret = z_swap_irqlock(key);
	LOCKED(&sched_spinlock) {
		if (pending_current == _current) {
			pending_current = NULL;
		}
	}
	return ret;
#else
	return z_swap_irqlock(key);
#endif
}

int z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, s32_t timeout)
{
#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif
	pend(_current, wait_q, timeout);
	return z_swap(lock, key);
}

struct k_thread *z_unpend_first_thread(_wait_q_t *wait_q)
{
	struct k_thread *t = z_unpend1_no_timeout(wait_q);

	if (t != NULL) {
		(void)z_abort_thread_timeout(t);
	}

	return t;
}

void z_unpend_thread(struct k_thread *thread)
{
	z_unpend_thread_no_timeout(thread);
	(void)z_abort_thread_timeout(thread);
}

/* Priority set utility that does no rescheduling, it just changes the
 * run queue state, returning true if a reschedule is needed later.
 */
bool z_set_prio(struct k_thread *thread, int prio)
{
	bool need_sched = 0;

	LOCKED(&sched_spinlock) {
		need_sched = z_is_thread_ready(thread);

		if (need_sched) {
			/* Don't requeue on SMP if it's the running thread */
			if (!IS_ENABLED(CONFIG_SMP) || z_is_thread_queued(thread)) {
				_priq_run_remove(&_kernel.ready_q.runq, thread);
				thread->base.prio = prio;
				_priq_run_add(&_kernel.ready_q.runq, thread);
			} else {
				thread->base.prio = prio;
			}
			update_cache(1);
		} else {
			thread->base.prio = prio;
		}
	}
	sys_trace_thread_priority_set(thread);

	return need_sched;
}

void z_thread_priority_set(struct k_thread *thread, int prio)
{
	bool need_sched = z_set_prio(thread, prio);

	if (IS_ENABLED(CONFIG_SMP) &&
	    !IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
		z_sched_ipi();
	}

	if (need_sched && _current->base.sched_locked == 0) {
		z_reschedule_unlocked();
	}
}

static inline int resched(u32_t key)
{
#ifdef CONFIG_SMP
	_current_cpu->swap_ok = 0;
#endif

	return z_arch_irq_unlocked(key) && !z_arch_is_in_isr();
}

void z_reschedule(struct k_spinlock *lock, k_spinlock_key_t key)
{
	if (resched(key.key)) {
		z_swap(lock, key);
	} else {
		k_spin_unlock(lock, key);
	}
}

void z_reschedule_irqlock(u32_t key)
{
	if (resched(key)) {
		z_swap_irqlock(key);
	} else {
		irq_unlock(key);
	}
}

void k_sched_lock(void)
{
	LOCKED(&sched_spinlock) {
		z_sched_lock();
	}
}

void k_sched_unlock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(_current->base.sched_locked != 0, "");
	__ASSERT(!z_arch_is_in_isr(), "");

	LOCKED(&sched_spinlock) {
		++_current->base.sched_locked;
		update_cache(0);
	}

	K_DEBUG("scheduler unlocked (%p:%d)\n",
		_current, _current->base.sched_locked);

	z_reschedule_unlocked();
#endif
}

#ifdef CONFIG_SMP
struct k_thread *z_get_next_ready_thread(void)
{
	struct k_thread *ret = 0;

	LOCKED(&sched_spinlock) {
		ret = next_up();
	}

	return ret;
}
#endif

/* Just a wrapper around _current = xxx with tracing */
static inline void set_current(struct k_thread *new_thread)
{
	_current = new_thread;
}

#ifdef CONFIG_USE_SWITCH
void *z_get_next_switch_handle(void *interrupted)
{
	_current->switch_handle = interrupted;

	z_check_stack_sentinel();

#ifdef CONFIG_SMP
	LOCKED(&sched_spinlock) {
		struct k_thread *th = next_up();

		if (_current != th) {
#ifdef CONFIG_TIMESLICING
			z_reset_time_slice();
#endif
			_current_cpu->swap_ok = 0;
			set_current(th);
#ifdef SPIN_VALIDATE
			/* Changed _current!  Update the spinlock
			 * bookeeping so the validation doesn't get
			 * confused when the "wrong" thread tries to
			 * release the lock.
			 */
			z_spin_lock_set_owner(&sched_spinlock);
#endif
		}
	}
#else
	set_current(z_get_next_ready_thread());
#endif

	/* Some architectures don't have a working IPI, so the best we
	 * can do there is check the abort status of the current
	 * thread here on ISR exit
	 */
	if (IS_ENABLED(CONFIG_SMP) &&
	    !IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
		z_sched_ipi();
	}
	return _current->switch_handle;
}
#endif

ALWAYS_INLINE void z_priq_dumb_add(sys_dlist_t *pq, struct k_thread *thread)
{
	struct k_thread *t;

	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));

	SYS_DLIST_FOR_EACH_CONTAINER(pq, t, base.qnode_dlist) {
		if (z_is_t1_higher_prio_than_t2(thread, t)) {
			sys_dlist_insert(&t->base.qnode_dlist,
					 &thread->base.qnode_dlist);
			return;
		}
	}

	sys_dlist_append(pq, &thread->base.qnode_dlist);
}

void z_priq_dumb_remove(sys_dlist_t *pq, struct k_thread *thread)
{
#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_SCHED_DUMB)
	if (pq == &_kernel.ready_q.runq && thread == _current &&
	    z_is_thread_prevented_from_running(thread)) {
		return;
	}
#endif

	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));

	sys_dlist_remove(&thread->base.qnode_dlist);
}

struct k_thread *z_priq_dumb_best(sys_dlist_t *pq)
{
	struct k_thread *t = NULL;
	sys_dnode_t *n = sys_dlist_peek_head(pq);

	if (n != NULL) {
		t = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
	}
	return t;
}

bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b)
{
	struct k_thread *ta, *tb;

	ta = CONTAINER_OF(a, struct k_thread, base.qnode_rb);
	tb = CONTAINER_OF(b, struct k_thread, base.qnode_rb);

	if (z_is_t1_higher_prio_than_t2(ta, tb)) {
		return true;
	} else if (z_is_t1_higher_prio_than_t2(tb, ta)) {
		return false;
	} else {
		return ta->base.order_key < tb->base.order_key ? 1 : 0;
	}
}

void z_priq_rb_add(struct _priq_rb *pq, struct k_thread *thread)
{
	struct k_thread *t;

	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));

	thread->base.order_key = pq->next_order_key++;

	/* Renumber at wraparound.  This is tiny code, and in practice
	 * will almost never be hit on real systems.  BUT on very
	 * long-running systems where a priq never completely empties
	 * AND that contains very large numbers of threads, it can be
	 * a latency glitch to loop over all the threads like this.
	 */
	if (!pq->next_order_key) {
		RB_FOR_EACH_CONTAINER(&pq->tree, t, base.qnode_rb) {
			t->base.order_key = pq->next_order_key++;
		}
	}

	rb_insert(&pq->tree, &thread->base.qnode_rb);
}

void z_priq_rb_remove(struct _priq_rb *pq, struct k_thread *thread)
{
#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_SCHED_SCALABLE)
	if (pq == &_kernel.ready_q.runq && thread == _current &&
	    z_is_thread_prevented_from_running(thread)) {
		return;
	}
#endif
	__ASSERT_NO_MSG(!z_is_idle_thread_object(thread));

	rb_remove(&pq->tree, &thread->base.qnode_rb);

	if (!pq->tree.root) {
		pq->next_order_key = 0;
	}
}

struct k_thread *z_priq_rb_best(struct _priq_rb *pq)
{
	struct k_thread *t = NULL;
	struct rbnode *n = rb_get_min(&pq->tree);

	if (n != NULL) {
		t = CONTAINER_OF(n, struct k_thread, base.qnode_rb);
	}
	return t;
}

#ifdef CONFIG_SCHED_MULTIQ
# if (K_LOWEST_THREAD_PRIO - K_HIGHEST_THREAD_PRIO) > 31
# error Too many priorities for multiqueue scheduler (max 32)
# endif
#endif

ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq, struct k_thread *thread)
{
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

	sys_dlist_append(&pq->queues[priority_bit], &thread->base.qnode_dlist);
	pq->bitmask |= BIT(priority_bit);
}

ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq, struct k_thread *thread)
{
#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_SCHED_MULTIQ)
	if (pq == &_kernel.ready_q.runq && thread == _current &&
	    z_is_thread_prevented_from_running(thread)) {
		return;
	}
#endif
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

	sys_dlist_remove(&thread->base.qnode_dlist);
	if (sys_dlist_is_empty(&pq->queues[priority_bit])) {
		pq->bitmask &= ~BIT(priority_bit);
	}
}

struct k_thread *z_priq_mq_best(struct _priq_mq *pq)
{
	if (!pq->bitmask) {
		return NULL;
	}

	struct k_thread *t = NULL;
	sys_dlist_t *l = &pq->queues[__builtin_ctz(pq->bitmask)];
	sys_dnode_t *n = sys_dlist_peek_head(l);

	if (n != NULL) {
		t = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
	}
	return t;
}

int z_unpend_all(_wait_q_t *wait_q)
{
	int need_sched = 0;
	struct k_thread *th;

	while ((th = z_waitq_head(wait_q)) != NULL) {
		z_unpend_thread(th);
		z_ready_thread(th);
		need_sched = 1;
	}

	return need_sched;
}

void z_sched_init(void)
{
#ifdef CONFIG_SCHED_DUMB
	sys_dlist_init(&_kernel.ready_q.runq);
#endif

#ifdef CONFIG_SCHED_SCALABLE
	_kernel.ready_q.runq = (struct _priq_rb) {
		.tree = {
			.lessthan_fn = z_priq_rb_lessthan,
		}
	};
#endif

#ifdef CONFIG_SCHED_MULTIQ
	for (int i = 0; i < ARRAY_SIZE(_kernel.ready_q.runq.queues); i++) {
		sys_dlist_init(&_kernel.ready_q.runq.queues[i]);
	}
#endif

#ifdef CONFIG_TIMESLICING
	k_sched_time_slice_set(CONFIG_TIMESLICE_SIZE,
		CONFIG_TIMESLICE_PRIORITY);
#endif
}

int z_impl_k_thread_priority_get(k_tid_t thread)
{
	return thread->base.prio;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_priority_get(k_tid_t thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_priority_get(thread);
}
#include <syscalls/k_thread_priority_get_mrsh.c>
#endif

void z_impl_k_thread_priority_set(k_tid_t tid, int prio)
{
	/*
	 * Use NULL, since we cannot know what the entry point is (we do not
	 * keep track of it) and idle cannot change its priority.
	 */
	Z_ASSERT_VALID_PRIO(prio, NULL);
	__ASSERT(!z_arch_is_in_isr(), "");

	struct k_thread *thread = (struct k_thread *)tid;

	z_thread_priority_set(thread, prio);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_priority_set(k_tid_t thread, int prio)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(_is_valid_prio(prio, NULL),
				    "invalid thread priority %d", prio));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG((s8_t)prio >= thread->base.prio,
				    "thread priority may only be downgraded (%d < %d)",
				    prio, thread->base.prio));

	z_impl_k_thread_priority_set(thread, prio);
}
#include <syscalls/k_thread_priority_set_mrsh.c>
#endif

#ifdef CONFIG_SCHED_DEADLINE
void z_impl_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *th = tid;

	LOCKED(&sched_spinlock) {
		th->base.prio_deadline = k_cycle_get_32() + deadline;
		if (z_is_thread_queued(th)) {
			_priq_run_remove(&_kernel.ready_q.runq, th);
			_priq_run_add(&_kernel.ready_q.runq, th);
		}
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = (struct k_thread *)thread_p;

	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(deadline > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	z_impl_k_thread_deadline_set((k_tid_t)thread, deadline);
}
#include <syscalls/k_thread_deadline_set_mrsh.c>
#endif
#endif

void z_impl_k_yield(void)
{
	__ASSERT(!z_arch_is_in_isr(), "");

	if (!z_is_idle_thread_object(_current)) {
		LOCKED(&sched_spinlock) {
			if (!IS_ENABLED(CONFIG_SMP) ||
			    z_is_thread_queued(_current)) {
				_priq_run_remove(&_kernel.ready_q.runq,
						 _current);
			}
			_priq_run_add(&_kernel.ready_q.runq, _current);
			z_mark_thread_as_queued(_current);
			update_cache(1);
		}
	}
	z_swap_unlocked();
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_yield(void)
{
	z_impl_k_yield();
}
#include <syscalls/k_yield_mrsh.c>
#endif

static s32_t z_tick_sleep(s32_t ticks)
{
#ifdef CONFIG_MULTITHREADING
	u32_t expected_wakeup_time;

	__ASSERT(!z_arch_is_in_isr(), "");

	K_DEBUG("thread %p for %d ticks\n", _current, ticks);

	/* wait of 0 ms is treated as a 'yield' */
	if (ticks == 0) {
		k_yield();
		return 0;
	}

	ticks += _TICK_ALIGN;
	expected_wakeup_time = ticks + z_tick_get_32();

	/* Spinlock purely for local interrupt locking to prevent us
	 * from being interrupted while _current is in an intermediate
	 * state.  Should unify this implementation with pend().
	 */
	struct k_spinlock local_lock = {};
	k_spinlock_key_t key = k_spin_lock(&local_lock);

#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif
	z_remove_thread_from_ready_q(_current);
	z_add_thread_timeout(_current, ticks);
	z_mark_thread_as_suspended(_current);

	(void)z_swap(&local_lock, key);

	__ASSERT(!z_is_thread_state_set(_current, _THREAD_SUSPENDED), "");

	ticks = expected_wakeup_time - z_tick_get_32();
	if (ticks > 0) {
		return ticks;
	}
#endif

	return 0;
}

s32_t z_impl_k_sleep(int ms)
{
	s32_t ticks;

	ticks = z_ms_to_ticks(ms);
	ticks = z_tick_sleep(ticks);
	return __ticks_to_ms(ticks);
}

#ifdef CONFIG_USERSPACE
static inline s32_t z_vrfy_k_sleep(int ms)
{
	return z_impl_k_sleep(ms);
}
#include <syscalls/k_sleep_mrsh.c>
#endif

s32_t z_impl_k_usleep(int us)
{
	s32_t ticks;

	ticks = z_us_to_ticks(us);
	ticks = z_tick_sleep(ticks);
	return __ticks_to_us(ticks);
}

#ifdef CONFIG_USERSPACE
static inline s32_t z_vrfy_k_usleep(int us)
{
	return z_impl_k_usleep(us);
}
#include <syscalls/k_usleep_mrsh.c>
#endif

void z_impl_k_wakeup(k_tid_t thread)
{
	if (z_is_thread_pending(thread)) {
		return;
	}

	if (z_abort_thread_timeout(thread) < 0) {
		return;
	}

	z_mark_thread_as_not_suspended(thread);
	z_ready_thread(thread);

	if (!z_arch_is_in_isr()) {
		z_reschedule_unlocked();
	}

	if (IS_ENABLED(CONFIG_SMP) &&
	    !IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
		z_sched_ipi();
	}
}

#ifdef CONFIG_SMP
/* Called out of the scheduler interprocessor interrupt.  All it does
 * is flag the current thread as dead if it needs to abort, so the ISR
 * return into something else and the other thread which called
 * k_thread_abort() can finish its work knowing the thing won't be
 * rescheduled.
 */
void z_sched_ipi(void)
{
	LOCKED(&sched_spinlock) {
		if (_current->base.thread_state & _THREAD_ABORTING) {
			_current->base.thread_state |= _THREAD_DEAD;
			_current_cpu->swap_ok = true;
		}
	}
}

void z_sched_abort(struct k_thread *thread)
{
	if (thread == _current) {
		z_remove_thread_from_ready_q(thread);
		return;
	}

	/* First broadcast an IPI to the other CPUs so they can stop
	 * it locally.  Not all architectures support that, alas.  If
	 * we don't have it, we need to wait for some other interrupt.
	 */
	thread->base.thread_state |= _THREAD_ABORTING;
#ifdef CONFIG_SCHED_IPI_SUPPORTED
	z_arch_sched_ipi();
#endif

	/* Wait for it to be flagged dead either by the CPU it was
	 * running on or because we caught it idle in the queue
	 */
	while ((thread->base.thread_state & _THREAD_DEAD) == 0U) {
		LOCKED(&sched_spinlock) {
			if (z_is_thread_prevented_from_running(thread)) {
				__ASSERT(!z_is_thread_queued(thread), "");
				thread->base.thread_state |= _THREAD_DEAD;
			} else if (z_is_thread_queued(thread)) {
				_priq_run_remove(&_kernel.ready_q.runq, thread);
				z_mark_thread_as_not_queued(thread);
				thread->base.thread_state |= _THREAD_DEAD;
			} else {
				k_busy_wait(100);
			}
		}
	}
}
#endif

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_wakeup(k_tid_t thread)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	z_impl_k_wakeup(thread);
}
#include <syscalls/k_wakeup_mrsh.c>
#endif

k_tid_t z_impl_k_current_get(void)
{
	return _current;
}

#ifdef CONFIG_USERSPACE
static inline k_tid_t z_vrfy_k_current_get(void)
{
	return z_impl_k_current_get();
}
#include <syscalls/k_current_get_mrsh.c>
#endif

int z_impl_k_is_preempt_thread(void)
{
	return !z_arch_is_in_isr() && is_preempt(_current);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_is_preempt_thread(void)
{
	return z_impl_k_is_preempt_thread();
}
#include <syscalls/k_is_preempt_thread_mrsh.c>
#endif

#ifdef CONFIG_SCHED_CPU_MASK
# ifdef CONFIG_SMP
/* Right now we use a single byte for this mask */
BUILD_ASSERT_MSG(CONFIG_MP_NUM_CPUS <= 8, "Too many CPUs for mask word");
# endif


static int cpu_mask_mod(k_tid_t t, u32_t enable_mask, u32_t disable_mask)
{
	int ret = 0;

	LOCKED(&sched_spinlock) {
		if (z_is_thread_prevented_from_running(t)) {
			t->base.cpu_mask |= enable_mask;
			t->base.cpu_mask  &= ~disable_mask;
		} else {
			ret = -EINVAL;
		}
	}
	return ret;
}

int k_thread_cpu_mask_clear(k_tid_t thread)
{
	return cpu_mask_mod(thread, 0, 0xffffffff);
}

int k_thread_cpu_mask_enable_all(k_tid_t thread)
{
	return cpu_mask_mod(thread, 0xffffffff, 0);
}

int k_thread_cpu_mask_enable(k_tid_t thread, int cpu)
{
	return cpu_mask_mod(thread, BIT(cpu), 0);
}

int k_thread_cpu_mask_disable(k_tid_t thread, int cpu)
{
	return cpu_mask_mod(thread, 0, BIT(cpu));
}

#endif /* CONFIG_SCHED_CPU_MASK */
