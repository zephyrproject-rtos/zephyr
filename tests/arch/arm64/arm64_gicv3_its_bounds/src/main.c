/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#include <zephyr/ztest.h>
#include <limits.h>

#define RACE_COUNT 32U
#define WORKERS    2U

static const struct device *const its = DEVICE_DT_GET(DT_NODELABEL(its));
static K_SEM_DEFINE(irq_received, 0, 1);
static K_SEM_DEFINE(start, 0, WORKERS);
static K_THREAD_STACK_ARRAY_DEFINE(stacks, WORKERS, 2048);
static struct k_thread threads[WORKERS];
static ATOMIC_DEFINE(seen, RACE_COUNT);
static atomic_t allocated;
static atomic_t failures;
static uint32_t race_start;
static uint32_t limit;

static void lpi_handler(const void *arg)
{
	ARG_UNUSED(arg);
	k_sem_give(&irq_received);
}

static void allocate_remaining(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (k_sem_take(&start, K_FOREVER) != 0) {
		atomic_inc(&failures);
		return;
	}

	for (uint32_t i = 0; i < RACE_COUNT * 2U; i++) {
		int intid = its_alloc_intid(its);

		if (intid != ITS_INTID_INVALID) {
			if ((uint32_t)intid < race_start || (uint32_t)intid >= limit ||
			    atomic_test_and_set_bit(seen, intid - race_start)) {
				atomic_inc(&failures);
			}
			atomic_inc(&allocated);
		}
		k_yield();
	}
}

ZTEST(gicv3_its_bounds, test_lpi_bounds)
{
	/* Read the width actually configured in the first redistributor's PROPBASER. */
	uint32_t bits = (sys_read64(DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1) + 0x70U) & 0x1fU) + 1U;
	uint32_t remaining;
	int intid;

	limit = MIN(BIT(bits), CONFIG_NUM_IRQS);
	zassert_true(device_is_ready(its));
	zassert_true(limit > GIC_LPI_INT_BASE);
	TC_PRINT("LPI limit: %u, configured IRQs: %u\n", limit, CONFIG_NUM_IRQS);

	intid = its_alloc_intid(its);
	zassert_equal(intid, GIC_LPI_INT_BASE);
	zassert_ok(its_setup_deviceid(its, 0U, 2U));
	zassert_ok(its_map_intid(its, 0U, 0U, intid));
	zassert_equal(irq_connect_dynamic(intid, 0, lpi_handler, NULL, 0), intid);
	irq_enable(intid);
	zassert_ok(its_send_int(its, 0U, 0U));
	zassert_ok(k_sem_take(&irq_received, K_MSEC(100)));
	irq_disable(intid);

	zassert_equal(its_map_intid(its, 0U, 0U, GIC_LPI_INT_BASE - 1U), -EINVAL);
	zassert_equal(its_map_intid(its, 0U, 0U, ITS_INTID_INVALID), -EINVAL);
	const uint32_t invalid[] = {limit, 65536U, 73728U, UINT_MAX};

	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		zassert_equal(its_map_intid(its, 0U, 0U, invalid[i]), -EINVAL);
		arm_gic_irq_enable(invalid[i]);
		zassert_false(arm_gic_irq_is_enabled(invalid[i]));
		arm_gic_irq_set_priority(invalid[i], 0U, 0U);
		arm_gic_irq_disable(invalid[i]);
		zassert_false(arm_gic_irq_is_enabled(invalid[i]));
	}

	remaining = MIN(RACE_COUNT, limit - GIC_LPI_INT_BASE - 1U);
	race_start = limit - remaining;
	for (uint32_t expected = GIC_LPI_INT_BASE + 1U; expected < race_start; expected++) {
		zassert_equal(its_alloc_intid(its), expected);
	}

	for (size_t i = 0; i < WORKERS; i++) {
		k_thread_create(&threads[i], stacks[i], K_THREAD_STACK_SIZEOF(stacks[i]),
				allocate_remaining, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0,
				K_NO_WAIT);
	}
	for (size_t i = 0; i < WORKERS; i++) {
		k_sem_give(&start);
	}
	for (size_t i = 0; i < WORKERS; i++) {
		zassert_ok(k_thread_join(&threads[i], K_SECONDS(1)));
	}
	zassert_equal(atomic_get(&allocated), remaining);
	zassert_equal(atomic_get(&failures), 0);
	for (size_t i = 0; i < remaining; i++) {
		zassert_true(atomic_test_bit(seen, i));
	}
	for (size_t i = 0; i < 100U; i++) {
		zassert_equal(its_alloc_intid(its), ITS_INTID_INVALID);
	}

	/* The last valid ID remains usable after allocation exhaustion. */
	intid = limit - 1U;
	zassert_ok(its_map_intid(its, 0U, 0U, intid));
	zassert_equal(irq_connect_dynamic(intid, 0, lpi_handler, NULL, 0), intid);
	irq_enable(intid);
	zassert_true(irq_is_enabled(intid));
	zassert_ok(its_send_int(its, 0U, 0U));
	zassert_ok(k_sem_take(&irq_received, K_MSEC(100)));
	irq_disable(intid);
	zassert_false(irq_is_enabled(intid));
}

ZTEST_SUITE(gicv3_its_bounds, NULL, NULL, NULL, NULL, NULL);
