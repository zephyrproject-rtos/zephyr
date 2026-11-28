/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT arm_gic_v3_its

#define ITS_TEST_LPI_INT_BASE 8192U
#define ITS_TEST_WORKERS      2U
#define ITS_TEST_SMALL_LPIS   (CONFIG_NUM_IRQS - ITS_TEST_LPI_INT_BASE)

static K_SEM_DEFINE(start, 0, ITS_TEST_WORKERS);
static ATOMIC_DEFINE(allocated, ITS_TEST_SMALL_LPIS);

static void allocate_lpis(void *p1, void *p2, void *p3)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	bool *failed = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (k_sem_take(&start, K_FOREVER) != 0) {
		*failed = true;
		return;
	}

	while (true) {
		unsigned int intid = UINT_MAX;
		int ret = its_alloc_intids(dev, 1U, &intid);

		if (ret == -ENOSPC) {
			*failed = intid != UINT_MAX;
			return;
		}

		if ((ret != 0) || (intid < ITS_TEST_LPI_INT_BASE) || (intid >= CONFIG_NUM_IRQS) ||
		    atomic_test_and_set_bit(allocated, (int)(intid - ITS_TEST_LPI_INT_BASE))) {
			*failed = true;
			return;
		}

		k_yield();
	}
}

ZTEST(arm64_gicv3_its, test_gicv3_its_concurrent_exhaustion)
{
	static K_THREAD_STACK_ARRAY_DEFINE(stacks, ITS_TEST_WORKERS, 2048);
	static struct k_thread workers[ITS_TEST_WORKERS];
	static bool failed[ITS_TEST_WORKERS];

	zassert_true(device_is_ready(DEVICE_DT_INST_GET(0)));

	for (size_t i = 0U; i < ITS_TEST_WORKERS; i++) {
		k_tid_t worker = k_thread_create(
			&workers[i], stacks[i], K_THREAD_STACK_SIZEOF(stacks[i]), allocate_lpis,
			&failed[i], NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);

#ifdef CONFIG_SMP
		zassert_ok(k_thread_cpu_pin(worker, (int)i));
#endif
		k_thread_start(worker);
	}

	for (size_t i = 0U; i < ITS_TEST_WORKERS; i++) {
		k_sem_give(&start);
	}

	for (size_t i = 0U; i < ITS_TEST_WORKERS; i++) {
		zassert_ok(k_thread_join(&workers[i], K_SECONDS(1)));
		zassert_false(failed[i]);
	}

	for (size_t i = 0U; i < ITS_TEST_SMALL_LPIS; i++) {
		zassert_true(atomic_test_bit(allocated, (int)i));
	}
}

ZTEST_SUITE(arm64_gicv3_its, NULL, NULL, NULL, NULL, NULL);
