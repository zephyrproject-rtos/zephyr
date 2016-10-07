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

#include <nano_private.h>
#include <misc/debug/object_tracing_common.h>
#include <wait_q.h>
#include <sched.h>
#include <init.h>

/**
 * @brief Timer expire handler
 *
 * @param t  Internal timeout structure
 *
 * @return N/A
 */
void timer_expiration_handler(struct _timeout *t)
{
	int key = irq_lock();
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *first_pending_thread =
		_unpend_first_thread(&timer->wait_q);

	/* if the time is periodic, start it again */
	if (timer->period > 0) {
		_add_timeout(NULL, &timer->timeout, &timer->wait_q,
				timer->period);
	}

	/* once timer is expired, it can return valid user data pointer */
	timer->user_data = timer->user_data_internal;
	/* resume thread waiting on the timer */
	if (first_pending_thread) {
		_ready_thread(first_pending_thread);
		_set_thread_return_value(first_pending_thread, 0);
		/*
		 * Since the routine is called from timer interrupt handler
		 * _Swap() is not invoked
		 */
	}
	if (timer->handler) {
		timer->handler(timer->handler_arg);
	}
	irq_unlock(key);
}


/**
 * @brief Initialize timer structure
 *
 * Routine initializes timer structure parameters and assigns the user
 * supplied data.
 * Routine needs to be called before timer is used
 *
 * @param timer  Pointer to the timer structure to be initialized
 * @param data   Pointer to user supplied data
 *
 * @return N/A
 */
void k_timer_init(struct k_timer *timer, void *data)
{
	timer->user_data = NULL;
	timer->user_data_internal = data;
	timer->period = 0;
	sys_dlist_init(&timer->wait_q);
	_init_timeout(&timer->timeout, timer_expiration_handler);
	SYS_TRACING_OBJ_INIT(micro_timer, timer);
}


#if (CONFIG_NUM_DYNAMIC_TIMERS > 0)

/* Implements legacy API support for dynamic timers */

static struct k_timer _dynamic_timers[CONFIG_NUM_DYNAMIC_TIMERS];
static sys_dlist_t _timer_pool;

/* Initialize the pool of timers for dynamic timer allocation */
static int init_dyamic_timers(struct device *dev)
{
	ARG_UNUSED(dev);

	int i;
	int n_timers = ARRAY_SIZE(_dynamic_timers);

	sys_dlist_init(&_timer_pool);
	for (i = 0; i < n_timers; i++) {
		k_timer_init(&_dynamic_timers[i], NULL);
		sys_dlist_append(&_timer_pool,
				 &_dynamic_timers[i].timeout.node);
	}
	return 0;
}

/**
 * @brief Allocate timer
 *
 * Allocates a new timer timer.
 *
 * @return pointer to the new timer structure
 */
struct k_timer *_k_timer_alloc(void)
{
	k_sched_lock();

	/*
	 * This conversion works only if timeout member
	 * variable is the first in time structure.
	 */
	struct k_timer *timer = (struct k_timer *)sys_dlist_get(&_timer_pool);

	k_sched_unlock();
	return timer;
}


/**
 * @brief Deallocate timer
 *
 * Deallocates timer and inserts it into the timer queue.
 * @param timer  Timer to free
 *
 * @return N/A
 */
void _k_timer_free(struct k_timer *timer)
{
	k_timer_stop(timer);
	k_sched_lock();
	sys_dlist_append(&_timer_pool, &timer->timeout.node);
	k_sched_unlock();
}

/**
 *
 * @brief Check if the timer pool is empty
 *
 * @return true if the timer pool is empty, false otherwise
 */
bool _k_timer_pool_is_empty(void)
{
	k_sched_lock();

	bool is_empty = sys_dlist_is_empty(&_timer_pool);

	k_sched_unlock();
	return is_empty;
}

