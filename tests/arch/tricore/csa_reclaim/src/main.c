/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TriCore CSA save/reclaim on thread termination: ISR self-abort and
 * double-abort of a dead thread. See README.rst.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define STACK_SIZE 2048
#define NVICT      4

static struct k_thread vict[NVICT];
static K_THREAD_STACK_ARRAY_DEFINE(vstk, NVICT, STACK_SIZE);

static volatile bool run;
static atomic_t killed_isr;
static atomic_t iterations;

/* Recurse and unwind so a preempted or aborted thread has a deep, constantly
 * recycled CSA chain.
 */
static uint32_t churn(volatile int d)
{
	volatile uint32_t buf[6];

	buf[0] = d;
	buf[1] = (d > 0) ? churn(d - 1) : 0;
	return buf[0] + buf[1];
}

static bool is_victim(k_tid_t t)
{
	for (int i = 0; i < NVICT; i++) {
		if (&vict[i] == t) {
			return true;
		}
	}
	return false;
}

/* Runs in the system-clock ISR; aborts the running victim it interrupted. */
static void tick_cb(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_tid_t cur = k_current_get();

	if (run && is_victim(cur)) {
		atomic_inc(&killed_isr);
		k_thread_abort(cur);
	}
}

K_TIMER_DEFINE(kill_timer, tick_cb, NULL);

static void churn_victim(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		(void)churn(10);
		atomic_inc(&iterations);
	}
}

ZTEST(tricore_csa_reclaim, test_isr_self_abort_churn)
{
	run = true;
	atomic_clear(&killed_isr);
	atomic_clear(&iterations);

	for (int i = 0; i < NVICT; i++) {
		k_thread_create(&vict[i], vstk[i], STACK_SIZE, churn_victim, NULL, NULL, NULL, 6, 0,
				K_NO_WAIT);
	}
	k_timer_start(&kill_timer, K_MSEC(1), K_MSEC(1));

	for (int s = 0; s < 12; s++) {
		k_msleep(250);
		/* Respawn victims the ISR has aborted, keeping aborts back-to-back. */
		for (int i = 0; i < NVICT; i++) {
			if (k_thread_join(&vict[i], K_NO_WAIT) == 0) {
				k_thread_create(&vict[i], vstk[i], STACK_SIZE, churn_victim, NULL,
						NULL, NULL, 6, 0, K_NO_WAIT);
			}
		}
	}

	run = false;
	k_timer_stop(&kill_timer);
	for (int i = 0; i < NVICT; i++) {
		k_thread_abort(&vict[i]);
	}

	zassert_true(atomic_get(&killed_isr) > 10, "too few ISR self-aborts: %ld",
		     (long)atomic_get(&killed_isr));
	printk("ISR self-abort churn survived: killed_isr=%ld\n", (long)atomic_get(&killed_isr));
}

static void short_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	(void)churn(8);
}

static void selfabort_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	(void)churn(8);
	k_thread_abort(k_current_get());
}

ZTEST(tricore_csa_reclaim, test_double_abort_dead)
{
	for (int round = 0; round < 200; round++) {
		/* Rotate the death mode: natural, self-abort, cross-abort. */
		int mode = round % 3;

		if (mode == 2) {
			k_thread_create(&vict[0], vstk[0], STACK_SIZE, churn_victim, NULL, NULL,
					NULL, 6, 0, K_NO_WAIT);
			k_msleep(1);
			k_thread_abort(&vict[0]);
		} else {
			k_thread_create(&vict[0], vstk[0], STACK_SIZE,
					mode == 0 ? short_thread : selfabort_thread, NULL, NULL,
					NULL, 6, 0, K_NO_WAIT);
			zassert_equal(k_thread_join(&vict[0], K_MSEC(1000)), 0, "round %d join",
				      round);
		}

		/* Abort the already-dead thread again; must be a no-op. */
		k_thread_abort(&vict[0]);
		k_thread_abort(&vict[0]);

		/* Churn CSAs so any free-list damage faults here. */
		for (int i = 1; i < NVICT; i++) {
			k_thread_create(&vict[i], vstk[i], STACK_SIZE, short_thread, NULL, NULL,
					NULL, 6, 0, K_NO_WAIT);
		}
		for (int i = 1; i < NVICT; i++) {
			zassert_equal(k_thread_join(&vict[i], K_MSEC(1000)), 0,
				      "round %d churn join %d", round, i);
		}
	}
	printk("double-abort survived 200 rounds x 3 modes\n");
}

ZTEST_SUITE(tricore_csa_reclaim, NULL, NULL, NULL, NULL, NULL);
