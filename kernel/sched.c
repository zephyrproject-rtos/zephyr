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

#if defined(CONFIG_SCHED_DUMB)
#define _priq_run_add		_priq_dumb_add
#define _priq_run_remove	_priq_dumb_remove
#define _priq_run_best		_priq_dumb_best
#elif defined(CONFIG_SCHED_SCALABLE)
#define _priq_run_add		_priq_rb_add
#define _priq_run_remove	_priq_rb_remove
#define _priq_run_best		_priq_rb_best
#elif defined(CONFIG_SCHED_MULTIQ)
#define _priq_run_add		_priq_mq_add
#define _priq_run_remove	_priq_mq_remove
#define _priq_run_best		_priq_mq_best
#endif

#if defined(CONFIG_WAITQ_SCALABLE)
#define _priq_wait_add		_priq_rb_add
#define _priq_wait_remove	_priq_rb_remove
#define _priq_wait_best		_priq_rb_best
#elif defined(CONFIG_WAITQ_DUMB)
#define _priq_wait_add		_priq_dumb_add
#define _priq_wait_remove	_priq_dumb_remove
#define _priq_wait_best		_priq_dumb_best
#endif

/* the only struct _kernel instance */
struct _kernel _kernel;

static struct k_spinlock sched_lock;

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			!__i.key;					\
			k_spin_unlock(lck, __key), __i.key = 1)

static inline int _is_preempt(struct k_thread *thread)
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
static inline int _is_thread_dummy(struct k_thread *thread)
{
	return !!(thread->base.thread_state & _THREAD_DUMMY);
}
#endif

static inline int _is_idle(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	return thread->base.is_idle;
#else
	extern struct k_thread * const _idle_thread;

	return thread == _idle_thread;
#endif
}

