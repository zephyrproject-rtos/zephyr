/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <kernel.h>
#include <misc/debug/object_tracing_common.h>
#include <init.h>
#include <wait_q.h>

extern struct k_timer _k_timer_list_start[];
extern struct k_timer _k_timer_list_end[];

struct k_timer *_trace_list_k_timer;

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS

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

#endif /* CONFIG_DEBUG_TRACING_KERNEL_OBJECTS */

/**
 * @brief Handle expiration of a kernel timer object.
 *
 * @param t  Timeout used by the timer.
 *
 * @return N/A
 */
void _timer_expiration_handler(struct _timeout *t)
{
	int key = irq_lock();
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *pending_thread;

	/*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
	if (timer->period > 0) {
		_add_timeout(NULL, &timer->timeout, &timer->wait_q,
				timer->period);
	}

	/* update timer's status */
	timer->status += 1;

	/* invoke timer expiry function */
	if (timer->expiry_fn) {
		timer->expiry_fn(timer);
	}
	/*
	 * wake up the (only) thread waiting on the timer, if there is one;
	 * don't invoke _Swap() since the timeout ISR called us, not a thread
	 */
	pending_thread = _unpend_first_thread(&timer->wait_q);
	if (pending_thread) {
		_ready_thread(pending_thread);
		_set_thread_return_value(pending_thread, 0);
	}

	irq_unlock(key);
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

	timer->_legacy_data = NULL;
}


void k_timer_start(struct k_timer *timer, int32_t duration, int32_t period)
{
	__ASSERT(duration >= 0 && period >= 0 &&
		 (duration != 0 || period != 0), "invalid parameters\n");

	unsigned int key = irq_lock();

	if (timer->timeout.delta_ticks_from_prev != -1) {
		_abort_timeout(&timer->timeout);
	}

	timer->period = _ms_to_ticks(period);
	_add_timeout(NULL, &timer->timeout, &timer->wait_q,
			_TICK_ALIGN + _ms_to_ticks(duration));
	timer->status = 0;
	irq_unlock(key);
}


void k_timer_stop(struct k_timer *timer)
{
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();
	int stopped = _abort_timeout(&timer->timeout);

	irq_unlock(key);

	if (stopped == -1) {
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


uint32_t k_timer_status_get(struct k_timer *timer)
{
	unsigned int key = irq_lock();
	uint32_t result = timer->status;

	timer->status = 0;
	irq_unlock(key);

	return result;
}


uint32_t k_timer_status_sync(struct k_timer *timer)
{
	__ASSERT(!_is_in_isr(), "");

	unsigned int key = irq_lock();
	uint32_t result = timer->status;

	if (result == 0) {
		if (timer->timeout.delta_ticks_from_prev != -1) {
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


int32_t k_timer_remaining_get(struct k_timer *timer)
{
	unsigned int key = irq_lock();
	int32_t remaining_ticks;

	if (timer->timeout.delta_ticks_from_prev == -1) {
		remaining_ticks = 0;
	} else {
		/*
		 * compute remaining ticks by walking the timeout list
		 * and summing up the various tick deltas involved
		 */
		struct _timeout *t =
			(struct _timeout *)sys_dlist_peek_head(&_timeout_q);

		remaining_ticks = t->delta_ticks_from_prev;
		while (t != &timer->timeout) {
			t = (struct _timeout *)sys_dlist_peek_next(&_timeout_q,
								   &t->node);
			remaining_ticks += t->delta_ticks_from_prev;
		}
	}

	irq_unlock(key);
	return _ticks_to_ms(remaining_ticks);
}
