/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

#define DT_DRV_COMPAT zephyr_fake_stepper

struct fake_stepper_data {
	enum stepper_micro_step_resolution micro_step_res;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_enable, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_disable, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_set_micro_step_res, const struct device *,
		       enum stepper_micro_step_resolution);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_get_micro_step_res, const struct device *,
		       enum stepper_micro_step_resolution *);

static int fake_stepper_set_micro_step_res_delegate(const struct device *dev,
						    const enum stepper_micro_step_resolution res)
{
	struct fake_stepper_data *data = dev->data;

	data->micro_step_res = res;

	return 0;
}

static int fake_stepper_get_micro_step_res_delegate(const struct device *dev,
						    enum stepper_micro_step_resolution *res)
{
	struct fake_stepper_data *data = dev->data;

	*res = data->micro_step_res;

	return 0;
}

#ifdef CONFIG_ZTEST
static void fake_stepper_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_stepper_enable);
	RESET_FAKE(fake_stepper_disable);
	RESET_FAKE(fake_stepper_set_micro_step_res);
	RESET_FAKE(fake_stepper_get_micro_step_res);

	/* Install custom fakes for the setter and getter functions */
	fake_stepper_set_micro_step_res_fake.custom_fake = fake_stepper_set_micro_step_res_delegate;
	fake_stepper_get_micro_step_res_fake.custom_fake = fake_stepper_get_micro_step_res_delegate;
}

ZTEST_RULE(fake_stepper_reset_rule, fake_stepper_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_stepper_init(const struct device *dev)
{
	fake_stepper_set_micro_step_res_fake.custom_fake = fake_stepper_set_micro_step_res_delegate;
	fake_stepper_get_micro_step_res_fake.custom_fake = fake_stepper_get_micro_step_res_delegate;
	return 0;
}

static DEVICE_API(stepper_drv, fake_stepper_driver_api) = {
	.enable = fake_stepper_enable,
	.disable = fake_stepper_disable,
	.set_micro_step_res = fake_stepper_set_micro_step_res,
	.get_micro_step_res = fake_stepper_get_micro_step_res,
};

#define FAKE_STEPPER_INIT(inst)                                                                    \
                                                                                                   \
	static struct fake_stepper_data fake_stepper_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fake_stepper_init, NULL, &fake_stepper_data_##inst, NULL,      \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,                           \
			      &fake_stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_STEPPER_INIT)