SYS_INIT(init_dyamic_timers, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* (CONFIG_NUM_DYNAMIC_TIMERS > 0) */

/**
 *
 * @brief Start timer
 *
 * @param timer     Timer structure
 * @param duration  Initial timer duration (ns)
 * @param period    Timer period (ns)
 * @param sem       Semaphore to signal timer expiration
 *
 * @return N/A
 */
void k_timer_start(struct k_timer *timer, int32_t duration, int32_t period,
		   void (*handler)(void *), void *handler_arg,
		   void (*stop_handler)(void *), void *stop_handler_arg)
{
	__ASSERT(duration >= 0 && period >= 0 &&
		 (duration != 0 || period != 0), "invalid parameters\n");

	unsigned int key = irq_lock();

	if (timer->timeout.delta_ticks_from_prev != -1) {
		_abort_timeout(&timer->timeout);
	}

	timer->period = _ms_to_ticks(period);

	timer->handler = handler;
	timer->handler_arg = handler_arg;
	timer->stop_handler = stop_handler;
	timer->stop_handler_arg = stop_handler_arg;

	_add_timeout(NULL, &timer->timeout, &timer->wait_q,
			_ms_to_ticks(duration));
	irq_unlock(key);
}


/**
 *
 * @brief Restart timer with new parameters
 *
 * @param timer     Timer structure
 * @param duration  Initial timer duration (ns)
 * @param period    Timer period (ns)
 *
 * @return N/A
 */
void k_timer_restart(struct k_timer *timer, int32_t duration, int32_t period)
{
	k_timer_start(timer, duration, period,
		      timer->handler, timer->handler_arg,
		      timer->stop_handler, timer->stop_handler_arg);
}


/**
 *
 * @brief Stop the timer
 *
 * @param timer  Timer structure
 *
 * @return N/A
 */
void k_timer_stop(struct k_timer *timer)
{
	__ASSERT(!_is_in_isr(), "");

	int key = irq_lock();

	_abort_timeout(&timer->timeout);

	irq_unlock(key);

	if (timer->stop_handler) {
		timer->stop_handler(timer->stop_handler_arg);
	}

	key = irq_lock();

	struct k_thread *pending_thread = _unpend_first_thread(&timer->wait_q);

	if (pending_thread) {
		_set_thread_return_value(pending_thread, -ECANCELED);
		_ready_thread(pending_thread);
	}

	_reschedule_threads(key);
}


/**
 *
 * @brief Test the timer for expiration
 *
 * The routine checks if the timer is expired and returns the pointer
 * to user data. Otherwise makes the thread wait for the timer expiration.
 *
 * @param timer  Timer structure
 * @param data   User data pointer
 * @param wait   May be K_NO_WAIT or K_FOREVER
 *
 * @return 0 or error code
 */
int k_timer_test(struct k_timer *timer, void **user_data_ptr, int wait)
{
	int result = 0;
	unsigned int key = irq_lock();

	/* check if the timer has expired */
	if (timer->timeout.delta_ticks_from_prev == -1) {
		*user_data_ptr = timer->user_data;
		timer->user_data = NULL;
	} else if (wait == K_NO_WAIT) {
		/* if the thread should not wait, return immediately */
		*user_data_ptr = NULL;
		result = -EAGAIN;
	} else {
		/* otherwise pend the thread */
		_pend_current_thread(&timer->wait_q, K_FOREVER);
		result = _Swap(key);
		key = irq_lock();
		if (result == 0) {
			*user_data_ptr = timer->user_data;
			timer->user_data = NULL;
		}
	}

	irq_unlock(key);
	return result;
}


/**
 *
 * @brief Get timer remaining time
 *
 * @param timer  Timer descriptor
 *
 * @return remaining time (ns)
 */

int32_t k_timer_remaining_get(struct k_timer *timer)
{
	unsigned int key = irq_lock();
	int32_t remaining_ticks;
	sys_dlist_t *timeout_q = &_nanokernel.timeout_q;

	if (timer->timeout.delta_ticks_from_prev == -1) {
		remaining_ticks = 0;
	} else {
		/*
		 * As nanokernel timeouts are stored in a linked list with
		 * delta_ticks_from_prev, to get the actual number of ticks
		 * remaining for the timer, walk through the timeouts list
		 * and accumulate all the delta_ticks_from_prev values up to
		 * the timer.
		 */
		struct _timeout *t =
			(struct _timeout *)sys_dlist_peek_head(timeout_q);

		remaining_ticks = t->delta_ticks_from_prev;
		while (t != &timer->timeout) {
			t = (struct _timeout *)sys_dlist_peek_next(timeout_q,
								   &t->node);
			remaining_ticks += t->delta_ticks_from_prev;
		}
	}

	irq_unlock(key);
	return _ticks_to_ms(remaining_ticks);
}
