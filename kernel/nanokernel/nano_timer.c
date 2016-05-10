/* nano_timer.c - timer for nanokernel-only systems */

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

void nano_timer_init(struct nano_timer *timer, void *data)
{
	/* initialize timer in expired state */
	timer->timeout_data.delta_ticks_from_prev = -1;

	/* initialize to no object to wait on */
	timer->timeout_data.wait_q = NULL;

	/* initialize to no fiber waiting for the timer expire */
	timer->timeout_data.tcs = NULL;

	/* nano_timer_test() returns NULL on timer that was not started */
	timer->user_data = NULL;

	timer->user_data_backup = data;

	SYS_TRACING_OBJ_INIT(nano_timer, timer);
}


FUNC_ALIAS(_timer_start, nano_isr_timer_start, void);
FUNC_ALIAS(_timer_start, nano_fiber_timer_start, void);
FUNC_ALIAS(_timer_start, nano_task_timer_start, void);
FUNC_ALIAS(_timer_start, nano_timer_start, void);

/**
 *
 * @brief Start a nanokernel timer (generic implementation)
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in <ticks> system clock ticks.
 *
 * @param timer The Timer to start
 * @param ticks The number of system ticks before expiration
 *
 * @return N/A
 */
void _timer_start(struct nano_timer *timer, int ticks)
{
	int key = irq_lock();

	/*
	 * Once timer is started nano_timer_test() returns
	 * the pointer to user data
	 */
	timer->user_data = timer->user_data_backup;
	_nano_timer_timeout_add(&timer->timeout_data,
				NULL, ticks);
	irq_unlock(key);
}

FUNC_ALIAS(_timer_stop_non_preemptible, nano_isr_timer_stop, void);
FUNC_ALIAS(_timer_stop_non_preemptible, nano_fiber_timer_stop, void);
void _timer_stop_non_preemptible(struct nano_timer *timer)
{
	struct _nano_timeout *t = &timer->timeout_data;
	struct tcs *tcs = t->tcs;
	int key = irq_lock();

	/*
	 * Verify first if fiber is not waiting on an object,
	 * timer is not expired and there is a fiber waiting
	 * on it
	 */
	if (!t->wait_q && (_nano_timer_timeout_abort(t) == 0) &&
	    tcs != NULL) {
		if (_IS_MICROKERNEL_TASK(tcs)) {
			_NANO_TIMER_TASK_READY(tcs);
		} else {
			_nano_fiber_ready(tcs);
		}
	}

	/*
	 * After timer gets aborted nano_timer_test() should
	 * return NULL until timer gets restarted
	 */
	timer->user_data = NULL;
	irq_unlock(key);
}

#ifdef CONFIG_MICROKERNEL
extern void _task_nano_timer_task_ready(void *uk_task_ptr);

#define _TASK_NANO_TIMER_TASK_READY(tcs)  \
		_task_nano_timer_task_ready(tcs->uk_task_ptr)
#else
#define _TASK_NANO_TIMER_TASK_READY(tcs)  do { } while (0)
#endif

void nano_task_timer_stop(struct nano_timer *timer)
{
	struct _nano_timeout *t = &timer->timeout_data;
	struct tcs *tcs = t->tcs;
	int key = irq_lock();

	timer->user_data = NULL;

	/*
	 * Verify first if fiber is not waiting on an object,
	 * timer is not expired and there is a fiber waiting
	 * on it
	 */
	if (!t->wait_q && (_nano_timer_timeout_abort(t) == 0) &&
	    tcs != NULL) {
		if (!_IS_MICROKERNEL_TASK(tcs)) {
			_nano_fiber_ready(tcs);
			_Swap(key);
			return;
		}
		_TASK_NANO_TIMER_TASK_READY(tcs);
	}
	irq_unlock(key);
}

void nano_timer_stop(struct nano_timer *timer)
{
	static void (*func[3])(struct nano_timer *) = {
		nano_isr_timer_stop,
		nano_fiber_timer_stop,
		nano_task_timer_stop,
	};

	func[sys_execution_context_type_get()](timer);
}

/**
 *
 * @brief Test nano timer for cases when the calling thread does not wait
 *
 * @param timer Timer to check
 * @param timeout_in_ticks Determines the action to take when the timer has
 *        not expired.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 * @param user_data_ptr Pointer to user data if the timer is expired
 *        it's set to timer->user_data. Otherwise it's set to NULL
 *
 * @return 1 if the thread waits for timer to expire and 0 otherwise
 */

