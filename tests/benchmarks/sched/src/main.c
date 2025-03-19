/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <wait_q.h>
#include <ksched.h>

/* This is a scheduler microbenchmark, designed to measure latencies
 * of specific low level scheduling primitives independent of overhead
 * from application or API abstractions.  It works very simply: a main
 * thread creates a "partner" thread at a higher priority, the partner
 * then sleeps using z_pend_curr().  From this initial
 * state:
 *
 * 1. The main thread calls z_unpend_first_thread()
 * 2. The main thread calls z_ready_thread()
 * 3. The main thread calls k_yield()
 *    (the kernel switches to the partner thread)
 * 4. The partner thread then runs and calls z_pend_curr() again
 *    (the kernel switches to the main thread)
 * 5. The main thread returns from k_yield()
 *
 * It then iterates this many times, reporting timestamp latencies
 * between each numbered step and for the whole cycle, and a running
 * average for all cycles run.
 */

#define N_RUNS 1000
#define N_SETTLE 10


static K_THREAD_STACK_DEFINE(partner_stack, 1024);
static struct k_thread partner_thread;

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
static struct k_thread busy_thread[CONFIG_MP_MAX_NUM_CPUS - 1];

#define BUSY_THREAD_STACK_SIZE  (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_ARRAY_DEFINE(busy_thread_stack, CONFIG_MP_MAX_NUM_CPUS - 1,
				   BUSY_THREAD_STACK_SIZE);
#endif /* (CONFIG_MP_MAX_NUM_CPUS > 1) */

_wait_q_t waitq;

enum {
	UNPENDING,
	UNPENDED_READYING,
	READIED_YIELDING,
	PARTNER_AWAKE_PENDING,
	YIELDED,
	NUM_STAMP_STATES
};

uint32_t stamps[NUM_STAMP_STATES];

static struct k_spinlock lock;

static inline int _stamp(int state)
{
	uint32_t t;

	/* In theory the TSC has much lower overhead and higher
	 * precision.  In practice it's VERY jittery in recent qemu
	 * versions and frankly too noisy to trust.
	 */
#ifdef CONFIG_X86
	__asm__ volatile("rdtsc" : "=a"(t) : : "edx");
#else
	t = k_cycle_get_32();
#endif

	stamps[state] = t;
	return t;
}

/* #define stamp(s) printk("%s @ %d\n", #s, _stamp(s)) */
#define stamp(s) _stamp(s)

static void partner_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	printk("Running %p\n", k_current_get());

	while (true) {
		k_spinlock_key_t  key = k_spin_lock(&lock);

		z_pend_curr(&lock, key, &waitq, K_FOREVER);
		stamp(PARTNER_AWAKE_PENDING);
	}
}

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
static void busy_thread_entry(void *arg1, void *arg2, void *arg3)
{
	while (true) {
	}
}
#endif /* (CONFIG_MP_MAX_NUM_CPUS > 1) */

int main(void)
{
#if (CONFIG_MP_MAX_NUM_CPUS > 1)
	/* Spawn busy threads that will execute on the other cores */
	for (uint32_t i = 0; i < CONFIG_MP_MAX_NUM_CPUS - 1; i++) {
		k_thread_create(&busy_thread[i], busy_thread_stack[i],
				BUSY_THREAD_STACK_SIZE, busy_thread_entry,
				NULL, NULL, NULL,
				K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
	}
#endif /* (CONFIG_MP_MAX_NUM_CPUS > 1) */

	z_waitq_init(&waitq);

	int main_prio = k_thread_priority_get(k_current_get());
	int partner_prio = main_prio - 1;

	k_tid_t th = k_thread_create(&partner_thread, partner_stack,
				     K_THREAD_STACK_SIZEOF(partner_stack),
				     partner_fn, NULL, NULL, NULL,
				     partner_prio, 0, K_NO_WAIT);

	/* Let it start running and pend */
	k_sleep(K_MSEC(100));

	uint64_t tot = 0U;
	uint32_t runs = 0U;

	int key;

	for (int i = 0; i < N_RUNS + N_SETTLE; i++) {
		key = arch_irq_lock();
		stamp(UNPENDING);
		z_unpend_first_thread(&waitq);
		arch_irq_unlock(key);
		stamp(UNPENDED_READYING);
		z_ready_thread(th);
		stamp(READIED_YIELDING);

		/* z_ready_thread() does not reschedule, so this is
		 * guaranteed to be the point where we will yield to
		 * the new thread, which (being higher priority) will
		 * run immediately, and we'll wake up synchronously as
		 * soon as it pends.
		 */
		k_yield();
		stamp(YIELDED);

		uint32_t avg, whole = stamps[4] - stamps[0];

		if (++runs > N_SETTLE) {
			/* Only compute averages after the first ~10
			 * runs to let performance settle, cache
			 * effects in the host pollute the early
			 * data
			 */
			tot += whole;
			avg = tot / (runs - 10);
		} else {
			tot = 0U;
			avg = 0U;
		}

		/* For reference, an unmodified HEAD on qemu_x86 with
		 * !USERSPACE and SCHED_SIMPLE and using -icount
		 * shift=0,sleep=off,align=off, I get results of:
		 *
		 * unpend 132 ready 257 switch 278 pend 321 tot 988 (avg 900)
		 */
		printk("unpend %4d ready %4d switch %4d pend %4d tot %4d (avg %4d)\n",
		       stamps[1] - stamps[0],
		       stamps[2] - stamps[1],
		       stamps[3] - stamps[2],
		       stamps[4] - stamps[3],
		       whole, avg);
	}
	printk("fin\n");
	return 0;
}
