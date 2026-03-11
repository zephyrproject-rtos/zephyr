/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_fake.h>
#include <zephyr/fff.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

struct fake_stepper_driver_data {
	enum stepper_micro_step_resolution micro_step_res;
};

struct fake_stepper_ctrl_data {
	int32_t actual_position;
};

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_driver_enable, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_driver_disable, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_driver_set_micro_step_res, const struct device *,
		       enum stepper_micro_step_resolution);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_driver_get_micro_step_res, const struct device *,
		       enum stepper_micro_step_resolution *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_driver_set_event_cb, const struct device *,
		       stepper_event_cb_t, void *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_is_moving, const struct device *, bool *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_move_by, const struct device *, int32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_set_microstep_interval, const struct device *,
		       uint64_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_set_reference_position, const struct device *,
		       int32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_get_actual_position, const struct device *,
		       int32_t *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_move_to, const struct device *, int32_t);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_run, const struct device *,
		       enum stepper_ctrl_direction);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_stop, const struct device *);

DEFINE_FAKE_VALUE_FUNC(int, fake_stepper_ctrl_set_event_cb, const struct device *,
		       stepper_ctrl_event_callback_t, void *);

static int
fake_stepper_driver_set_micro_step_res_delegate(const struct device *dev,
					     const enum stepper_micro_step_resolution res)
{
	struct fake_stepper_driver_data *data = dev->data;

	data->micro_step_res = res;

	return 0;
}

static int fake_stepper_driver_get_micro_step_res_delegate(const struct device *dev,
							enum stepper_micro_step_resolution *res)
{
	struct fake_stepper_driver_data *data = dev->data;

	*res = data->micro_step_res;

	return 0;
}

static int fake_stepper_ctrl_set_reference_position_delegate(const struct device *dev,
							      const int32_t pos)
{
	struct fake_stepper_ctrl_data *data = dev->data;

	data->actual_position = pos;

	return 0;
}

static int fake_stepper_ctrl_get_actual_position_delegate(const struct device *dev, int32_t *pos)
{
	struct fake_stepper_ctrl_data *data = dev->data;

	*pos = data->actual_position;

	return 0;
}

#ifdef CONFIG_ZTEST
static void fake_stepper_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(fake_stepper_driver_enable);
	RESET_FAKE(fake_stepper_driver_disable);
	RESET_FAKE(fake_stepper_driver_set_micro_step_res);
	RESET_FAKE(fake_stepper_driver_get_micro_step_res);
	RESET_FAKE(fake_stepper_driver_set_event_cb);
	RESET_FAKE(fake_stepper_ctrl_move_by);
	RESET_FAKE(fake_stepper_ctrl_is_moving);
	RESET_FAKE(fake_stepper_ctrl_set_microstep_interval);
	RESET_FAKE(fake_stepper_ctrl_set_reference_position);
	RESET_FAKE(fake_stepper_ctrl_get_actual_position);
	RESET_FAKE(fake_stepper_ctrl_move_to);
	RESET_FAKE(fake_stepper_ctrl_run);
	RESET_FAKE(fake_stepper_ctrl_stop);

	/* Install custom fakes for the setter and getter functions */
	fake_stepper_driver_set_micro_step_res_fake.custom_fake =
		fake_stepper_driver_set_micro_step_res_delegate;
	fake_stepper_driver_get_micro_step_res_fake.custom_fake =
		fake_stepper_driver_get_micro_step_res_delegate;
	fake_stepper_ctrl_set_reference_position_fake.custom_fake =
		fake_stepper_ctrl_set_reference_position_delegate;
	fake_stepper_ctrl_get_actual_position_fake.custom_fake =
		fake_stepper_ctrl_get_actual_position_delegate;
}

ZTEST_RULE(fake_stepper_reset_rule, fake_stepper_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static int fake_stepper_driver_init(const struct device *dev)
{
	fake_stepper_driver_set_micro_step_res_fake.custom_fake =
		fake_stepper_driver_set_micro_step_res_delegate;
	fake_stepper_driver_get_micro_step_res_fake.custom_fake =
		fake_stepper_driver_get_micro_step_res_delegate;

	return 0;
}

static int fake_stepper_init(const struct device *dev)
{
	fake_stepper_ctrl_set_reference_position_fake.custom_fake =
		fake_stepper_ctrl_set_reference_position_delegate;
	fake_stepper_ctrl_get_actual_position_fake.custom_fake =
		fake_stepper_ctrl_get_actual_position_delegate;

	return 0;
}

static DEVICE_API(stepper, fake_stepper_driver_api) = {
	.enable = fake_stepper_driver_enable,
	.disable = fake_stepper_driver_disable,
	.set_micro_step_res = fake_stepper_driver_set_micro_step_res,
	.get_micro_step_res = fake_stepper_driver_get_micro_step_res,
	.set_event_cb = fake_stepper_driver_set_event_cb,
};

static DEVICE_API(stepper_ctrl, fake_stepper_ctrl_api) = {
	.move_by = fake_stepper_ctrl_move_by,
	.is_moving = fake_stepper_ctrl_is_moving,
	.set_microstep_interval = fake_stepper_ctrl_set_microstep_interval,
	.set_reference_position = fake_stepper_ctrl_set_reference_position,
	.get_actual_position = fake_stepper_ctrl_get_actual_position,
	.move_to = fake_stepper_ctrl_move_to,
	.run = fake_stepper_ctrl_run,
	.stop = fake_stepper_ctrl_stop,
	.set_event_cb = fake_stepper_ctrl_set_event_cb,
};

#define FAKE_STEPPER_DRIVER_INIT(inst)                                                             \
                                                                                                   \
	static struct fake_stepper_driver_data fake_stepper_driver_data_##inst;                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fake_stepper_driver_init, NULL,                                \
			      &fake_stepper_driver_data_##inst, NULL, POST_KERNEL,                 \
			      CONFIG_STEPPER_INIT_PRIORITY, &fake_stepper_driver_api);

#define FAKE_STEPPER_CTRL_INIT(inst)                                                              \
                                                                                                   \
	static struct fake_stepper_ctrl_data fake_stepper_ctrl_data_##inst;                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fake_stepper_init, NULL, &fake_stepper_ctrl_data_##inst,      \
			      NULL, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,                     \
			      &fake_stepper_ctrl_api);

#define DT_DRV_COMPAT zephyr_fake_stepper_driver
DT_INST_FOREACH_STATUS_OKAY(FAKE_STEPPER_DRIVER_INIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zephyr_fake_stepper_ctrl
DT_INST_FOREACH_STATUS_OKAY(FAKE_STEPPER_CTRL_INIT)
#undef DT_DRV_COMPAT