static int _nano_timer_expire_wait(struct nano_timer *timer,
				    int32_t timeout_in_ticks,
				    void **user_data_ptr)
{
	struct _nano_timeout *t = &timer->timeout_data;

	/* check if the timer has expired */
	if (t->delta_ticks_from_prev == -1) {
		*user_data_ptr = timer->user_data;
		timer->user_data = NULL;
	/* if the thread should not wait, return immediately */
	} else if (timeout_in_ticks == TICKS_NONE) {
		*user_data_ptr = NULL;
	} else {
		return 1;
	}
	return 0;
}

void *nano_isr_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks)
{
	int key = irq_lock();
	void *user_data;

	if (_nano_timer_expire_wait(timer, timeout_in_ticks, &user_data)) {
		/* since ISR can not wait, return NULL */
		user_data = NULL;
	}
	irq_unlock(key);
	return user_data;
}

void *nano_fiber_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks)
{
	int key = irq_lock();
	struct _nano_timeout *t = &timer->timeout_data;
	void *user_data;

	if (_nano_timer_expire_wait(timer, timeout_in_ticks, &user_data)) {
		t->tcs = _nanokernel.current;
		_Swap(key);
		key = irq_lock();
		user_data = timer->user_data;
		timer->user_data = NULL;
	}
	irq_unlock(key);
	return user_data;
}

#define IDLE_TASK_TIMER_PEND(timer, key) \
	do {                                                                \
		_nanokernel.task_timeout = nano_timer_ticks_remain(timer);  \
		nano_cpu_atomic_idle(key);                                  \
		key = irq_lock();                                           \
	} while (0)

#ifdef CONFIG_MICROKERNEL
extern void _task_nano_timer_pend_task(struct nano_timer *timer);

#define NANO_TASK_TIMER_PEND(timer, key)                                      \
	do {                                                                  \
		if (_IS_IDLE_TASK()) {  \
			IDLE_TASK_TIMER_PEND(timer, key);                     \
		} else {                                                      \
			_task_nano_timer_pend_task(timer);                    \
		}                                                             \
	} while (0)
#else
#define NANO_TASK_TIMER_PEND(timer, key)  IDLE_TASK_TIMER_PEND(timer, key)
#endif

void *nano_task_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks)
{
	int key = irq_lock();
	struct _nano_timeout *t = &timer->timeout_data;
	void *user_data;

	if (_nano_timer_expire_wait(timer, timeout_in_ticks, &user_data)) {
		/* task goes to busy waiting loop */
		while (t->delta_ticks_from_prev != -1) {
			NANO_TASK_TIMER_PEND(timer, key);
		}
		user_data = timer->user_data;
		timer->user_data = NULL;
	}
	irq_unlock(key);
	return user_data;
}

void *nano_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks)
{
	static void *(*func[3])(struct nano_timer *, int32_t) = {
		nano_isr_timer_test,
		nano_fiber_timer_test,
		nano_task_timer_test,
	};

	return func[sys_execution_context_type_get()](timer, timeout_in_ticks);
}

int32_t nano_timer_ticks_remain(struct nano_timer *timer)
{
	int key = irq_lock();
	int32_t remaining_ticks;
	struct _nano_timeout *t = &timer->timeout_data;
	sys_dlist_t *timeout_q = &_nanokernel.timeout_q;
	struct _nano_timeout *iterator;

	if (t->delta_ticks_from_prev == -1) {
		remaining_ticks = 0;
	} else {
		/*
		 * As nanokernel timeouts are stored in a linked list with
		 * delta_ticks_from_prev, to get the actual number of ticks
		 * remaining for the timer, walk through the timeouts list
		 * and accumulate all the delta_ticks_from_prev values up to
		 * the timer.
		 */
		iterator =
			(struct _nano_timeout *)sys_dlist_peek_head(timeout_q);
		remaining_ticks = iterator->delta_ticks_from_prev;
		while (iterator != t) {
			iterator = (struct _nano_timeout *)sys_dlist_peek_next(
				timeout_q, &iterator->node);
			remaining_ticks += iterator->delta_ticks_from_prev;
		}
	}

	irq_unlock(key);
	return remaining_ticks;
}
