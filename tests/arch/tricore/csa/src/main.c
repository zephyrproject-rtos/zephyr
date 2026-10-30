/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/tricore/cr.h>

#define WORK_STACK_SIZE 1024
#define CYCLES          200
#define WALK_LIMIT      (CONFIG_TRICORE_CSA_COUNT + 16)

static K_THREAD_STACK_DEFINE(work_stack, WORK_STACK_SIZE);
static struct k_thread work_thread;

static int csa_free_count(void)
{
	unsigned int key = irq_lock();
	uint32_t pcxi = cr_read(TRICORE_FCX) & 0xFFFFF;
	int n = 0;

	while (pcxi != 0 && n < WALK_LIMIT) {
		volatile uint32_t *csa =
			(volatile uint32_t *)(((pcxi & 0xF0000) << 12) | ((pcxi & 0xFFFF) << 6));

		pcxi = csa[0] & 0xFFFFF;
		n++;
	}

	irq_unlock(key);

	return n;
}

static void worker(void *a, void *b, void *c)
{
	volatile int x = 0;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (int i = 0; i < 64; i++) {
		x += i;
	}
}

ZTEST(tricore_csa, test_free_list_intact)
{
	int avail = csa_free_count();

	zassert_true(avail > 8, "free CSA count %d too low - chain broken?", avail);
	zassert_true(avail <= CONFIG_TRICORE_CSA_COUNT, "free CSA count %d exceeds total %d", avail,
		     CONFIG_TRICORE_CSA_COUNT);
}

ZTEST(tricore_csa, test_no_leak_on_abort)
{
	int base = csa_free_count();

	for (int i = 0; i < CYCLES; i++) {
		k_tid_t t = k_thread_create(&work_thread, work_stack, WORK_STACK_SIZE, worker, NULL,
					    NULL, NULL, K_PRIO_PREEMPT(0), 0, K_MSEC(60000));

		k_thread_abort(t);
	}

	zassert_equal(base, csa_free_count(), "CSA leak across %d create/abort cycles (base=%d)",
		      CYCLES, base);
}

static void run_exit_cycles(int n)
{
	for (int i = 0; i < n; i++) {
		k_tid_t t = k_thread_create(&work_thread, work_stack, WORK_STACK_SIZE, worker, NULL,
					    NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

		k_thread_join(t, K_FOREVER);
	}
}

ZTEST(tricore_csa, test_no_leak_on_exit)
{
	/* Warm up so the idle and system threads settle to a steady CSA
	 * depth, then assert the free count does not drop across further
	 * create/exit cycles. A per-cycle leak makes m2 < m1; a one-time
	 * system offset leaves m1 == m2.
	 */
	run_exit_cycles(20);
	k_msleep(5);
	int m1 = csa_free_count();

	run_exit_cycles(CYCLES);
	k_msleep(5);
	int m2 = csa_free_count();

	zassert_equal(m1, m2, "CSA leak in self-abort path: m1=%d m2=%d over %d cycles", m1, m2,
		      CYCLES);
}

ZTEST_SUITE(tricore_csa, NULL, NULL, NULL, NULL, NULL);