int _is_t1_higher_prio_than_t2(struct k_thread *t1, struct k_thread *t2)
{
	if (t1->base.prio < t2->base.prio) {
		return 1;
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

	return 0;
}

static int should_preempt(struct k_thread *th, int preempt_ok)
{
	/* Preemption is OK if it's being explicitly allowed by
	 * software state (e.g. the thread called k_yield())
	 */
	if (preempt_ok) {
		return 1;
	}

	/* Or if we're pended/suspended/dummy (duh) */
	if (!_current || !_is_thread_ready(_current)) {
		return 1;
	}

	/* Otherwise we have to be running a preemptible thread or
	 * switching to a metairq
	 */
	if (_is_preempt(_current) || is_metairq(th)) {
		return 1;
	}

	/* The idle threads can look "cooperative" if there are no
	 * preemptible priorities (this is sort of an API glitch).
	 * They must always be preemptible.
	 */
	if (_is_idle(_current)) {
		return 1;
	}

	return 0;
}

static struct k_thread *next_up(void)
{
#ifndef CONFIG_SMP
	/* In uniprocessor mode, we can leave the current thread in
	 * the queue (actually we have to, otherwise the assembly
	 * context switch code for all architectures would be
	 * responsible for putting it back in _Swap and ISR return!),
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
	int queued = _is_thread_queued(_current);
	int active = !_is_thread_prevented_from_running(_current);

	/* Choose the best thread that is not current */
	struct k_thread *th = _priq_run_best(&_kernel.ready_q.runq);
	if (th == NULL) {
		th = _current_cpu->idle_thread;
	}

	if (active) {
		if (!queued &&
		    !_is_t1_higher_prio_than_t2(th, _current)) {
			th = _current;
		}

		if (!should_preempt(th, _current_cpu->swap_ok)) {
			th = _current;
		}
	}

	/* Put _current back into the queue */
	if (th != _current && active && !_is_idle(_current) && !queued) {
		_priq_run_add(&_kernel.ready_q.runq, _current);
		_mark_thread_as_queued(_current);
	}

	/* Take the new _current out of the queue */
	if (_is_thread_queued(th)) {
		_priq_run_remove(&_kernel.ready_q.runq, th);
	}
	_mark_thread_as_not_queued(th);

	return th;
#endif
}

static void update_cache(int preempt_ok)
{
#ifndef CONFIG_SMP
	struct k_thread *th = next_up();

	if (should_preempt(th, preempt_ok)) {
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

void _add_thread_to_ready_q(struct k_thread *thread)
{
	LOCKED(&sched_lock) {
		_priq_run_add(&_kernel.ready_q.runq, thread);
		_mark_thread_as_queued(thread);
		update_cache(0);
	}
}

void _move_thread_to_end_of_prio_q(struct k_thread *thread)
{
	LOCKED(&sched_lock) {
		_priq_run_remove(&_kernel.ready_q.runq, thread);
		_priq_run_add(&_kernel.ready_q.runq, thread);
		_mark_thread_as_queued(thread);
		update_cache(0);
	}
}

void _remove_thread_from_ready_q(struct k_thread *thread)
{
	LOCKED(&sched_lock) {
		if (_is_thread_queued(thread)) {
			_priq_run_remove(&_kernel.ready_q.runq, thread);
			_mark_thread_as_not_queued(thread);
			update_cache(thread == _current);
		}
	}
}

static void pend(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	_remove_thread_from_ready_q(thread);
	_mark_thread_as_pending(thread);

	/* The timeout handling is currently synchronized external to
	 * the scheduler using the legacy global lock.  Should fix
	 * that.
	 */
	if (timeout != K_FOREVER) {
		s32_t ticks = _TICK_ALIGN + _ms_to_ticks(timeout);
		unsigned int key = irq_lock();

		_add_thread_timeout(thread, wait_q, ticks);
		irq_unlock(key);
	}

	if (wait_q != NULL) {
#ifdef CONFIG_WAITQ_SCALABLE
		thread->base.pended_on = wait_q;
#endif
		_priq_wait_add(&wait_q->waitq, thread);
	}

	sys_trace_thread_pend(thread);
}

void _pend_thread(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	__ASSERT_NO_MSG(thread == _current || _is_thread_dummy(thread));
	pend(thread, wait_q, timeout);
}

static _wait_q_t *pended_on(struct k_thread *thread)
{
#ifdef CONFIG_WAITQ_SCALABLE
	__ASSERT_NO_MSG(thread->base.pended_on);

	return thread->base.pended_on;
#else
	ARG_UNUSED(thread);
	return NULL;
#endif
}

struct k_thread *_find_first_thread_to_unpend(_wait_q_t *wait_q,
					      struct k_thread *from)
{
	ARG_UNUSED(from);

	struct k_thread *ret = NULL;

	LOCKED(&sched_lock) {
		ret = _priq_wait_best(&wait_q->waitq);
	}

	return ret;
}

void _unpend_thread_no_timeout(struct k_thread *thread)
{
	LOCKED(&sched_lock) {
		_priq_wait_remove(&pended_on(thread)->waitq, thread);
		_mark_thread_as_not_pending(thread);
	}

#if defined(CONFIG_ASSERT) && defined(CONFIG_WAITQ_SCALABLE)
	thread->base.pended_on = NULL;
#endif
}

int _pend_current_thread(int key, _wait_q_t *wait_q, s32_t timeout)
{
	pend(_current, wait_q, timeout);
	return _Swap(key);
}

struct k_thread *_unpend_first_thread(_wait_q_t *wait_q)
{
	struct k_thread *t = _unpend1_no_timeout(wait_q);

	if (t != NULL) {
		(void)_abort_thread_timeout(t);
	}

	return t;
}

void _unpend_thread(struct k_thread *thread)
{
	_unpend_thread_no_timeout(thread);
	(void)_abort_thread_timeout(thread);
}

/* FIXME: this API is glitchy when used in SMP.  If the thread is
 * currently scheduled on the other CPU, it will silently set it's
 * priority but nothing will cause a reschedule until the next
 * interrupt.  An audit seems to show that all current usage is to set
 * priorities on either _current or a pended thread, though, so it's
 * fine for now.
 */
void _thread_priority_set(struct k_thread *thread, int prio)
{
	int need_sched = 0;

	LOCKED(&sched_lock) {
		need_sched = _is_thread_ready(thread);

		if (need_sched) {
			_priq_run_remove(&_kernel.ready_q.runq, thread);
			thread->base.prio = prio;
			_priq_run_add(&_kernel.ready_q.runq, thread);
			update_cache(1);
		} else {
			thread->base.prio = prio;
		}
	}
	sys_trace_thread_priority_set(thread);

	if (need_sched) {
		_reschedule(irq_lock());
	}
}

void _reschedule(int key)
{
#ifdef CONFIG_SMP
	if (!_current_cpu->swap_ok) {
		goto noswap;
	}

	_current_cpu->swap_ok = 0;
#endif

	if (_is_in_isr()) {
		goto noswap;
	}

#ifdef CONFIG_SMP
	return _Swap(key);
#else
	if (_get_next_ready_thread() != _current) {
		(void)_Swap(key);
		return;
	}
#endif

 noswap:
	irq_unlock(key);
}

void k_sched_lock(void)
{
	LOCKED(&sched_lock) {
		_sched_lock();
	}
}

void k_sched_unlock(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	__ASSERT(_current->base.sched_locked != 0, "");
	__ASSERT(!_is_in_isr(), "");

	LOCKED(&sched_lock) {
		++_current->base.sched_locked;
		update_cache(1);
	}

	K_DEBUG("scheduler unlocked (%p:%d)\n",
		_current, _current->base.sched_locked);

	_reschedule(irq_lock());
#endif
}

#ifdef CONFIG_SMP
struct k_thread *_get_next_ready_thread(void)
{
	struct k_thread *ret = 0;

	LOCKED(&sched_lock) {
		ret = next_up();
	}

	return ret;
}
#endif

#ifdef CONFIG_USE_SWITCH
void *_get_next_switch_handle(void *interrupted)
{
	_current->switch_handle = interrupted;

#ifdef CONFIG_SMP
	LOCKED(&sched_lock) {
		struct k_thread *th = next_up();

		if (_current != th) {
			_current_cpu->swap_ok = 0;
			_current = th;
		}
	}

#else
	_current = _get_next_ready_thread();
#endif

	_check_stack_sentinel();

	return _current->switch_handle;
}
#endif

void _priq_dumb_add(sys_dlist_t *pq, struct k_thread *thread)
{
	struct k_thread *t;

	__ASSERT_NO_MSG(!_is_idle(thread));

	SYS_DLIST_FOR_EACH_CONTAINER(pq, t, base.qnode_dlist) {
		if (_is_t1_higher_prio_than_t2(thread, t)) {
			sys_dlist_insert_before(pq, &t->base.qnode_dlist,
						&thread->base.qnode_dlist);
			return;
		}
	}

	sys_dlist_append(pq, &thread->base.qnode_dlist);
}

void _priq_dumb_remove(sys_dlist_t *pq, struct k_thread *thread)
{
	__ASSERT_NO_MSG(!_is_idle(thread));

	sys_dlist_remove(&thread->base.qnode_dlist);
}

struct k_thread *_priq_dumb_best(sys_dlist_t *pq)
{
	return CONTAINER_OF(sys_dlist_peek_head(pq),
			    struct k_thread, base.qnode_dlist);
}

int _priq_rb_lessthan(struct rbnode *a, struct rbnode *b)
{
	struct k_thread *ta, *tb;

	ta = CONTAINER_OF(a, struct k_thread, base.qnode_rb);
	tb = CONTAINER_OF(b, struct k_thread, base.qnode_rb);

	if (_is_t1_higher_prio_than_t2(ta, tb)) {
		return 1;
	} else if (_is_t1_higher_prio_than_t2(tb, ta)) {
		return 0;
	} else {
		return ta->base.order_key < tb->base.order_key ? 1 : 0;
	}
}

void _priq_rb_add(struct _priq_rb *pq, struct k_thread *thread)
{
	struct k_thread *t;

	__ASSERT_NO_MSG(!_is_idle(thread));

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

void _priq_rb_remove(struct _priq_rb *pq, struct k_thread *thread)
{
	__ASSERT_NO_MSG(!_is_idle(thread));

	rb_remove(&pq->tree, &thread->base.qnode_rb);

	if (!pq->tree.root) {
		pq->next_order_key = 0;
	}
}

struct k_thread *_priq_rb_best(struct _priq_rb *pq)
{
	struct rbnode *n = rb_get_min(&pq->tree);

	return CONTAINER_OF(n, struct k_thread, base.qnode_rb);
}

#ifdef CONFIG_SCHED_MULTIQ
# if (K_LOWEST_THREAD_PRIO - K_HIGHEST_THREAD_PRIO) > 31
# error Too many priorities for multiqueue scheduler (max 32)
# endif
#endif

void _priq_mq_add(struct _priq_mq *pq, struct k_thread *thread)
{
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

	sys_dlist_append(&pq->queues[priority_bit], &thread->base.qnode_dlist);
	pq->bitmask |= (1 << priority_bit);
}

void _priq_mq_remove(struct _priq_mq *pq, struct k_thread *thread)
{
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

	sys_dlist_remove(&thread->base.qnode_dlist);
	if (sys_dlist_is_empty(&pq->queues[priority_bit])) {
		pq->bitmask &= ~(1 << priority_bit);
	}
}

struct k_thread *_priq_mq_best(struct _priq_mq *pq)
{
	if (!pq->bitmask) {
		return NULL;
	}

	sys_dlist_t *l = &pq->queues[__builtin_ctz(pq->bitmask)];

	return CONTAINER_OF(sys_dlist_peek_head(l),
			    struct k_thread, base.qnode_dlist);
}

#ifdef CONFIG_TIMESLICING
extern s32_t _time_slice_duration;    /* Measured in ticks */
extern s32_t _time_slice_elapsed;     /* Measured in ticks */
extern int _time_slice_prio_ceiling;

void k_sched_time_slice_set(s32_t duration_in_ms, int prio)
{
	__ASSERT(duration_in_ms >= 0, "");
	__ASSERT((prio >= 0) && (prio < CONFIG_NUM_PREEMPT_PRIORITIES), "");

	_time_slice_duration = _ms_to_ticks(duration_in_ms);
	_time_slice_elapsed = 0;
	_time_slice_prio_ceiling = prio;
}

int _is_thread_time_slicing(struct k_thread *thread)
{
	int ret = 0;

	/* Should fix API.  Doesn't make sense for non-running threads
	 * to call this
	 */
	__ASSERT_NO_MSG(thread == _current);

	if (_time_slice_duration <= 0 || !_is_preempt(thread) ||
	    _is_prio_higher(thread->base.prio, _time_slice_prio_ceiling)) {
		return 0;
	}


	LOCKED(&sched_lock) {
		struct k_thread *next = _priq_run_best(&_kernel.ready_q.runq);

		if (next != NULL) {
			ret = thread->base.prio == next->base.prio;
		}
	}

	return ret;
}

#ifdef CONFIG_TICKLESS_KERNEL
void z_reset_timeslice(void)
{
	if (_is_thread_time_slicing(_get_next_ready_thread())) {
		_set_time(_time_slice_duration);
	}
}
#endif

/* Must be called with interrupts locked */
/* Should be called only immediately before a thread switch */
void _update_time_slice_before_swap(void)
{
#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_SMP)
	if (!_is_thread_time_slicing(_get_next_ready_thread())) {
		return;
	}

	u32_t remaining = _get_remaining_program_time();

	if (!remaining || (_time_slice_duration < remaining)) {
		_set_time(_time_slice_duration);
	} else {
		/* Account previous elapsed time and reprogram
		 * timer with remaining time
		 */
		_set_time(remaining);
	}

#endif
	/* Restart time slice count at new thread switch */
	_time_slice_elapsed = 0;
}
#endif /* CONFIG_TIMESLICING */

int _unpend_all(_wait_q_t *waitq)
{
	int need_sched = 0;
	struct k_thread *th;

	while ((th = _waitq_head(waitq))) {
		_unpend_thread(th);
		_ready_thread(th);
		need_sched = 1;
	}

	return need_sched;
}

void _sched_init(void)
{
#ifdef CONFIG_SCHED_DUMB
	sys_dlist_init(&_kernel.ready_q.runq);
#endif

#ifdef CONFIG_SCHED_SCALABLE
	_kernel.ready_q.runq = (struct _priq_rb) {
		.tree = {
			.lessthan_fn = _priq_rb_lessthan,
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

int _impl_k_thread_priority_get(k_tid_t thread)
{
	return thread->base.prio;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE(k_thread_priority_get, K_OBJ_THREAD,
			  struct k_thread *);
#endif

void _impl_k_thread_priority_set(k_tid_t tid, int prio)
{
	/*
	 * Use NULL, since we cannot know what the entry point is (we do not
	 * keep track of it) and idle cannot change its priority.
	 */
	_ASSERT_VALID_PRIO(prio, NULL);
	__ASSERT(!_is_in_isr(), "");

	struct k_thread *thread = (struct k_thread *)tid;

	_thread_priority_set(thread, prio);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_priority_set, thread_p, prio)
{
	struct k_thread *thread = (struct k_thread *)thread_p;

	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(_is_valid_prio(prio, NULL),
				    "invalid thread priority %d", (int)prio));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG((s8_t)prio >= thread->base.prio,
				    "thread priority may only be downgraded (%d < %d)",
				    prio, thread->base.prio));

	_impl_k_thread_priority_set((k_tid_t)thread, prio);
	return 0;
}
#endif

#ifdef CONFIG_SCHED_DEADLINE
void _impl_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *th = tid;

	LOCKED(&sched_lock) {
		th->base.prio_deadline = k_cycle_get_32() + deadline;
		if (_is_thread_queued(th)) {
			_priq_run_remove(&_kernel.ready_q.runq, th);
			_priq_run_add(&_kernel.ready_q.runq, th);
		}
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_thread_deadline_set, thread_p, deadline)
{
	struct k_thread *thread = (struct k_thread *)thread_p;

	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(deadline > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	_impl_k_thread_deadline_set((k_tid_t)thread, deadline);
	return 0;
}
#endif
#endif

void _impl_k_yield(void)
{
	__ASSERT(!_is_in_isr(), "");

	if (!_is_idle(_current)) {
		LOCKED(&sched_lock) {
			_priq_run_remove(&_kernel.ready_q.runq, _current);
			_priq_run_add(&_kernel.ready_q.runq, _current);
			update_cache(1);
		}
	}

#ifdef CONFIG_SMP
	(void)_Swap(irq_lock());
#else
	if (_get_next_ready_thread() != _current) {
		(void)_Swap(irq_lock());
	}
#endif
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER0_SIMPLE_VOID(k_yield);
#endif

void _impl_k_sleep(s32_t duration)
{
#ifdef CONFIG_MULTITHREADING
	/* volatile to guarantee that irq_lock() is executed after ticks is
	 * populated
	 */
	volatile s32_t ticks;
	unsigned int key;

	__ASSERT(!_is_in_isr(), "");
	__ASSERT(duration != K_FOREVER, "");

	K_DEBUG("thread %p for %d ns\n", _current, duration);

	/* wait of 0 ms is treated as a 'yield' */
	if (duration == 0) {
		k_yield();
		return;
	}

	ticks = _TICK_ALIGN + _ms_to_ticks(duration);
	key = irq_lock();

	_remove_thread_from_ready_q(_current);
	_add_thread_timeout(_current, NULL, ticks);

	(void)_Swap(key);
#endif
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_sleep, duration)
{
	/* FIXME there were some discussions recently on whether we should
	 * relax this, thread would be unscheduled until k_wakeup issued
	 */
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(duration != K_FOREVER,
				    "sleeping forever not allowed"));
	_impl_k_sleep(duration);

	return 0;
}
#endif

void _impl_k_wakeup(k_tid_t thread)
{
	unsigned int key = irq_lock();

	/* verify first if thread is not waiting on an object */
	if (_is_thread_pending(thread)) {
		irq_unlock(key);
		return;
	}

	if (_abort_thread_timeout(thread) == _INACTIVE) {
		irq_unlock(key);
		return;
	}

	_ready_thread(thread);

	if (_is_in_isr()) {
		irq_unlock(key);
	} else {
		_reschedule(key);
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER1_SIMPLE_VOID(k_wakeup, K_OBJ_THREAD, k_tid_t);
#endif

k_tid_t _impl_k_current_get(void)
{
	return _current;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER0_SIMPLE(k_current_get);
#endif

int _impl_k_is_preempt_thread(void)
{
	return !_is_in_isr() && _is_preempt(_current);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER0_SIMPLE(k_is_preempt_thread);
#endif
