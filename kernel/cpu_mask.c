/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/spinlock.h>

extern struct k_spinlock _sched_spinlock;


# ifdef CONFIG_SMP
/* Right now we use a two byte for this mask */
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 16, "Too many CPUs for mask word");
# endif


static int cpu_mask_mod(k_tid_t thread, uint32_t enable_mask, uint32_t disable_mask)
{
	int ret = 0;

#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	__ASSERT(z_is_thread_prevented_from_running(thread),
		 "Running threads cannot change CPU pin");
#endif

	K_SPINLOCK(&_sched_spinlock) {
		if (z_is_thread_prevented_from_running(thread)) {
			thread->base.cpu_mask |= enable_mask;
			thread->base.cpu_mask  &= ~disable_mask;
		} else {
			ret = -EINVAL;
		}
	}

#if defined(CONFIG_ASSERT) && defined(CONFIG_SCHED_CPU_MASK_PIN_ONLY)
		int m = thread->base.cpu_mask;

		__ASSERT((m == 0) || ((m & (m - 1)) == 0),
			 "Only one CPU allowed in mask when PIN_ONLY");
#endif

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

int k_thread_cpu_pin(k_tid_t thread, int cpu)
{
	int ret;

	ret = k_thread_cpu_mask_clear(thread);
	if (ret == 0) {
		return k_thread_cpu_mask_enable(thread, cpu);
	}
	return ret;
}
