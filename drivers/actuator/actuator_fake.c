/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/actuator/fake.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif

#define DT_DRV_COMPAT zephyr_actuator_fake

DEFINE_FAKE_VALUE_FUNC(int,
		       actuator_fake_set_setpoint,
		       const struct device *,
		       q31_t);

static DEVICE_API(actuator, actuator_fake_api) = {
	.set_setpoint = actuator_fake_set_setpoint,
};

#ifdef CONFIG_ZTEST
static void actuator_fake_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(actuator_fake_set_setpoint);
}

ZTEST_RULE(actuator_fake_reset_rule_before, actuator_fake_reset_rule_before, NULL);
#endif

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_ACTUATOR_INIT_PRIORITY,
	&actuator_fake_api
);
