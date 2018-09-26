/* system clock support */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <drivers/system_timer.h>
#include <syscall_handler.h>

#ifdef CONFIG_SYS_CLOCK_EXISTS
#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
#warning "non-optimized system clock frequency chosen: performance may suffer"
#endif
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void _handle_expired_timeouts(sys_dlist_t *expired);
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif
#else
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int z_clock_hw_cycles_per_sec;
#endif
#endif

/* Note that this value is 64 bits, and thus non-atomic on almost all
 * Zephyr archtictures.  And of course it's routinely updated inside
 * timer interrupts.  Access to it must be locked.
 */
static volatile u64_t tick_count;

u64_t z_last_tick_announced;

#ifdef CONFIG_TICKLESS_KERNEL
/*
 * If this flag is set, system clock will run continuously even if
 * there are no timer events programmed. This allows using the
 * system clock to track passage of time without interruption.
 * To save power, this should be turned on only when required.
 */
int _sys_clock_always_on = 1;

static u32_t next_ts;
#endif

u32_t z_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (u32_t)z_clock_uptime();
#else
	return (u32_t)tick_count;
#endif
}

u32_t _impl_k_uptime_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	__ASSERT(_sys_clock_always_on,
		 "Call k_enable_sys_clock_always_on to use clock API");
#endif
	return __ticks_to_ms(z_tick_get_32());
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_uptime_get_32)
{
#ifdef CONFIG_TICKLESS_KERNEL
	Z_OOPS(Z_SYSCALL_VERIFY(_sys_clock_always_on));
#endif
	return _impl_k_uptime_get_32();
}
#endif

s64_t z_tick_get(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return z_clock_uptime();
#else
	unsigned int key = irq_lock();
	s64_t ret = tick_count;

	irq_unlock(key);
	return ret;
#endif
}

void z_tick_set(s64_t val)
{
	unsigned int key = irq_lock();

	__ASSERT_NO_MSG(val > tick_count);
	__ASSERT_NO_MSG(val > z_last_tick_announced);

	tick_count = val;
	irq_unlock(key);
}

s64_t _impl_k_uptime_get(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	__ASSERT(_sys_clock_always_on,
		 "Call k_enable_sys_clock_always_on to use clock API");
#endif
	return __ticks_to_ms(z_tick_get());
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_uptime_get, ret_p)
{
	u64_t *ret = (u64_t *)ret_p;

	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(ret, sizeof(*ret)));
	*ret = _impl_k_uptime_get();
	return 0;
}
#endif

