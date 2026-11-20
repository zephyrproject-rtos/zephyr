/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/intc_rp1.h>
#include <zephyr/drivers/interrupt_controller/msi_proxy.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "soc_irq.h"

/* The native test calls the BCM2712 IRQ hooks directly. */
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);
void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);
void z_soc_irq_eoi(unsigned int irq);

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

FAKE_VOID_FUNC(mip1_source_enable, const struct device *, unsigned int);
FAKE_VOID_FUNC(mip1_source_disable, const struct device *, unsigned int);
FAKE_VALUE_FUNC(int, mip1_source_is_enabled, const struct device *, unsigned int);
FAKE_VOID_FUNC(mip1_source_eoi, const struct device *, unsigned int);

FAKE_VALUE_FUNC(int, fake_mip_enable, const struct device *, unsigned int);
FAKE_VOID_FUNC(fake_mip_disable, const struct device *, unsigned int);

static DEVICE_API(msi_proxy, fake_mip_api) = {
	.enable = fake_mip_enable,
	.disable = fake_mip_disable,
};

DEVICE_DT_DEFINE(DT_NODELABEL(test_route_mip), NULL, NULL, NULL, NULL, POST_KERNEL, 0,
		 &fake_mip_api);
DEVICE_DT_DEFINE(DT_NODELABEL(test_route_mip1), NULL, NULL, NULL, NULL, POST_KERNEL, 0,
		 &fake_mip_api);
DEVICE_DT_DEFINE(DT_NODELABEL(test_rp1), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(test_mip1_source), NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

static const struct bcm_irq_ops mip1_source_ops = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(test_mip1_source)),
	.enable = mip1_source_enable,
	.disable = mip1_source_disable,
	.is_enabled = mip1_source_is_enabled,
	.eoi = mip1_source_eoi,
};

BCM2712_IRQ_ROUTE_DEFINE(test_mip1_source_route, DT_NODELABEL(test_mip1_source), DT_INVALID_NODE,
			 mip1_source_ops);

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
	RESET_FAKE(mip1_source_enable);
	RESET_FAKE(mip1_source_disable);
	RESET_FAKE(mip1_source_is_enabled);
	RESET_FAKE(mip1_source_eoi);
	RESET_FAKE(fake_mip_enable);
	RESET_FAKE(fake_mip_disable);
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
		unsigned int channel;
	} cases[] = {{193U, 26U, 31U}, {192U, 25U, 32U}, {195U, 27U, 35U}};

	zassert_true(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(test_rp1))));
	rp1_irq_is_enabled_fake.return_val = 1;
	arm_gic_irq_is_enabled_fake.return_val = true;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		FFF_RESET_HISTORY();
		z_soc_irq_enable(cases[i].irq);
		zassert_equal(fff.call_history_idx, 4U);
		zassert_equal(fff.call_history[1], (fff_function_t)arm_gic_irq_enable);
		zassert_equal(fff.call_history[2], (fff_function_t)fake_mip_enable);
		zassert_equal(fff.call_history[3], (fff_function_t)rp1_irq_enable);
		zassert_equal(arm_gic_irq_enable_fake.arg0_val, cases[i].irq);
		zassert_equal_ptr(fake_mip_enable_fake.arg0_val,
				  DEVICE_DT_GET(DT_NODELABEL(test_route_mip)));
		zassert_equal(fake_mip_enable_fake.arg1_val, cases[i].channel);
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

		FFF_RESET_HISTORY();
		z_soc_irq_disable(cases[i].irq);
		zassert_equal(fff.call_history_idx, 3U);
		zassert_equal(fff.call_history[0], (fff_function_t)rp1_irq_disable);
		zassert_equal(fff.call_history[1], (fff_function_t)fake_mip_disable);
		zassert_equal(fff.call_history[2], (fff_function_t)arm_gic_irq_disable);
		zassert_equal(rp1_irq_disable_fake.call_count, i + 1U);
		zassert_equal(rp1_irq_disable_fake.arg1_val, cases[i].local);
		zassert_equal(fake_mip_disable_fake.arg1_val, cases[i].channel);
		zassert_equal(arm_gic_irq_disable_fake.arg0_val, cases[i].irq);
	}

	z_soc_irq_enable(194U);
	zassert_equal(arm_gic_irq_enable_fake.arg0_val, 194U);
	zassert_equal(rp1_irq_enable_fake.call_count, ARRAY_SIZE(cases));
	zassert_equal(fake_mip_enable_fake.call_count, ARRAY_SIZE(cases));

	z_soc_irq_eoi(27U);
	zassert_equal(rp1_irq_eoi_fake.call_count, ARRAY_SIZE(cases));
	zassert_equal(arm_gic_eoi_fake.arg0_val, 27U);
}

