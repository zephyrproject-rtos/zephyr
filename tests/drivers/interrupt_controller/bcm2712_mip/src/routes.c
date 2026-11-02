/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(arm_gic_irq_enable, unsigned int);
FAKE_VOID_FUNC(arm_gic_irq_disable, unsigned int);
FAKE_VALUE_FUNC(bool, arm_gic_irq_is_enabled, unsigned int);
FAKE_VOID_FUNC(arm_gic_irq_clear_pending, unsigned int);
FAKE_VOID_FUNC(arm_gic_irq_set_pending, unsigned int);
FAKE_VALUE_FUNC(bool, arm_gic_irq_is_pending, unsigned int);
FAKE_VOID_FUNC(arm_gic_irq_set_priority, unsigned int, unsigned int, uint32_t);
FAKE_VALUE_FUNC(unsigned int, arm_gic_get_active);
FAKE_VOID_FUNC(arm_gic_eoi, unsigned int);

FAKE_VOID_FUNC(rp1_irq_enable, const struct device *, unsigned int);
FAKE_VOID_FUNC(rp1_irq_disable, const struct device *, unsigned int);
FAKE_VALUE_FUNC(int, rp1_irq_is_enabled, const struct device *, unsigned int);
FAKE_VOID_FUNC(rp1_irq_eoi, const struct device *, unsigned int);

DEVICE_DT_DEFINE(DT_NODELABEL(test_rp1), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

/* Keep the real SoC routing while replacing the hardware operations. */
#include "soc_irq.c"

BUILD_ASSERT(DT_IRQN(DT_NODELABEL(test_client)) == 192U);

static void routes_before(void *fixture)
{
	ARG_UNUSED(fixture);
	RESET_FAKE(arm_gic_irq_enable);
	RESET_FAKE(arm_gic_irq_disable);
	RESET_FAKE(arm_gic_irq_is_enabled);
	RESET_FAKE(arm_gic_irq_set_priority);
	RESET_FAKE(arm_gic_eoi);
	RESET_FAKE(rp1_irq_enable);
	RESET_FAKE(rp1_irq_disable);
	RESET_FAKE(rp1_irq_is_enabled);
	RESET_FAKE(rp1_irq_eoi);
	FFF_RESET_HISTORY();
}

static void routes_after(void *fixture)
{
	ARG_UNUSED(fixture);
	DEVICE_DT_GET(DT_NODELABEL(test_rp1))->state->initialized = true;
}

ZTEST(bcm2712_routes, test_nonzero_channel_base)
{
	const struct {
		unsigned int irq;
		unsigned int local;
	} cases[] = {{191U, 26U}, {192U, 25U}, {193U, 27U}};

	zassert_true(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(test_rp1))));
	rp1_irq_is_enabled_fake.return_val = 1;
	arm_gic_irq_is_enabled_fake.return_val = true;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		z_soc_irq_enable(cases[i].irq);
		zassert_equal(arm_gic_irq_enable_fake.arg0_val, cases[i].irq);
		zassert_equal(rp1_irq_enable_fake.call_count, i + 1U);
		zassert_equal_ptr(rp1_irq_enable_fake.arg0_val,
				  DEVICE_DT_GET(DT_NODELABEL(test_rp1)));
		zassert_equal(rp1_irq_enable_fake.arg1_val, cases[i].local);
		zassert_true(z_soc_irq_is_enabled(cases[i].irq));
		zassert_equal(rp1_irq_is_enabled_fake.arg1_val, cases[i].local);

		z_soc_irq_priority_set(cases[i].irq, 3U, IRQ_TYPE_LEVEL);
		zassert_equal(arm_gic_irq_set_priority_fake.arg0_val, cases[i].irq);
		zassert_equal(arm_gic_irq_set_priority_fake.arg1_val, 3U);
		zassert_equal(arm_gic_irq_set_priority_fake.arg2_val, IRQ_TYPE_EDGE);

		FFF_RESET_HISTORY();
		z_soc_irq_eoi(cases[i].irq);
		zassert_equal(rp1_irq_eoi_fake.call_count, i + 1U);
		zassert_equal(rp1_irq_eoi_fake.arg1_val, cases[i].local);
		zassert_equal(arm_gic_eoi_fake.arg0_val, cases[i].irq);
		zassert_equal(fff.call_history_idx, 2U);
		zassert_equal(fff.call_history[0], (fff_function_t)rp1_irq_eoi);
		zassert_equal(fff.call_history[1], (fff_function_t)arm_gic_eoi);

		z_soc_irq_disable(cases[i].irq);
		zassert_equal(rp1_irq_disable_fake.call_count, i + 1U);
		zassert_equal(rp1_irq_disable_fake.arg1_val, cases[i].local);
		zassert_equal(arm_gic_irq_disable_fake.arg0_val, cases[i].irq);
	}

	z_soc_irq_enable(194U);
	zassert_equal(arm_gic_irq_enable_fake.arg0_val, 194U);
	zassert_equal(rp1_irq_enable_fake.call_count, ARRAY_SIZE(cases));
}

ZTEST(bcm2712_routes, test_controller_not_ready)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_rp1));

	dev->state->initialized = false;
	zassert_false(device_is_ready(dev));

	z_soc_irq_enable(192U);
	z_soc_irq_disable(192U);
	zassert_false(z_soc_irq_is_enabled(192U));
	z_soc_irq_priority_set(192U, 3U, IRQ_TYPE_LEVEL);
	z_soc_irq_eoi(192U);

	zassert_equal(rp1_irq_enable_fake.call_count, 0U);
	zassert_equal(rp1_irq_disable_fake.call_count, 0U);
	zassert_equal(rp1_irq_is_enabled_fake.call_count, 0U);
	zassert_equal(rp1_irq_eoi_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_enable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_disable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_is_enabled_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_set_priority_fake.call_count, 0U);
	zassert_equal(arm_gic_eoi_fake.call_count, 1U);
	zassert_equal(arm_gic_eoi_fake.arg0_val, 192U);

	z_soc_irq_enable(194U);
	zassert_equal(arm_gic_irq_enable_fake.call_count, 1U);
	zassert_equal(arm_gic_irq_enable_fake.arg0_val, 194U);
}

ZTEST_SUITE(bcm2712_routes, NULL, NULL, routes_before, routes_after, NULL);
