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

#define NWORKERS   4
#define WORK_STACK 1024
#define REC_DEPTH  20
#define WALK_LIMIT (CONFIG_TRICORE_CSA_COUNT + 16)

static K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NWORKERS, WORK_STACK);
static struct k_thread workers[NWORKERS];

static atomic_t timer_fires;
static atomic_t iterations;
static volatile bool stop;
static volatile bool stall_seen;
static volatile uint32_t sink;

static void on_timer(struct k_timer *t)
{
	ARG_UNUSED(t);
	atomic_inc(&timer_fires);
}

static K_TIMER_DEFINE(soak_timer, on_timer, NULL);

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

static uint32_t recurse(int depth)
{
	if (depth == 0) {
		return 1;
	}

	return depth + recurse(depth - 1);
}

static void worker(void *a, void *b, void *c)
{
	uint64_t ceiling = sys_clock_hw_cycles_per_sec() / 10;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (!stop) {
		sink += recurse(REC_DEPTH);

		uint64_t t0 = k_cycle_get_64();

		k_sleep(K_TICKS(1 + (sink & 3)));

		if ((k_cycle_get_64() - t0) >= ceiling) {
			stall_seen = true;
		}

		atomic_inc(&iterations);
	}
}

static void soak_workers(int seconds)
{
	stop = false;

	for (int i = 0; i < NWORKERS; i++) {
		k_thread_create(&workers[i], worker_stacks[i], WORK_STACK, worker, NULL, NULL, NULL,
				K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	}

	k_sleep(K_SECONDS(seconds));
	stop = true;

	for (int i = 0; i < NWORKERS; i++) {
		k_thread_join(&workers[i], K_FOREVER);
	}

	k_msleep(20);
}

ZTEST(tricore_soak, test_concurrent_soak)
{
	stall_seen = false;
	k_timer_start(&soak_timer, K_MSEC(1), K_MSEC(1));

	/* Warm up so idle and system threads settle to a steady CSA depth,
	 * then assert the free count does not drop across the full soak. A
	 * per-iteration CSA leak makes m2 < m1; a one-time system offset
	 * leaves m1 == m2.
	 */
	soak_workers(2);
	int m1 = csa_free_count();

	atomic_set(&timer_fires, 0);
	atomic_set(&iterations, 0);

	soak_workers(CONFIG_TRICORE_SOAK_SECONDS);
	int m2 = csa_free_count();

	k_timer_stop(&soak_timer);

	TC_PRINT("soak: %u iterations, %u timer fires, csa m1=%d m2=%d\n",
		 (uint32_t)atomic_get(&iterations), (uint32_t)atomic_get(&timer_fires), m1, m2);

	zassert_false(stall_seen, "a worker k_sleep stalled under load");
	zassert_equal(m1, m2, "CSA leak across soak: m1=%d m2=%d", m1, m2);
	zassert_true(atomic_get(&iterations) > 0, "workers made no progress");
	zassert_true(atomic_get(&timer_fires) > (CONFIG_TRICORE_SOAK_SECONDS * 500),
		     "timer underfired: %u in %d s", (uint32_t)atomic_get(&timer_fires),
		     CONFIG_TRICORE_SOAK_SECONDS);
}

ZTEST_SUITE(tricore_soak, NULL, NULL, NULL, NULL, NULL);
