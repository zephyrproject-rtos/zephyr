/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TriCore CSA reclaim under free-list (FCX) depletion. See README.rst.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/arch/tricore/cr.h>

#define STACK_SIZE 2048
#define ROUNDS     120
#define MARGIN     20 /* stop this many CSAs before FCD (LCX reserve is 8) */
#define MAXDEPTH   200

static struct k_thread hold_thr;
static K_THREAD_STACK_DEFINE(hold_stk, STACK_SIZE);
static struct k_thread canary_thr;
static K_THREAD_STACK_DEFINE(canary_stk, STACK_SIZE);

static struct k_sem hold_sem;
static volatile bool run_canary;
static atomic_t canary_ok;
static atomic_t canary_bad;
static atomic_t min_free = ATOMIC_INIT(0x7fffffff);

/* CSAs available before the FCD limit (LCX offset - FCX offset). */
static int free_before_fcd(void)
{
	int fcx = (int)(cr_read(TRICORE_FCX) & 0xFFFF);
	int lcx = (int)(cr_read(TRICORE_LCX) & 0xFFFF);

	return lcx - fcx;
}

static void deep_hold(volatile int d)
{
	volatile uint32_t pad[2];
	int f = free_before_fcd(); /* per-frame side effect: forces a CALL */

	pad[0] = (uint32_t)f;
	if (f > MARGIN && d < MAXDEPTH) {
		deep_hold(d + 1);
	} else {
		if (f < atomic_get(&min_free)) {
			atomic_set(&min_free, f);
		}
		k_sem_take(&hold_sem, K_FOREVER);
	}
	pad[1] = pad[0];
	(void)pad[1];
}

static void holder(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	deep_hold(0);
}

static uint32_t sum_to(volatile uint32_t n)
{
	volatile uint32_t acc = 0;

	for (volatile uint32_t i = 1; i <= n; i++) {
		acc += i;
	}
	return acc;
}

/* Fixed-result work; a wrong sum means a reclaim corrupted this context. */
static void canary(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	while (run_canary) {
		atomic_inc(sum_to(100) == 5050U ? &canary_ok : &canary_bad);
		k_yield();
	}
}

ZTEST(tricore_fcx_depletion, test_reclaim_near_depletion)
{
	k_sem_init(&hold_sem, 0, 2);
	run_canary = true;
	atomic_clear(&canary_ok);
	atomic_clear(&canary_bad);
	atomic_set(&min_free, 0x7fffffff);

	k_thread_create(&canary_thr, canary_stk, STACK_SIZE, canary, NULL, NULL, NULL, 8, 0,
			K_NO_WAIT);

	for (int r = 0; r < ROUNDS; r++) {
		k_thread_create(&hold_thr, hold_stk, STACK_SIZE, holder, NULL, NULL, NULL, 6, 0,
				K_NO_WAIT);
		k_msleep(3); /* holder recurses to margin, blocks */

		k_thread_abort(&hold_thr); /* reclaim deep chain under low FCX */
		k_thread_abort(&hold_thr); /* dead thread; must be a no-op */
	}

	run_canary = false;
	zassert_equal(k_thread_join(&canary_thr, K_MSEC(1000)), 0, NULL);

	printk("done min_free=%ld canary_ok=%ld canary_bad=%ld\n", (long)atomic_get(&min_free),
	       (long)atomic_get(&canary_ok), (long)atomic_get(&canary_bad));
	zassert_equal(atomic_get(&canary_bad), 0, "canary corrupted %ld times",
		      (long)atomic_get(&canary_bad));
	zassert_true(atomic_get(&min_free) <= MARGIN + 3,
		     "did not approach depletion (min_free=%ld)", (long)atomic_get(&min_free));
}

ZTEST_SUITE(tricore_fcx_depletion, NULL, NULL, NULL, NULL, NULL);