s64_t k_uptime_delta(s64_t *reftime)
{
	s64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

u32_t k_uptime_delta_32(s64_t *reftime)
{
	return (u32_t)k_uptime_delta(reftime);
}

/* handle the expired timeouts in the nano timeout queue */

#ifdef CONFIG_SYS_CLOCK_EXISTS
/*
 * Handle timeouts by dequeuing the expired ones from _timeout_q and queue
 * them on a local one, then doing the real handling from that queue. This
 * allows going through the second queue without needing to have the
 * interrupts locked since it is a local queue. Each expired timeout is marked
 * as _EXPIRED so that an ISR preempting us and releasing an object on which
 * a thread was timing out and expired will not give the object to that thread.
 *
 * Always called from interrupt level, and always only from the system clock
 * interrupt.
 */

static inline void handle_timeouts(s32_t ticks)
{
	sys_dlist_t expired;
	unsigned int key;

	/* init before locking interrupts */
	sys_dlist_init(&expired);

	key = irq_lock();

	sys_dnode_t *next = sys_dlist_peek_head(&_timeout_q);
	struct _timeout *timeout = (struct _timeout *)next;

	K_DEBUG("head: %p, delta: %d\n",
		timeout, timeout ? timeout->delta_ticks_from_prev : -2112);

	if (next == NULL) {
		irq_unlock(key);
		return;
	}

	/*
	 * Dequeue all expired timeouts from _timeout_q, relieving irq lock
	 * pressure between each of them, allowing handling of higher priority
	 * interrupts. We know that no new timeout will be prepended in front
	 * of a timeout which delta is 0, since timeouts of 0 ticks are
	 * prohibited.
	 */

	while (next != NULL) {

		/*
		 * In the case where ticks number is greater than the first
		 * timeout delta of the list, the lag produced by this initial
		 * difference must also be applied to others timeouts in list
		 * until it was entirely consumed.
		 */

		s32_t tmp = timeout->delta_ticks_from_prev;

		if (timeout->delta_ticks_from_prev < ticks) {
			timeout->delta_ticks_from_prev = 0;
		} else {
			timeout->delta_ticks_from_prev -= ticks;
		}

		ticks -= tmp;

		next = sys_dlist_peek_next(&_timeout_q, next);

		if (timeout->delta_ticks_from_prev == 0) {
			sys_dnode_t *node = &timeout->node;

			sys_dlist_remove(node);

			/*
			 * Reverse the order that that were queued in the
			 * timeout_q: timeouts expiring on the same ticks are
			 * queued in the reverse order, time-wise, that they are
			 * added to shorten the amount of time with interrupts
			 * locked while walking the timeout_q. By reversing the
			 * order _again_ when building the expired queue, they
			 * end up being processed in the same order they were
			 * added, time-wise.
			 */

			sys_dlist_prepend(&expired, node);

			timeout->delta_ticks_from_prev = _EXPIRED;

		} else {
			break;
		}

		irq_unlock(key);
		key = irq_lock();

		timeout = (struct _timeout *)next;
	}

	irq_unlock(key);

	_handle_expired_timeouts(&expired);
}
#else
	#define handle_timeouts(ticks) do { } while (false)
#endif

/**
 *
 * @brief Announce ticks to the kernel
 *
 * This function is only to be called by the system clock timer driver when a
 * tick is to be announced to the kernel. It takes care of dequeuing the
 * timers that have expired and wake up the threads pending on them.
 *
 * @return N/A
 */
void z_clock_announce(s32_t ticks)
{
	z_last_tick_announced += ticks;

	__ASSERT_NO_MSG(z_last_tick_announced >= tick_count);

#ifdef CONFIG_SMP
	/* sys_clock timekeeping happens only on the main CPU */
	if (_arch_curr_cpu()->id) {
		return;
	}
#endif

#ifndef CONFIG_TICKLESS_KERNEL
	unsigned int  key;

	K_DEBUG("ticks: %d\n", ticks);

	key = irq_lock();
	tick_count += ticks;
	irq_unlock(key);
#endif
	handle_timeouts(ticks);

#ifdef CONFIG_TIMESLICING
	z_time_slice(ticks);
#endif

#ifdef CONFIG_TICKLESS_KERNEL
	u32_t next_to = _get_next_timeout_expiry();

	next_to = next_to == K_FOREVER ? 0 : next_to;
	next_to = !next_to || (next_ts
			       && next_to) > next_ts ? next_ts : next_to;

	if (next_to) {
		/* Clears current program if next_to = 0 and remaining > 0 */
		int dt = next_to ? next_to : (_sys_clock_always_on ? INT_MAX : K_FOREVER);
		z_clock_set_timeout(dt, false);
	}
#endif
}

int k_enable_sys_clock_always_on(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	int prev_status = _sys_clock_always_on;

	_sys_clock_always_on = 1;
	_enable_sys_clock();

	return prev_status;
#else
	return -ENOTSUP;
#endif
}

void k_disable_sys_clock_always_on(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	_sys_clock_always_on = 0;
#endif
}

#ifdef CONFIG_SYS_CLOCK_EXISTS

extern u64_t z_last_tick_announced;

/* initialize the timeouts part of k_thread when enabled in the kernel */

void _init_timeout(struct _timeout *t, _timeout_func_t func)
{
	/*
	 * Must be initialized here and when dequeueing a timeout so that code
	 * not dealing with timeouts does not have to handle this, such as when
	 * waiting forever on a semaphore.
	 */
	t->delta_ticks_from_prev = _INACTIVE;

	/*
	 * Must be initialized here so that k_wakeup can
	 * verify the thread is not on a wait queue before aborting a timeout.
	 */
	t->wait_q = NULL;

	/*
	 * Must be initialized here, so the _handle_one_timeout()
	 * routine can check if there is a thread waiting on this timeout
	 */
	t->thread = NULL;

	/*
	 * Function must be initialized before being potentially called.
	 */
	t->func = func;

	/*
	 * These are initialized when enqueing on the timeout queue:
	 *
	 *   thread->timeout.node.next
	 *   thread->timeout.node.prev
	 */
}

void _init_thread_timeout(struct _thread_base *thread_base)
{
	_init_timeout(&thread_base->timeout, NULL);
}

/* remove a thread timing out from kernel object's wait queue */

static inline void _unpend_thread_timing_out(struct k_thread *thread,
					     struct _timeout *timeout_obj)
{
	__ASSERT(thread->base.pended_on == (void*)timeout_obj->wait_q, "");
	if (timeout_obj->wait_q) {
		_unpend_thread_no_timeout(thread);
		thread->base.timeout.wait_q = NULL;
	}
}


/*
 * Handle one timeout from the expired timeout queue. Removes it from the wait
 * queue it is on if waiting for an object; in this case, the return value is
 * kept as -EAGAIN, set previously in _Swap().
 */

static inline void _handle_one_expired_timeout(struct _timeout *timeout)
{
	struct k_thread *thread = timeout->thread;
	unsigned int key = irq_lock();

	timeout->delta_ticks_from_prev = _INACTIVE;

	K_DEBUG("timeout %p\n", timeout);
	if (thread) {
		_unpend_thread_timing_out(thread, timeout);
		_mark_thread_as_started(thread);
		_ready_thread(thread);
		irq_unlock(key);
	} else {
		irq_unlock(key);
		if (timeout->func) {
			timeout->func(timeout);
		}
	}
}

/*
 * Loop over all expired timeouts and handle them one by one. Should be called
 * with interrupts unlocked: interrupts will be locked on each interation only
 * for the amount of time necessary.
 */

static void _handle_expired_timeouts(sys_dlist_t *expired)
{
	struct _timeout *timeout, *next;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(expired, timeout, next, node) {
		_handle_one_expired_timeout(timeout);
	}
}

/* returns _INACTIVE if the timer is not active */
int _abort_timeout(struct _timeout *timeout)
{
	if (timeout->delta_ticks_from_prev == _INACTIVE) {
		return _INACTIVE;
	}

	if (!sys_dlist_is_tail(&_timeout_q, &timeout->node)) {
		sys_dnode_t *next_node =
			sys_dlist_peek_next(&_timeout_q, &timeout->node);
		struct _timeout *next = (struct _timeout *)next_node;

		next->delta_ticks_from_prev += timeout->delta_ticks_from_prev;
	}
	sys_dlist_remove(&timeout->node);
	timeout->delta_ticks_from_prev = _INACTIVE;

	return 0;
}

/* returns _INACTIVE if the timer has already expired */
int _abort_thread_timeout(struct k_thread *thread)
{
	return _abort_timeout(&thread->base.timeout);
}

static inline void _dump_timeout(struct _timeout *timeout, int extra_tab)
{
#ifdef CONFIG_KERNEL_DEBUG
	char *tab = extra_tab ? "\t" : "";

	K_DEBUG("%stimeout %p, prev: %p, next: %p\n"
		"%s\tthread: %p, wait_q: %p\n"
		"%s\tticks remaining: %d\n"
		"%s\tfunction: %p\n",
		tab, timeout, timeout->node.prev, timeout->node.next,
		tab, timeout->thread, timeout->wait_q,
		tab, timeout->delta_ticks_from_prev,
		tab, timeout->func);
#endif
}

static inline void _dump_timeout_q(void)
{
#ifdef CONFIG_KERNEL_DEBUG
	struct _timeout *timeout;

	K_DEBUG("_timeout_q: %p, head: %p, tail: %p\n",
		&_timeout_q, _timeout_q.head, _timeout_q.tail);

	SYS_DLIST_FOR_EACH_CONTAINER(&_timeout_q, timeout, node) {
		_dump_timeout(timeout, 1);
	}
#endif
}

/* find the closest deadline in the timeout queue */

s32_t _get_next_timeout_expiry(void)
{
	struct _timeout *t = (struct _timeout *)
			     sys_dlist_peek_head(&_timeout_q);

	return t ? t->delta_ticks_from_prev : K_FOREVER;
}

/*
 * Add timeout to timeout queue. Record waiting thread and wait queue if any.
 *
 * Cannot handle timeout == 0 and timeout == K_FOREVER.
 *
 * If the new timeout is expiring on the same system clock tick as other
 * timeouts already present in the _timeout_q, it is be _prepended_ to these
 * timeouts. This allows exiting the loop sooner, which is good, since
 * interrupts are locked while trying to find the insert point. Note that the
 * timeouts are then processed in the _reverse order_ if they expire on the
 * same tick.
 *
 * This should not cause problems to applications, unless they really expect
 * two timeouts queued very close to one another to expire in the same order
 * they were queued. This could be changed at the cost of potential longer
 * interrupt latency.
 *
 * Must be called with interrupts locked.
 */

void _add_timeout(struct k_thread *thread, struct _timeout *timeout,
		  _wait_q_t *wait_q, s32_t timeout_in_ticks)
{
	__ASSERT(timeout_in_ticks >= 0, "");
	__ASSERT(!thread || thread->base.pended_on == wait_q, "");

	timeout->delta_ticks_from_prev = timeout_in_ticks;
	timeout->thread = thread;
	timeout->wait_q = (sys_dlist_t *)wait_q;

	K_DEBUG("before adding timeout %p\n", timeout);
	_dump_timeout(timeout, 0);
	_dump_timeout_q();

	/* If timer is submitted to expire ASAP with
	 * timeout_in_ticks (duration) as zero value,
	 * then handle timeout immedately without going
	 * through timeout queue.
	 */
	if (!timeout_in_ticks) {
		_handle_one_expired_timeout(timeout);
		return;
	}

	s32_t *delta = &timeout->delta_ticks_from_prev;
	struct _timeout *in_q;

#ifdef CONFIG_TICKLESS_KERNEL
	/*
	 * If some time has already passed since timer was last
	 * programmed, then that time needs to be accounted when
	 * inserting the new timeout. We account for this
	 * by adding the already elapsed time to the new timeout.
	 * This is like adding this timout back in history.
	 */
	u32_t adjusted_timeout;

	*delta += (int)(z_clock_uptime() - z_last_tick_announced);

	adjusted_timeout = *delta;
#endif
	SYS_DLIST_FOR_EACH_CONTAINER(&_timeout_q, in_q, node) {
		if (*delta <= in_q->delta_ticks_from_prev) {
			in_q->delta_ticks_from_prev -= *delta;
			sys_dlist_insert_before(&_timeout_q, &in_q->node,
						&timeout->node);
			goto inserted;
		}

		*delta -= in_q->delta_ticks_from_prev;
	}

	sys_dlist_append(&_timeout_q, &timeout->node);

inserted:
	K_DEBUG("after adding timeout %p\n", timeout);
	_dump_timeout(timeout, 0);
	_dump_timeout_q();

#ifdef CONFIG_TICKLESS_KERNEL
	if (adjusted_timeout < _get_next_timeout_expiry()) {
		z_clock_set_timeout(adjusted_timeout, false);
	}
#endif
}

/*
 * Put thread on timeout queue. Record wait queue if any.
 *
 * Cannot handle timeout == 0 and timeout == K_FOREVER.
 *
 * Must be called with interrupts locked.
 */

void _add_thread_timeout(struct k_thread *thread,
			 _wait_q_t *wait_q,
			 s32_t timeout_in_ticks)
{
	__ASSERT(thread && thread->base.pended_on == wait_q, "");
	_add_timeout(thread, &thread->base.timeout, wait_q, timeout_in_ticks);
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */
