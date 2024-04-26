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

/**
 * Flag to tell recently powered up CPU to start
 * initialization routine.
 *
 * 0 to tell powered up CPU to wait.
 * 1 to tell powered up CPU to continue initialization.
 */
static atomic_t cpu_start_flag;

/**
 * Flag to tell caller that the target CPU is now
 * powered up and ready to be initialized.
 *
 * 0 if target CPU is not yet ready.
 * 1 if target CPU has powered up and ready to be initialized.
 */
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

	/** Invoke scheduler after CPU has started if true. */
	bool invoke_sched;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	/** True if smp_timer_init() needs to be called. */
	bool reinit_timer;
#endif /* CONFIG_SYS_CLOCK_EXISTS */
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

static inline void smp_init_top(void *arg)
{
	struct cpu_start_cb csc = arg ? *(struct cpu_start_cb *)arg : (struct cpu_start_cb){0};

	/* Let start_cpu() know that this CPU has powered up. */
	(void)atomic_set(&ready_flag, 1);

	/* Wait for the CPU start caller to signal that
	 * we can start initialization.
	 */
	wait_for_start_signal(&cpu_start_flag);

	if ((arg == NULL) || csc.invoke_sched) {
		/* Initialize the dummy thread struct so that
		 * the scheduler can schedule actual threads to run.
		 */
		z_dummy_thread_init(&_thread_dummies[arch_curr_cpu()->id]);
	}

#ifdef CONFIG_SYS_CLOCK_EXISTS
	if ((arg == NULL) || csc.reinit_timer) {
		smp_timer_init();
	}
#endif /* CONFIG_SYS_CLOCK_EXISTS */

	/* Do additional initialization steps if needed. */
	if (csc.fn != NULL) {
		csc.fn(csc.arg);
	}

	if ((arg != NULL) && !csc.invoke_sched) {
		/* Don't invoke scheduler. */
		return;
	}

	/* Let scheduler decide what thread to run next. */
	z_swap_unlocked();

	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}

static void start_cpu(int id, struct cpu_start_cb *csc)
{
	/* Clear the ready flag so the newly powered up CPU can
	 * signal that it has powered up.
	 */
	(void)atomic_clear(&ready_flag);

	/* Power up the CPU */
	arch_cpu_start(id, z_interrupt_stacks[id], CONFIG_ISR_STACK_SIZE,
		       smp_init_top, csc);

	/* Wait until the newly powered up CPU to signal that
	 * it has powered up.
	 */
	while (!atomic_get(&ready_flag)) {
		local_delay();
	}
}

void k_smp_cpu_start(int id, smp_init_fn fn, void *arg)
{
	k_spinlock_key_t key = k_spin_lock(&cpu_start_lock);

	cpu_start_fn.fn = fn;
	cpu_start_fn.arg = arg;
	cpu_start_fn.invoke_sched = true;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	cpu_start_fn.reinit_timer = true;
#endif /* CONFIG_SYS_CLOCK_EXISTS */

	/* We are only starting one CPU so we do not need to synchronize
	 * across all CPUs using the start_flag. So just set it to 1.
	 */
	(void)atomic_set(&cpu_start_flag, 1); /* async, don't care */

	/* Initialize various CPU structs related to this CPU. */
	z_init_cpu(id);

	/* Start the CPU! */
	start_cpu(id, &cpu_start_fn);

	k_spin_unlock(&cpu_start_lock, key);
}

void k_smp_cpu_resume(int id, smp_init_fn fn, void *arg,
		      bool reinit_timer, bool invoke_sched)
{
	k_spinlock_key_t key = k_spin_lock(&cpu_start_lock);

	cpu_start_fn.fn = fn;
	cpu_start_fn.arg = arg;
	cpu_start_fn.invoke_sched = invoke_sched;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	cpu_start_fn.reinit_timer = reinit_timer;
#else
	ARG_UNUSED(reinit_timer);
#endif /* CONFIG_SYS_CLOCK_EXISTS */

	/* We are only starting one CPU so we do not need to synchronize
	 * across all CPUs using the start_flag. So just set it to 1.
	 */
	(void)atomic_set(&cpu_start_flag, 1);

	/* Start the CPU! */
	start_cpu(id, &cpu_start_fn);

	k_spin_unlock(&cpu_start_lock, key);
}

void z_smp_init(void)
{
	/* We are powering up all CPUs and we want to synchronize their
	 * entry into scheduler. So set the start flag to 0 here.
	 */
	(void)atomic_clear(&cpu_start_flag);

	/* Just start CPUs one by one. */
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 1; i < num_cpus; i++) {
		z_init_cpu(i);
		start_cpu(i, NULL);
	}

	/* Let loose those CPUs so they can start scheduling
	 * threads to run.
	 */
	(void)atomic_set(&cpu_start_flag, 1);
}

bool z_smp_cpu_mobile(void)
{
	unsigned int k = arch_irq_lock();
	bool pinned = arch_is_in_isr() || !arch_irq_unlocked(k);

	arch_irq_unlock(k);
	return !pinned;
}
