/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <kspinlock.h>
#include <ksched.h>
#include <zephyr/spinlock.h>

# ifdef CONFIG_SMP
/* The mask width scales with CONFIG_MP_MAX_NUM_CPUS, up to 32 bits */
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 32, "Too many CPUs for mask word");
# endif /* CONFIG_SMP */


static int cpu_mask_mod(k_tid_t thread, uint32_t enable_mask, uint32_t disable_mask)
{
	int ret = 0;

#ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
	__ASSERT(z_is_thread_prevented_from_running(thread),
		 "Running threads cannot change CPU pin");
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */

	Z_SCHED_SPINLOCK {
		if (z_is_thread_prevented_from_running(thread)) {
			thread->base.cpu_mask |= enable_mask;
			thread->base.cpu_mask  &= ~disable_mask;

#if defined(CONFIG_ASSERT) && defined(CONFIG_SCHED_CPU_MASK_PIN_ONLY)
			uint32_t m = thread->base.cpu_mask;

			__ASSERT(m != 0 && (m & (m - 1)) == 0,
				 "PIN_ONLY requires exactly one CPU in mask");
#endif /* defined(CONFIG_ASSERT) && defined(CONFIG_SCHED_CPU_MASK_PIN_ONLY) */

		} else {
			ret = -EINVAL;
		}
	}

	return ret;
}

#ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
static bool cpu_range_is_valid(int cpu_first, int cpu_last)
{
	return IN_RANGE(cpu_first, 0, (int)arch_num_cpus() - 1) &&
	       IN_RANGE(cpu_last, cpu_first, (int)arch_num_cpus() - 1);
}
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */

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
	if (!IN_RANGE(cpu, 0, (int)arch_num_cpus() - 1)) {
		return -EINVAL;
	}

	return cpu_mask_mod(thread, BIT(cpu), 0);
}

int k_thread_cpu_mask_disable(k_tid_t thread, int cpu)
{
	if (!IN_RANGE(cpu, 0, (int)arch_num_cpus() - 1)) {
		return -EINVAL;
	}

	return cpu_mask_mod(thread, 0, BIT(cpu));
}

#ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
int k_thread_cpu_mask_enable_range(k_tid_t thread, int cpu_first, int cpu_last)
{
	if (!cpu_range_is_valid(cpu_first, cpu_last)) {
		return -EINVAL;
	}

	return cpu_mask_mod(thread, CPU_MASK_CPU_RANGE(cpu_first, cpu_last), 0);
}

int k_thread_cpu_mask_disable_range(k_tid_t thread, int cpu_first, int cpu_last)
{
	if (!cpu_range_is_valid(cpu_first, cpu_last)) {
		return -EINVAL;
	}

	return cpu_mask_mod(thread, 0, CPU_MASK_CPU_RANGE(cpu_first, cpu_last));
}
#endif /* CONFIG_SCHED_CPU_MASK_PIN_ONLY */

int k_thread_cpu_pin(k_tid_t thread, int cpu)
{
	uint32_t mask;

	if (!IN_RANGE(cpu, 0, (int)arch_num_cpus() - 1)) {
		return -EINVAL;
	}

	mask = BIT(cpu);

	return cpu_mask_mod(thread, mask, ~mask);
}