ZTEST(bcm2712_routes, test_mip_routes_without_local_source)
{
	const struct {
		const struct device *mip;
		unsigned int irq;
		unsigned int channel;
	} cases[] = {
		{DEVICE_DT_GET(DT_NODELABEL(test_route_mip)), 199U, 36U},
		{DEVICE_DT_GET(DT_NODELABEL(test_route_mip1)), 260U, 47U},
	};

	DEVICE_DT_GET(DT_NODELABEL(test_rp1))->state->initialized = false;
	arm_gic_irq_is_enabled_fake.return_val = true;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		FFF_RESET_HISTORY();
		z_soc_irq_enable(cases[i].irq);
		zassert_equal(fff.call_history_idx, 3U);
		zassert_equal(fff.call_history[1], (fff_function_t)arm_gic_irq_enable);
		zassert_equal(fff.call_history[2], (fff_function_t)fake_mip_enable);
		zassert_equal(arm_gic_irq_enable_fake.arg0_val, cases[i].irq);
		zassert_equal_ptr(fake_mip_enable_fake.arg0_val, cases[i].mip);
		zassert_equal(fake_mip_enable_fake.arg1_val, cases[i].channel);
		zassert_true(z_soc_irq_is_enabled(cases[i].irq));

		z_soc_irq_priority_set(cases[i].irq, 3U, IRQ_TYPE_LEVEL);
		zassert_equal(arm_gic_irq_set_priority_fake.arg0_val, cases[i].irq);
		zassert_equal(arm_gic_irq_set_priority_fake.arg2_val, IRQ_TYPE_EDGE);

		z_soc_irq_eoi(cases[i].irq);
		zassert_equal(arm_gic_eoi_fake.arg0_val, cases[i].irq);

		FFF_RESET_HISTORY();
		z_soc_irq_disable(cases[i].irq);
		zassert_equal(fff.call_history_idx, 2U);
		zassert_equal(fff.call_history[0], (fff_function_t)fake_mip_disable);
		zassert_equal(fff.call_history[1], (fff_function_t)arm_gic_irq_disable);
		zassert_equal_ptr(fake_mip_disable_fake.arg0_val, cases[i].mip);
		zassert_equal(fake_mip_disable_fake.arg1_val, cases[i].channel);
		zassert_equal(arm_gic_irq_disable_fake.arg0_val, cases[i].irq);
	}

	zassert_equal(rp1_irq_enable_fake.call_count, 0U);
	zassert_equal(rp1_irq_is_enabled_fake.call_count, 0U);
	zassert_equal(rp1_irq_eoi_fake.call_count, 0U);
	zassert_equal(rp1_irq_disable_fake.call_count, 0U);
	zassert_equal(mip1_source_enable_fake.call_count, 0U);
	zassert_equal(mip1_source_is_enabled_fake.call_count, 0U);
	zassert_equal(mip1_source_eoi_fake.call_count, 0U);
	zassert_equal(mip1_source_disable_fake.call_count, 0U);
}

