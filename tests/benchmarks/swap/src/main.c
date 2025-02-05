/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/* This app implements a simple microbenchmark of z_swap(), carefully
 * tuned to measure only cooperative swapping performance and nothing
 * else.  Build it and run.  No special usage needed.
 *
 * Subtle sequencing, see comments below.  This runs without a timer
 * driver (and in fact disables ARM SysTick so it can use it
 * directly), controlling execution order via scheduler priority only.
 */

#include <math.h>
#include <zephyr/kernel.h>
#include <kswap.h>

/* Use a microtuned platform timing hook if available, otherwise k_cycle_get_32(). */
#ifdef CONFIG_CPU_CORTEX_M
#include "time_arm_m.h"
#else
#include "time_generic.h"
#endif

/* Check config for obvious mistakes. */
#ifdef CONFIG_ASSERT
#warning "This is a performance benchmark, debug features should not normally be enabled"
#define ORDER_CHECK() printk("%s:%d\n", __func__, __LINE__)
#else
#define ORDER_CHECK()
#endif

/* This ends up measuring a bunch of stuff not involved in arch code */
#if defined(CONFIG_TIMESLICING)
#warning "Timeslicing can pollute the microbenchmark"
#endif

/* And these absolutely explode the numbers, making the arch layer just noise */
#if defined(CONFIG_MPU) || defined(CONFIG_MMU)
#error "Don't enable memory management hardware in a microbenchmark!"
#endif

#define HI_PRIO   0
#define LO_PRIO   1
#define MAIN_PRIO 2
#define DONE_PRIO 3

void thread_fn(void *t0_arg, void *t1_arg, void *c);

int swap_count;

/* swap enter/exit times for each thread */
int t0_0, t0_1, t1_0, t1_1;

uint32_t samples[8 * 1024];

struct k_spinlock lock;

K_SEM_DEFINE(done_sem, 0, 999);
K_THREAD_DEFINE(thread0, 1024, thread_fn, &t0_0, &t0_1, NULL, HI_PRIO, 0, 0);
K_THREAD_DEFINE(thread1, 1024, thread_fn, &t1_0, &t1_1, NULL, HI_PRIO, 0, 0);

void thread_fn(void *t0_arg, void *t1_arg, void *c)
{
	uint32_t t0, t1, *t0_out = t0_arg, *t1_out = t1_arg;

	while (true) {
		k_spinlock_key_t k = k_spin_lock(&lock);

		k_thread_priority_set(k_current_get(), DONE_PRIO);

		ORDER_CHECK();
		t0 = time();
		z_swap(&lock, k);
		t1 = time();

		*t0_out = t0;
		*t1_out = t1;
	}
}

int main(void)
{
	uint64_t total = 0, avg8, var8 = 0;
	size_t n = ARRAY_SIZE(samples);

#ifdef CONFIG_SMP
	__ASSERT(arch_num_cpus() == 1, "Test requires only one CPU be active");
#endif

	time_setup();
	k_thread_priority_set(k_current_get(), MAIN_PRIO);

	/* The threads are launched by the kernel at HI priority, so
	 * they've already run and are suspended in swap for us.
	 */
	for (int i = 0; i < n; i++) {
		k_sched_lock();

		ORDER_CHECK();
		k_thread_priority_set(thread0, HI_PRIO);
		ORDER_CHECK();
		k_thread_priority_set(thread1, LO_PRIO);
		ORDER_CHECK();

		/* Now unlock: thread0 will run first, looping around
		 * and calling z_swap, with an entry timestamp of
		 * t0_0.  That will swap to thread1 (which timestamps
		 * its exit as t1_1).  Then we end up back here.
		 */
		ORDER_CHECK();
		k_sched_unlock();

		/* And some complexity: thread1 "woke up" on our cycle
		 * and stored its exit time in t1_1.  But thread0's
		 * entry time is still a local variable suspended on
		 * its stack.  So we pump it once to get it to store
		 * its output.
		 */
		k_thread_priority_set(thread0, HI_PRIO);

		uint32_t dt = time_delta(t0_0, t1_1);

		samples[i] = dt;
		total += dt;
	}

	k_thread_abort(thread0);
	k_thread_abort(thread1);

	/* Do the variance math left-shifted by 8 bits */
	avg8 = (int)(total * 256 / n);
	for (int i = 0; i < n; i++) {
		int d = (int)(samples[i] * 256) - avg8;

		var8 += d * d;
	}

	double stdev = sqrt((var8 / 256.0) / n);
	int stdev_i = (int)stdev;
	int stdev_f = (int)(10 * (stdev - stdev_i));

	printk("Swap average %d stdev %d.%d\n", (int)((avg8 + 128) / 256), stdev_i, stdev_f);

	return 0;
}
