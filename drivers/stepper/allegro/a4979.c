/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_a4979

#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/gpio.h>
#include "../step_dir/step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(a4979, CONFIG_STEPPER_LOG_LEVEL);

struct a4979_config {
	const struct step_dir_stepper_common_config common;
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec reset_pin;
	const struct gpio_dt_spec m0_pin;
	const struct gpio_dt_spec m1_pin;
};

struct a4979_data {
	const struct step_dir_stepper_common_data common;
	enum stepper_micro_step_resolution micro_step_res;
	bool enabled;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct a4979_config, struct a4979_data);

static int a4979_set_microstep_pin(const struct device *dev, const struct gpio_dt_spec *pin,
				   int value)
{
	int ret;

	/* Reset microstep pin as it may have been disconnected. */
	ret = gpio_pin_configure_dt(pin, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure microstep pin (error: %d)", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(pin, value);
	if (ret != 0) {
		LOG_ERR("Failed to set microstep pin (error: %d)", ret);
		return ret;
	}

	return 0;
}

static int a4979_stepper_enable(const struct device *dev)
{
	int ret;
	const struct a4979_config *config = dev->config;
	struct a4979_data *data = dev->data;
	bool has_enable_pin = config->en_pin.port != NULL;

	/* Check availability of enable pin, as it might be hardwired. */
	if (!has_enable_pin) {
		LOG_ERR("%s: Enable pin undefined.", dev->name);
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->en_pin, 1);
	if (ret != 0) {
		LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
		return ret;
	}

	data->enabled = true;

	return 0;
}

static int a4979_stepper_disable(const struct device *dev)
{
	int ret;
	const struct a4979_config *config = dev->config;
	struct a4979_data *data = dev->data;
	bool has_enable_pin = config->en_pin.port != NULL;

	/* Check availability of enable pin, as it might be hardwired. */
	if (!has_enable_pin) {
		LOG_ERR("%s: Enable pin undefined.", dev->name);
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->en_pin, 0);
	if (ret != 0) {
		LOG_ERR("%s: Failed to set en_pin (error: %d)", dev->name, ret);
		return ret;
	}

	config->common.timing_source->stop(dev);
	data->enabled = false;

	return 0;
}

static int a4979_stepper_set_micro_step_res(const struct device *dev,
					    const enum stepper_micro_step_resolution micro_step_res)
{
	const struct a4979_config *config = dev->config;
	struct a4979_data *data = dev->data;
	int ret;

	uint8_t m0_value = 0;
	uint8_t m1_value = 0;

	switch (micro_step_res) {
	case STEPPER_MICRO_STEP_1:
		m0_value = 0;
		m1_value = 0;
		break;
	case STEPPER_MICRO_STEP_2:
		m0_value = 1;
		m1_value = 0;
		break;
	case STEPPER_MICRO_STEP_4:
		m0_value = 0;
		m1_value = 1;
	case STEPPER_MICRO_STEP_16:
		m0_value = 1;
		m1_value = 1;
		break;
	default:
		LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
		return -ENOTSUP;
	}

	ret = a4979_set_microstep_pin(dev, &config->m0_pin, m0_value);
	if (ret != 0) {
		return ret;
	}
	ret = a4979_set_microstep_pin(dev, &config->m1_pin, m1_value);
	if (ret != 0) {
		return ret;
	}

	data->micro_step_res = micro_step_res;
	return 0;
}

static int a4979_stepper_get_micro_step_res(const struct device *dev,
					    enum stepper_micro_step_resolution *micro_step_res)
{
	struct a4979_data *data = dev->data;

	*micro_step_res = data->micro_step_res;
	return 0;
}

static int a4979_move_to(const struct device *dev, int32_t target)
{
	struct a4979_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to move to target position, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_move_to(dev, target);
}

static int a4979_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct a4979_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to move by delta, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_move_by(dev, micro_steps);
}

static int a4979_run(const struct device *dev, enum stepper_direction direction)
{
	struct a4979_data *data = dev->data;

	if (!data->enabled) {
		LOG_ERR("Failed to run stepper, device is not enabled");
		return -ECANCELED;
	}

	return step_dir_stepper_common_run(dev, direction);
}

static int a4979_init(const struct device *dev)
{
	const struct a4979_config *config = dev->config;
	struct a4979_data *data = dev->data;
	int ret;

	bool has_enable_pin = config->en_pin.port != NULL;
	bool has_reset_pin = config->reset_pin.port != NULL;

	LOG_DBG("Initializing %s gpios", dev->name);

	/* Configure reset pin if it is available */
	if (has_reset_pin) {
		if (!gpio_is_ready_dt(&config->reset_pin)) {
			LOG_ERR("Enable Pin is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_pin, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("%s: Failed to configure reset_pin (error: %d)", dev->name, ret);
			return ret;
		}
	}

	/* Configure enable pin if it is available */
	if (has_enable_pin) {
		if (!gpio_is_ready_dt(&config->en_pin)) {
			LOG_ERR("Enable Pin is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("%s: Failed to configure en_pin (error: %d)", dev->name, ret);
			return ret;
		}
	}

	/* Configure microstep pin 0 */
	if (!gpio_is_ready_dt(&config->m0_pin)) {
		LOG_ERR("m0 Pin is not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->m0_pin, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("%s: Failed to configure m0_pin (error: %d)", dev->name, ret);
		return ret;
	}

	/* Configure microstep pin 1 */
	if (!gpio_is_ready_dt(&config->m1_pin)) {
		LOG_ERR("m1 Pin is not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->m1_pin, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("%s: Failed to configure m1_pin (error: %d)", dev->name, ret);
		return ret;
	}

	ret = a4979_stepper_set_micro_step_res(dev, data->micro_step_res);
	if (ret != 0) {
		LOG_ERR("Failed to set micro step resolution: %d", ret);
		return ret;
	}

	ret = step_dir_stepper_common_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize common stepper data: %d", ret);
		return ret;
	}
	gpio_pin_set_dt(&config->common.step_pin, 0);

	return 0;
}

static DEVICE_API(stepper, a4979_stepper_api) = {
	.enable = a4979_stepper_enable,
	.disable = a4979_stepper_disable,
	.move_by = a4979_stepper_move_by,
	.move_to = a4979_move_to,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.set_microstep_interval = step_dir_stepper_common_set_microstep_interval,
	.run = a4979_run,
	.stop = step_dir_stepper_common_stop,
	.set_micro_step_res = a4979_stepper_set_micro_step_res,
	.get_micro_step_res = a4979_stepper_get_micro_step_res,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
};

#define A4979_DEVICE(inst)                                                                         \
                                                                                                   \
	static const struct a4979_config a4979_config_##inst = {                                   \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                           \
		.reset_pin = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                     \
		.m0_pin = GPIO_DT_SPEC_INST_GET(inst, m0_gpios),                                   \
		.m1_pin = GPIO_DT_SPEC_INST_GET(inst, m1_gpios),                                   \
	};                                                                                         \
                                                                                                   \
	static struct a4979_data a4979_data_##inst = {                                             \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                         \
		.micro_step_res = DT_INST_PROP(inst, micro_step_res),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, a4979_init, NULL, &a4979_data_##inst, &a4979_config_##inst,    \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY, &a4979_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(A4979_DEVICE)
