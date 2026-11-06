/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/tricore/cr.h>

#define RSTACK_SIZE 32768
#define WALK_LIMIT  (CONFIG_TRICORE_CSA_COUNT + 16)

static K_THREAD_STACK_DEFINE(rstack, RSTACK_SIZE);
static struct k_thread rthread;
static volatile uint32_t sink;

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

static uint32_t deep_recurse(int depth)
{
	volatile uint32_t local = depth;

	if (depth > 0) {
		local += deep_recurse(depth - 1);
	}

	sink += local;

	return local;
}

static void exhaust_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	ztest_set_fault_valid(true);

	/* Recurse far past the free context list so the upper-context save on
	 * call depletes it and raises the recoverable FCD trap.
	 */
	deep_recurse(CONFIG_TRICORE_CSA_COUNT * 4);

	zassert_unreachable("CSA depletion did not trap");
}

ZTEST(tricore_csa_exhaust, test_fcd_depletion_recovers)
{
	k_tid_t t = k_thread_create(&rthread, rstack, RSTACK_SIZE, exhaust_entry, NULL, NULL, NULL,
				    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	k_thread_join(t, K_FOREVER);

	/* Survived the FCD trap; the free list must be replenished and the
	 * system still healthy enough to keep scheduling.
	 */
	k_msleep(2);
	zassert_true(csa_free_count() > 8, "free list not recovered after FCD");
}

ZTEST_SUITE(tricore_csa_exhaust, NULL, NULL, NULL, NULL, NULL);