ZTEST(bcm2712_routes, test_mip1_local_source)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_mip1_source));
	const struct device *mip = DEVICE_DT_GET(DT_NODELABEL(test_route_mip1));
	const unsigned int irq = 272U;

	z_soc_irq_enable(irq);
	zassert_equal_ptr(fake_mip_enable_fake.arg0_val, mip);
	zassert_equal(fake_mip_enable_fake.arg1_val, 32U);
	zassert_equal(fff.call_history[2], (fff_function_t)fake_mip_enable);
	zassert_equal(fff.call_history[3], (fff_function_t)mip1_source_enable);
	zassert_equal_ptr(mip1_source_enable_fake.arg0_val, dev);
	zassert_equal(mip1_source_enable_fake.arg1_val, 3U);

	arm_gic_irq_is_enabled_fake.return_val = true;
	mip1_source_is_enabled_fake.return_val = 1;
	zassert_true(z_soc_irq_is_enabled(irq));
	zassert_equal_ptr(mip1_source_is_enabled_fake.arg0_val, dev);
	zassert_equal(mip1_source_is_enabled_fake.arg1_val, 3U);

	FFF_RESET_HISTORY();
	z_soc_irq_eoi(irq);
	zassert_equal(fff.call_history[0], (fff_function_t)mip1_source_eoi);
	zassert_equal(fff.call_history[1], (fff_function_t)arm_gic_eoi);
	zassert_equal_ptr(mip1_source_eoi_fake.arg0_val, dev);
	zassert_equal(mip1_source_eoi_fake.arg1_val, 3U);

	FFF_RESET_HISTORY();
	z_soc_irq_disable(irq);
	zassert_equal(fff.call_history[0], (fff_function_t)mip1_source_disable);
	zassert_equal(fff.call_history[1], (fff_function_t)fake_mip_disable);
	zassert_equal(fff.call_history[2], (fff_function_t)arm_gic_irq_disable);
	zassert_equal_ptr(mip1_source_disable_fake.arg0_val, dev);
	zassert_equal(mip1_source_disable_fake.arg1_val, 3U);
	zassert_equal(rp1_irq_enable_fake.call_count, 0U);
	zassert_equal(rp1_irq_is_enabled_fake.call_count, 0U);
	zassert_equal(rp1_irq_eoi_fake.call_count, 0U);
	zassert_equal(rp1_irq_disable_fake.call_count, 0U);
}

ZTEST(bcm2712_routes, test_mip_enable_failure)
{
	fake_mip_enable_fake.return_val = -EINVAL;

	z_soc_irq_enable(192U);

	zassert_equal(arm_gic_irq_enable_fake.call_count, 1U);
	zassert_equal(fake_mip_enable_fake.call_count, 1U);
	zassert_equal(rp1_irq_enable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_disable_fake.call_count, 1U);
	zassert_equal(arm_gic_irq_disable_fake.arg0_val, 192U);

	arm_gic_irq_is_enabled_fake.return_val = true;
	z_soc_irq_enable(192U);

	zassert_equal(fake_mip_enable_fake.call_count, 2U);
	zassert_equal(rp1_irq_enable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_disable_fake.call_count, 1U);
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
	zassert_equal(fake_mip_enable_fake.call_count, 0U);
	zassert_equal(fake_mip_disable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_enable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_disable_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_is_enabled_fake.call_count, 0U);
	zassert_equal(arm_gic_irq_set_priority_fake.call_count, 1U);
	zassert_equal(arm_gic_irq_set_priority_fake.arg0_val, 192U);
	zassert_equal(arm_gic_irq_set_priority_fake.arg1_val, 3U);
	zassert_equal(arm_gic_irq_set_priority_fake.arg2_val, IRQ_TYPE_EDGE);
	zassert_equal(arm_gic_eoi_fake.call_count, 1U);
	zassert_equal(arm_gic_eoi_fake.arg0_val, 192U);

	z_soc_irq_enable(194U);
	zassert_equal(arm_gic_irq_enable_fake.call_count, 1U);
	zassert_equal(arm_gic_irq_enable_fake.arg0_val, 194U);

	dev->state->initialized = true;
	z_soc_irq_enable(192U);
	zassert_equal(arm_gic_irq_enable_fake.call_count, 2U);
	zassert_equal(arm_gic_irq_enable_fake.arg0_val, 192U);
	zassert_equal(fake_mip_enable_fake.call_count, 1U);
	zassert_equal(rp1_irq_enable_fake.call_count, 1U);
	zassert_equal(arm_gic_irq_set_priority_fake.call_count, 1U);
}

ZTEST_SUITE(bcm2712_routes, NULL, NULL, routes_before, routes_after, NULL);
