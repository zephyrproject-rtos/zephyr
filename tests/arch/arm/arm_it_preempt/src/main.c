/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * An interrupt taken inside an IT block saves the IT state in the
 * EPSR bits of the stacked xPSR, and only an exception return can
 * restore it.  This test has a timer interrupt wake a higher priority
 * thread while a lower priority thread runs IT blocks, so the lower
 * priority thread is switched out on interrupt exit and switched back
 * in when the other thread blocks on its semaphore.  If the IT state
 * is lost on the way back, the rest of the block runs unconditionally
 * and the else-branch adds, which must never execute, change r1.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define WAKES   2000U
#define LOOPS   1000U
#define STACKSZ (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(lo_stack, STACKSZ);
static K_THREAD_STACK_DEFINE(hi_stack, STACKSZ);
static struct k_thread lo_thread, hi_thread;
static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(done_sem, 0, 1);
static volatile uint32_t wakes, passes, bad_passes;

static void lo_fn(void *p1, void *p2, void *p3)
{
	while (wakes < WAKES) {
		uint32_t r0 = 0, r1 = 0, n = LOOPS, z = 0;

		__asm__ volatile("1: cmp %[z], #0;"
				 "itete eq;"
				 "addeq %[r0], %[r0], #1;"
				 "addne %[r1], %[r1], #1;"
				 "addeq %[r0], %[r0], #1;"
				 "addne %[r1], %[r1], #1;"
				 "subs %[n], %[n], #1;"
				 "bne 1b;"
				 : [r0] "+r"(r0), [r1] "+r"(r1), [n] "+r"(n)
				 : [z] "r"(z)
				 : "cc");
		if (r0 != 2U * LOOPS || r1 != 0U) {
			bad_passes++;
		}
		passes++;
	}
	k_sem_give(&done_sem);
}

static void hi_fn(void *p1, void *p2, void *p3)
{
	for (;;) {
		k_sem_take(&wake_sem, K_FOREVER);
		wakes++;
	}
}

static void timer_fn(struct k_timer *timer)
{
	k_sem_give(&wake_sem);
}

static K_TIMER_DEFINE(wake_timer, timer_fn, NULL);

ZTEST(arm_it_preempt, test_it_block_preempt)
{
	int prio = k_thread_priority_get(k_current_get());

	k_thread_create(&hi_thread, hi_stack, STACKSZ, hi_fn, NULL, NULL, NULL, prio + 1, 0,
			K_NO_WAIT);
	k_thread_create(&lo_thread, lo_stack, STACKSZ, lo_fn, NULL, NULL, NULL, prio + 2, 0,
			K_NO_WAIT);
	k_timer_start(&wake_timer, K_TICKS(1), K_TICKS(1));

	int ret = k_sem_take(&done_sem, K_SECONDS(60));

	k_timer_stop(&wake_timer);
	k_thread_abort(&hi_thread);
	k_thread_abort(&lo_thread);

	zassert_ok(ret, "only %u wakeups in 60 seconds", wakes);
	zassert_equal(bad_passes, 0, "%u of %u passes lost the IT state", bad_passes, passes);
}

ZTEST_SUITE(arm_it_preempt, NULL, NULL, NULL, NULL, NULL);
