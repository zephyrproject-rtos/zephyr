/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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
#include <init.h>
#include <ksched.h>
#include <wait_q.h>
#include <misc/__assert.h>
#include <misc/util.h>

void _legacy_sleep(int32_t ticks)
{
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(ticks != TICKS_UNLIMITED, "");

	if (ticks <= 0) {
		k_yield();
		return;
	}

	int key = irq_lock();

	_mark_thread_as_timing(_current);
	_remove_thread_from_ready_q(_current);
	_add_thread_timeout(_current, NULL, ticks);

	_Swap(key);
}

#if (CONFIG_NUM_DYNAMIC_TIMERS > 0)

static struct k_timer dynamic_timers[CONFIG_NUM_DYNAMIC_TIMERS];
static sys_dlist_t timer_pool;

static void timer_sem_give(struct k_timer *timer)
{
	k_sem_give((ksem_t)timer->_legacy_data);
}

static int init_dyamic_timers(struct device *dev)
{
	ARG_UNUSED(dev);

	int i;
	int n_timers = ARRAY_SIZE(dynamic_timers);

	sys_dlist_init(&timer_pool);
	for (i = 0; i < n_timers; i++) {
		k_timer_init(&dynamic_timers[i], timer_sem_give, NULL);
		sys_dlist_append(&timer_pool,
				 &dynamic_timers[i].timeout.node);
	}
	return 0;
}

SYS_INIT(init_dyamic_timers, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

ktimer_t task_timer_alloc(void)
{
	_sched_lock();

	/*
	 * This conversion works only if timeout member
	 * variable is the first in time structure.
	 */
	struct k_timer *timer = (struct k_timer *)sys_dlist_get(&timer_pool);

	k_sched_unlock();
	return timer;
}

void task_timer_free(ktimer_t timer)
{
	k_timer_stop(timer);
	_sched_lock();
	sys_dlist_append(&timer_pool, &timer->timeout.node);
	k_sched_unlock();
}

void task_timer_start(ktimer_t timer, int32_t duration,
		      int32_t period, ksem_t sema)
{
	if (duration < 0 || period < 0 || (duration == 0 && period == 0)) {
		k_timer_stop(timer);
		return;
	}

	timer->_legacy_data = (void *)sema;

	k_timer_start(timer, _ticks_to_ms(duration), _ticks_to_ms(period));
}

bool _timer_pool_is_empty(void)
{
	_sched_lock();

	bool is_empty = sys_dlist_is_empty(&timer_pool);

	k_sched_unlock();
	return is_empty;
}

#endif /* (CONFIG_NUM_DYNAMIC_TIMERS > 0) */

void *nano_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks)
{
	uint32_t (*test_fn)(struct k_timer *timer);

	if (timeout_in_ticks == TICKS_NONE) {
		test_fn = k_timer_status_get;
	} else {
		test_fn = k_timer_status_sync;
	}
	return test_fn(timer) ? timer->_legacy_data : NULL;
}
