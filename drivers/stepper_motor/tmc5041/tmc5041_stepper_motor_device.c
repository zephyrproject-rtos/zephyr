/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT tmc5041_stepper_motor_device

#include <zephyr/drivers/stepper_motor_device.h>
#include <zephyr/drivers/stepper_motor/tmc5041.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(tmc5041_stepper_motor_device, CONFIG_STEPPER_MOTOR_DEVICE_LOG_LEVEL);

struct stepper_motor_controller_spec_reg {
	uint32_t value;
};

static int motor_init(const struct device *dev)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;

	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	stepper_motor_controller_write(&config->stepper_motor_bus, STALL_DETECTION,
				       config->stall_guard_setting);

	for (int i = 0; i < config->num_stepper_motor_controller_spec_regs; i += 2) {
		LOG_DBG("Setting Hold Run Register Value to %d 0x%x 0x%x",
			config->num_stepper_motor_controller_spec_regs,
			config->controller_spec_regs_value_set[i].value,
			config->controller_spec_regs_value_set[i + 1].value);
		stepper_motor_controller_write_reg(
			config->stepper_motor_bus.bus,
			config->controller_spec_regs_value_set[i].value,
			config->controller_spec_regs_value_set[i + 1].value);
	}

	k_mutex_init(&data->calibration_mutex);

	return 0;
}

static int32_t motor_reset(const struct device *dev)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;
	stepper_motor_controller_reset(&config->stepper_motor_bus);

	return 0;
}

static int32_t motor_run(const struct device *dev, const struct stepper_motor_run_info *run_info)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;
	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	if ((0U == data->min_position) && (0U == data->max_position)) {
		LOG_WRN("trying to run %s motor in an uncalibrated state", dev->name);
	}
	int32_t ret = 0;

	(void)k_mutex_lock(&data->calibration_mutex, K_FOREVER);
	stepper_motor_controller_write(&config->stepper_motor_bus, STALL_GUARD,
				       TMC5041_SW_MODE_SG_STOP_DISABLE);
	switch (run_info->direction) {
	case POSITIVE: {
		stepper_motor_controller_write(&config->stepper_motor_bus, FREE_WHEELING,
					       TMC5041_RAMPMODE_POSITIVE_VELOCITY_MODE);
		break;
	}

	case NEGATIVE: {
		stepper_motor_controller_write(&config->stepper_motor_bus, FREE_WHEELING,
					       TMC5041_RAMPMODE_NEGATIVE_VELOCITY_MODE);
		break;
	}

	default: {
		ret = -EINVAL;
		break;
	}
	}
	(void)k_mutex_unlock(&data->calibration_mutex);

    /**
     * Do not enable during motor spin-up, wait until the motor velocity exceeds a certain value,
     * where StallGuard2 delivers a stable result
     */
	k_sleep(K_MSEC(100));
	stepper_motor_controller_write(&config->stepper_motor_bus, STALL_GUARD,
				       TMC5041_SW_MODE_SG_STOP_ENABLE);
	return ret;
}

static int32_t motor_stop(const struct device *dev)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;
	stepper_motor_controller_write(&config->stepper_motor_bus, FREE_WHEELING,
				       TMC5041_RAMPMODE_HOLD_MODE);
	return 0;
}

static int32_t motor_set_position(const struct device *dev,
				  const struct stepper_motor_position_info *position_info)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;

	int32_t ret = 0;

	/** release motor if stalled,alternatively one can poll rampstat in order to deactivate sg*/
	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	if ((0U == data->min_position) && (0U == data->max_position)) {
		LOG_WRN("%s motor is running in an uncalibrated state", dev->name);
	}

	(void)k_mutex_lock(&data->calibration_mutex, K_FOREVER);

	switch (position_info->type) {
	case MOTOR_POSITION_MIN: {
		data->min_position = position_info->position;
		break;
	}
	case MOTOR_POSITION_MAX: {
		data->max_position = position_info->position;
		break;
	}
	case MOTOR_POSITION_TARGET: {
		stepper_motor_controller_write(&config->stepper_motor_bus, STALL_GUARD,
					       TMC5041_SW_MODE_SG_STOP_DISABLE);
		stepper_motor_controller_write(&config->stepper_motor_bus, FREE_WHEELING,
					       TMC5041_RAMPMODE_POSITIONING_MODE);

		stepper_motor_controller_write(&config->stepper_motor_bus, TARGET_POSITION,
					       position_info->position);

		k_sleep(K_MSEC(100));
		stepper_motor_controller_write(&config->stepper_motor_bus, STALL_GUARD,
					       TMC5041_SW_MODE_SG_STOP_ENABLE);
		break;
	}
	case MOTOR_POSITION_ACTUAL: {
		stepper_motor_controller_write(&config->stepper_motor_bus, FREE_WHEELING,
					       TMC5041_RAMPMODE_HOLD_MODE);
		stepper_motor_controller_write(&config->stepper_motor_bus, ACTUAL_POSITION,
					       position_info->position);
		break;
	}
	default: {
		ret = -EINVAL;
		break;
	}
	}

	(void)k_mutex_unlock(&data->calibration_mutex);

	return ret;
}

