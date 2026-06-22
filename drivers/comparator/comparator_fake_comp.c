/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator/fake_comp.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif

#define DT_DRV_COMPAT zephyr_fake_comp

DEFINE_FAKE_VALUE_FUNC(int,
		       comp_fake_comp_get_output,
		       const struct device *);

DEFINE_FAKE_VALUE_FUNC(int,
		       comp_fake_comp_set_trigger,
		       const struct device *,
		       enum comparator_trigger);

DEFINE_FAKE_VALUE_FUNC(int,
		       comp_fake_comp_set_trigger_callback,
		       const struct device *,
		       comparator_callback_t,
		       void *);

DEFINE_FAKE_VALUE_FUNC(int,
		       comp_fake_comp_trigger_is_pending,
		       const struct device *);

static DEVICE_API(comparator, fake_comp_api) = {
	.get_output = comp_fake_comp_get_output,
	.set_trigger = comp_fake_comp_set_trigger,
	.set_trigger_callback = comp_fake_comp_set_trigger_callback,
	.trigger_is_pending = comp_fake_comp_trigger_is_pending,
};

#ifdef CONFIG_ZTEST
static void fake_comp_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(comp_fake_comp_get_output);
	RESET_FAKE(comp_fake_comp_set_trigger);
	RESET_FAKE(comp_fake_comp_set_trigger_callback);
	RESET_FAKE(comp_fake_comp_trigger_is_pending);
}

ZTEST_RULE(comp_fake_comp_reset_rule, fake_comp_reset_rule_before, NULL);
#endif

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_COMPARATOR_INIT_PRIORITY,
	&fake_comp_api
);
