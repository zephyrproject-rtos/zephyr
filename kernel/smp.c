/*
 * Copyright (c) 2018 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>
#include <kswap.h>
#include <kernel_internal.h>

static atomic_t global_lock;
static atomic_t start_flag;

#if CONFIG_MP_NUM_CPUS > 1
static atomic_t ready_flag;
#endif

unsigned int z_smp_global_lock(void)
{
	unsigned int key = arch_irq_lock();

	if (!_current->base.global_lock_count) {
		while (!atomic_cas(&global_lock, 0, 1)) {
		}
	}

	_current->base.global_lock_count++;

	return key;
}

void z_smp_global_unlock(unsigned int key)
{
	if (_current->base.global_lock_count) {
		_current->base.global_lock_count--;

		if (!_current->base.global_lock_count) {
			atomic_clear(&global_lock);
		}
	}

	arch_irq_unlock(key);
}

/* Called from within z_swap(), so assumes lock already held */
void z_smp_release_global_lock(struct k_thread *thread)
{
	if (!thread->base.global_lock_count) {
		atomic_clear(&global_lock);
	}
}

/* Tiny delay that relaxes bus traffic to avoid spamming a shared
 * memory bus looking at an atomic variable
 */
static inline void local_delay(void)
{
	for (volatile int i = 0; i < 1000; i++) {
	}
}

#if CONFIG_MP_NUM_CPUS > 1

void z_smp_thread_init(void *arg, struct k_thread *thread)
{
	atomic_t *cpu_start_flag = arg;

	/* Wait for the signal to begin scheduling */
	while (!atomic_get(cpu_start_flag)) {
		local_delay();
	}

	z_dummy_thread_init(thread);
}

void z_smp_thread_swap(void)
{
	z_swap_unlocked();
}

static inline FUNC_NORETURN void smp_init_top(void *arg)
{
	struct k_thread dummy_thread;

	(void)atomic_set(&ready_flag, 1);
	z_smp_thread_init(arg, &dummy_thread);
	smp_timer_init();
	z_swap_unlocked();

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

void z_smp_start_cpu(int id)
{
	(void)atomic_clear(&start_flag);
	arch_start_cpu(id, z_interrupt_stacks[id], CONFIG_ISR_STACK_SIZE,
		       smp_init_top, &start_flag);
	(void)atomic_set(&start_flag, 1);
}

#endif

void z_smp_init(void)
{
	(void)atomic_clear(&start_flag);

#if !defined(CONFIG_SMP_BOOT_DELAY) && (CONFIG_MP_NUM_CPUS > 1)
	for (int i = 1; i < CONFIG_MP_NUM_CPUS; i++) {
		(void)atomic_clear(&ready_flag);
		arch_start_cpu(i, z_interrupt_stacks[i], CONFIG_ISR_STACK_SIZE,
			       smp_init_top, &start_flag);
		while (!atomic_get(&ready_flag)) {
			local_delay();
		}
	}
#endif

	(void)atomic_set(&start_flag, 1);
}

bool z_smp_cpu_mobile(void)
{
	unsigned int k = arch_irq_lock();
	bool pinned = arch_is_in_isr() || !arch_irq_unlocked(k);

	arch_irq_unlock(k);
	return !pinned;
}
