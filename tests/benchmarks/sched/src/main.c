/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <wait_q.h>
#include <ksched.h>

/* This is a scheduler microbenchmark, designed to measure latencies
 * of specific low level scheduling primitives independent of overhead
 * from application or API abstractions.  It works very simply: a main
 * thread creates a "partner" thread at a higher priority, the partner
 * then sleeps using z_pend_curr_irqlock().  From this initial
 * state:
 *
 * 1. The main thread calls z_unpend_first_thread()
 * 2. The main thread calls z_ready_thread()
 * 3. The main thread calls k_yield()
 *    (the kernel switches to the partner thread)
 * 4. The partner thread then runs and calls z_pend_curr_irqlock() again
 *    (the kernel switches to the main thread)
 * 5. The main thread returns from k_yield()
 *
 * It then iterates this many times, reporting timestamp latencies
 * between each numbered step and for the whole cycle, and a running
 * average for all cycles run.
 *
 * Note that because this involves no timer interaction (except, on
 * some architectures, k_cycle_get_32()), it works correctly when run
 * in qemu using the -icount argument, which can produce 100%
 * deterministic behavior (not cycle-exact hardware simulation, but
 * exactly N instructions per simulated nanosecond).  You can enable
 * for "make run" using an environment variable:
 *
 * export QEMU_EXTRA_FLAGS="-icount shift=0,align=off,sleep=off"
 */

#define N_RUNS 1000
#define N_SETTLE 10


static K_THREAD_STACK_DEFINE(partner_stack, 1024);
static struct k_thread partner_thread;

_wait_q_t waitq;

enum {
	UNPENDING,
	UNPENDED_READYING,
	READIED_YIELDING,
	PARTNER_AWAKE_PENDING,
	YIELDED,
	NUM_STAMP_STATES
};

u32_t stamps[NUM_STAMP_STATES];

static inline int _stamp(int state)
{
	u32_t t;

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
		unsigned int key = irq_lock();

		z_pend_curr_irqlock(key, &waitq, K_FOREVER);
		stamp(PARTNER_AWAKE_PENDING);
	}
}

void main(void)
{
	z_waitq_init(&waitq);

	int main_prio = k_thread_priority_get(k_current_get());
	int partner_prio = main_prio - 1;

	k_tid_t th = k_thread_create(&partner_thread, partner_stack,
				     K_THREAD_STACK_SIZEOF(partner_stack),
				     partner_fn, NULL, NULL, NULL,
				     partner_prio, 0, K_NO_WAIT);

	/* Let it start running and pend */
	k_sleep(K_MSEC(100));

	u64_t tot = 0U;
	u32_t runs = 0U;

	for (int i = 0; i < N_RUNS + N_SETTLE; i++) {
		stamp(UNPENDING);
		z_unpend_first_thread(&waitq);
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

		u32_t avg, whole = stamps[4] - stamps[0];

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
		 * !USERSPACE and SCHED_DUMB and using -icount
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
}
