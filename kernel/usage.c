/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <timing/timing.h>
#include <ksched.h>
#include <spinlock.h>

/* Need one of these for this to work */
#if !defined(CONFIG_USE_SWITCH) && !defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
#error "No data backend configured for CONFIG_SCHED_THREAD_USAGE"
#endif

static struct k_spinlock usage_lock;

static uint32_t usage_now(void)
{
	uint32_t now;

#ifdef CONFIG_THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS
	now = (uint32_t)timing_counter_get();
#else
	now = k_cycle_get_32();
#endif

	/* Edge case: we use a zero as a null ("stop() already called") */
	return (now == 0) ? 1 : now;
}

void z_sched_usage_start(struct k_thread *thread)
{
	/* One write through a volatile pointer doesn't require
	 * synchronization as long as _usage() treats it as volatile
	 * (we can't race with _stop() by design).
	 */

	_current_cpu->usage0 = usage_now();
}

void z_sched_usage_stop(void)
{
	k_spinlock_key_t k = k_spin_lock(&usage_lock);
	uint32_t u0 = _current_cpu->usage0;

	if (u0 != 0) {
		uint32_t dt = usage_now() - u0;

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
		if (z_is_idle_thread_object(_current)) {
			_kernel.idle_thread_usage += dt;
		} else {
			_kernel.all_thread_usage += dt;
		}
#endif
		_current->base.usage += dt;
	}

	_current_cpu->usage0 = 0;
	k_spin_unlock(&usage_lock, k);
}

uint64_t z_sched_thread_usage(struct k_thread *thread)
{
	k_spinlock_key_t k = k_spin_lock(&usage_lock);
	uint32_t u0 = _current_cpu->usage0, now = usage_now();
	uint64_t ret = thread->base.usage;

	if (u0 != 0) {
		uint32_t dt = now - u0;

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
		if (z_is_idle_thread_object(thread)) {
			_kernel.idle_thread_usage += dt;
		} else {
			_kernel.all_thread_usage += dt;
		}
#endif

		ret += dt;
		thread->base.usage = ret;
		_current_cpu->usage0 = now;
	}

	k_spin_unlock(&usage_lock, k);
	return ret;
}