static int32_t motor_get_position(const struct device *dev,
				  struct stepper_motor_position_info *position_info)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;
	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	int32_t ret = 0;

	switch (position_info->type) {
	case MOTOR_POSITION_MIN: {
		position_info->position = data->min_position;
		break;
	}
	case MOTOR_POSITION_MAX: {
		position_info->position = data->max_position;
		break;
	}
	case MOTOR_POSITION_ACTUAL: {
		stepper_motor_controller_read(&config->stepper_motor_bus, ACTUAL_POSITION,
					      &position_info->position);
		break;
	}
	case MOTOR_POSITION_TARGET: {
		stepper_motor_controller_read(&config->stepper_motor_bus, TARGET_POSITION,
					      &position_info->position);
		break;
	}
	default: {
		ret = -EINVAL;
		break;
	}
	}
	return ret;
}

static int32_t motor_get_stall_status(const struct device *dev, bool *stall_status)
{
	const struct stepper_motor_config *config =
		(const struct stepper_motor_config *)dev->config;
	int32_t sg_result = 0;

	stepper_motor_controller_read(&config->stepper_motor_bus, STALL_DETECTION, &sg_result);

	LOG_DBG("Detecting Load for motor %s %ld %ld", dev->name,
		sg_result & TMC5041_DRV_STATUS_SG_RESULT_MASK,
		(sg_result & TMC5041_DRV_STATUS_SG_STATUS_MASK) >>
			TMC5041_DRV_STATUS_SG_STATUS_SHIFT);

	if (0U == (sg_result & TMC5041_DRV_STATUS_SG_RESULT_MASK)) {
		*stall_status = true;
	} else {
		*stall_status = false;
	}
	return 0;
}

static int32_t motor_calibrate(const struct device *dev)
{
	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	int32_t ret = 0;

	if (data->stepper_motor_calibrate_func) {
		(void)k_mutex_lock(&data->calibration_mutex, K_FOREVER);
		data->stepper_motor_calibrate_func(dev, &data->min_position, &data->max_position);
		(void)k_mutex_unlock(&data->calibration_mutex);

		LOG_INF("Min Pos:%d Max Pos:%d", data->min_position, data->max_position);
	} else {
		LOG_ERR("%s motor cannot be calibrated without calibration function", dev->name);
		ret = -ENOTSUP;
	}
	return ret;
}

static int32_t motor_register_calibrate_func(const struct device *dev,
					     int32_t (*calibrate_func)(const struct device *dev,
								       int32_t *min_pos,
								       int32_t *max_pos))
{
	struct stepper_motor_data *data = (struct stepper_motor_data *)dev->data;

	(void)k_mutex_lock(&data->calibration_mutex, K_FOREVER);
	if (calibrate_func) {
		data->stepper_motor_calibrate_func = calibrate_func;
	}
	(void)k_mutex_unlock(&data->calibration_mutex);
	return 0;
}

static const struct stepper_motor_api motor_api = {
	.stepper_motor_reset = motor_reset,
	.stepper_motor_run = motor_run,
	.stepper_motor_stop = motor_stop,
	.stepper_motor_get_stall_status = motor_get_stall_status,
	.stepper_motor_get_position = motor_get_position,
	.stepper_motor_set_position = motor_set_position,
	.stepper_motor_calibrate = motor_calibrate,
	.stepper_motor_register_calibrate_func = motor_register_calibrate_func,
};

#define CONTROLLER_SPEC_SPEC_GET_BY_IDX_WRAPPER(node_id, prop, idx)                                \
	{.value = DT_PROP_BY_IDX(node_id, prop, idx)},

#define STEPPER_MOTOR_DEFINE(inst)                                                                 \
	static struct stepper_motor_data stepper_motor_data_##inst = {                             \
		.min_position = 0U,                                                                \
		.max_position = 0U,                                                                \
		.stepper_motor_calibrate_func = NULL,                                              \
		.micro_step_resolution = DT_INST_PROP_OR(inst, micro_step_res, 256)};              \
	static const struct stepper_motor_controller_spec_reg controller_reg_##inst[] = {          \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, controller_spec_reg_settings),              \
			   (DT_INST_FOREACH_PROP_ELEM(inst, controller_spec_reg_settings,          \
						      CONTROLLER_SPEC_SPEC_GET_BY_IDX_WRAPPER)))}; \
	static const struct stepper_motor_config motor_config_##inst = {                           \
		.num_stepper_motor_controller_spec_regs = ARRAY_SIZE(controller_reg_##inst),       \
		.stepper_motor_bus = STEPPER_MOTOR_DT_SPEC_INST_GET(inst),                         \
		.stall_guard_setting = DT_INST_PROP_OR(inst, stall_guard_setting, NULL),           \
		.gear_ratio = DT_INST_STRING_UNQUOTED(inst, gear_ratio),                           \
		.steps_per_revolution = DT_INST_PROP(inst, steps_per_revolution),                  \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, controller_spec_reg_settings),              \
			   (.controller_spec_regs_value_set = controller_reg_##inst))};          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, motor_init, NULL, &stepper_motor_data_##inst,                  \
			      &motor_config_##inst, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
			      &motor_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_MOTOR_DEFINE)
