/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <debug/object_tracing_common.h>
#include <init.h>
#include <wait_q.h>
#include <syscall_handler.h>

extern struct k_timer _k_timer_list_start[];
extern struct k_timer _k_timer_list_end[];

#ifdef CONFIG_OBJECT_TRACING

struct k_timer *_trace_list_k_timer;

/*
 * Complete initialization of statically defined timers.
 */
static int init_timer_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_timer *timer;

	for (timer = _k_timer_list_start; timer < _k_timer_list_end; timer++) {
		SYS_TRACING_OBJ_INIT(k_timer, timer);
	}
	return 0;
}

SYS_INIT(init_timer_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

/**
 * @brief Handle expiration of a kernel timer object.
 *
 * @param t  Timeout used by the timer.
 *
 * @return N/A
 */
void _timer_expiration_handler(struct _timeout *t)
{
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *thread;
	unsigned int key;

	/*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
	if (timer->period > 0) {
		key = irq_lock();
		_add_timeout(NULL, &timer->timeout, &timer->wait_q,
				timer->period);
		irq_unlock(key);
	}

	/* update timer's status */
	timer->status += 1;

	/* invoke timer expiry function */
	if (timer->expiry_fn) {
		timer->expiry_fn(timer);
	}

	thread = (struct k_thread *)sys_dlist_peek_head(&timer->wait_q);

	if (!thread) {
		return;
	}

	/*
	 * Interrupts _DO NOT_ have to be locked in this specific instance of
	 * calling _unpend_thread() because a) this is the only place a thread
	 * can be taken off this pend queue, and b) the only place a thread
	 * can be put on the pend queue is at thread level, which of course
	 * cannot interrupt the current context.
	 */
	_unpend_thread(thread);

	key = irq_lock();
	_ready_thread(thread);
	irq_unlock(key);

	_set_thread_return_value(thread, 0);
}


void k_timer_init(struct k_timer *timer,
		  void (*expiry_fn)(struct k_timer *),
		  void (*stop_fn)(struct k_timer *))
{
	timer->expiry_fn = expiry_fn;
	timer->stop_fn = stop_fn;
	timer->status = 0;

	sys_dlist_init(&timer->wait_q);
	_init_timeout(&timer->timeout, _timer_expiration_handler);
	SYS_TRACING_OBJ_INIT(k_timer, timer);

	timer->user_data = NULL;

	_k_object_init(timer);
}


void _impl_k_timer_start(struct k_timer *timer, s32_t duration, s32_t period)
{
	__ASSERT(duration >= 0 && period >= 0 &&
		 (duration != 0 || period != 0), "invalid parameters\n");

	volatile s32_t period_in_ticks, duration_in_ticks;

	period_in_ticks = _ms_to_ticks(period);
	duration_in_ticks = _ms_to_ticks(duration);

	unsigned int key = irq_lock();

	if (timer->timeout.delta_ticks_from_prev != _INACTIVE) {
		_abort_timeout(&timer->timeout);
	}

	timer->period = period_in_ticks;
	timer->status = 0;
	_add_timeout(NULL, &timer->timeout, &timer->wait_q, duration_in_ticks);
	irq_unlock(key);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_timer_start, timer, duration_p, period_p)
{
	s32_t duration, period;

	duration = (s32_t)duration_p;
	period = (s32_t)period_p;

	_SYSCALL_VERIFY(duration >= 0 && period >= 0 &&
			(duration != 0 || period != 0));
	_SYSCALL_OBJ(timer, K_OBJ_TIMER);
	_impl_k_timer_start((struct k_timer *)timer, duration, period);
	return 0;
}
#endif

void _impl_k_timer_stop(struct k_timer *timer)
{
	int key = irq_lock();
	int inactive = (_abort_timeout(&timer->timeout) == _INACTIVE);

	irq_unlock(key);

	if (inactive) {
		return;
	}

	if (timer->stop_fn) {
		timer->stop_fn(timer);
	}

	key = irq_lock();
	struct k_thread *pending_thread = _unpend_first_thread(&timer->wait_q);

	if (pending_thread) {
		_ready_thread(pending_thread);
	}

	if (_is_in_isr()) {
		irq_unlock(key);
	} else {
		_reschedule_threads(key);
	}
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE_VOID(k_timer_stop, K_OBJ_TIMER, struct k_timer *);
#endif

u32_t _impl_k_timer_status_get(struct k_timer *timer)
{
	unsigned int key = irq_lock();
	u32_t result = timer->status;

	timer->status = 0;
	irq_unlock(key);

	return result;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE(k_timer_status_get, K_OBJ_TIMER, struct k_timer *);
#endif

u32_t _impl_k_timer_status_sync(struct k_timer *timer)
{
	__ASSERT(!_is_in_isr(), "");

	unsigned int key = irq_lock();
	u32_t result = timer->status;

	if (result == 0) {
		if (timer->timeout.delta_ticks_from_prev != _INACTIVE) {
			/* wait for timer to expire or stop */
			_pend_current_thread(&timer->wait_q, K_FOREVER);
			_Swap(key);

			/* get updated timer status */
			key = irq_lock();
			result = timer->status;
		} else {
			/* timer is already stopped */
		}
	} else {
		/* timer has already expired at least once */
	}

	timer->status = 0;
	irq_unlock(key);

	return result;
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE(k_timer_status_sync, K_OBJ_TIMER, struct k_timer *);
#endif

s32_t _timeout_remaining_get(struct _timeout *timeout)
{
	unsigned int key = irq_lock();
	s32_t remaining_ticks;

	if (timeout->delta_ticks_from_prev == _INACTIVE) {
		remaining_ticks = 0;
	} else {
		/*
		 * compute remaining ticks by walking the timeout list
		 * and summing up the various tick deltas involved
		 */
		struct _timeout *t =
			(struct _timeout *)sys_dlist_peek_head(&_timeout_q);

		remaining_ticks = t->delta_ticks_from_prev;
		while (t != timeout) {
			t = (struct _timeout *)sys_dlist_peek_next(&_timeout_q,
								   &t->node);
			remaining_ticks += t->delta_ticks_from_prev;
		}
	}

	irq_unlock(key);
	return __ticks_to_ms(remaining_ticks);
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER1_SIMPLE(k_timer_remaining_get, K_OBJ_TIMER, struct k_timer *);
_SYSCALL_HANDLER1_SIMPLE(k_timer_user_data_get, K_OBJ_TIMER, struct k_timer *);

_SYSCALL_HANDLER(k_timer_user_data_set, timer, user_data)
{
	_SYSCALL_OBJ(timer, K_OBJ_TIMER);
	_impl_k_timer_user_data_set((struct k_timer *)timer, (void *)user_data);
	return 0;
}
#endif
