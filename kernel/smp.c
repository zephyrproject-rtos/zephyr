/* Copyright (c) 2022 Intel corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel/smp.h>
#include <zephyr/spinlock.h>
#include <kswap.h>
#include <kernel_internal.h>

static atomic_t global_lock;
static atomic_t cpu_start_flag;
static atomic_t ready_flag;

/**
 * Struct holding the function to be called before handing off
 * to schedule and its argument.
 */
static struct cpu_start_cb {
	/**
	 * Function to be called before handing off to scheduler.
	 * Can be NULL.
	 */
	smp_init_fn fn;

	/** Argument to @ref cpu_start_fn.fn. */
	void *arg;
} cpu_start_fn;

static struct k_spinlock cpu_start_lock;

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
	if (_current->base.global_lock_count != 0U) {
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

static void wait_for_start_signal(atomic_t *start_flag)
{
	/* Wait for the signal to begin scheduling */
	while (!atomic_get(start_flag)) {
		local_delay();
	}
}

/* Legacy interfaces for early-version SOF CPU bringup.  To be removed */
#ifdef CONFIG_SOF
void z_smp_thread_init(void *arg, struct k_thread *thread)
{
	z_dummy_thread_init(thread);
	wait_for_start_signal(arg);
}
void z_smp_thread_swap(void)
{
	z_swap_unlocked();
}
#endif

static inline FUNC_NORETURN void smp_init_top(void *arg)
{
	struct k_thread dummy_thread;
	struct cpu_start_cb *csc = arg;

	(void)atomic_set(&ready_flag, 1);

	wait_for_start_signal(&cpu_start_flag);
	z_dummy_thread_init(&dummy_thread);
#ifdef CONFIG_SYS_CLOCK_EXISTS
	smp_timer_init();
#endif

	/* Do additional initialization steps if needed. */
	if ((csc != NULL) && (csc->fn != NULL)) {
		csc->fn(csc->arg);
	}

	z_swap_unlocked();

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

static void start_cpu(int id, struct cpu_start_cb *csc)
{
	z_init_cpu(id);
	(void)atomic_clear(&ready_flag);
	arch_start_cpu(id, z_interrupt_stacks[id], CONFIG_ISR_STACK_SIZE,
		       smp_init_top, csc);
	while (!atomic_get(&ready_flag)) {
		local_delay();
	}
}

void k_smp_cpu_start(int id, smp_init_fn fn, void *arg)
{
	k_spinlock_key_t key = k_spin_lock(&cpu_start_lock);

	cpu_start_fn.fn = fn;
	cpu_start_fn.arg = arg;

	(void)atomic_set(&cpu_start_flag, 1); /* async, don't care */
	start_cpu(id, &cpu_start_fn);

	k_spin_unlock(&cpu_start_lock, key);
}

void z_smp_init(void)
{
	(void)atomic_clear(&cpu_start_flag);

	unsigned int num_cpus = arch_num_cpus();

	for (int i = 1; i < num_cpus; i++) {
		start_cpu(i, NULL);
	}
	(void)atomic_set(&cpu_start_flag, 1);
}

bool z_smp_cpu_mobile(void)
{
	unsigned int k = arch_irq_lock();
	bool pinned = arch_is_in_isr() || !arch_irq_unlocked(k);

	arch_irq_unlock(k);
	return !pinned;
}
