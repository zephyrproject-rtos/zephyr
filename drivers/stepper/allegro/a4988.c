/*
 * Copyright (c) 2024 Armin Kessler
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_a4988

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include "../step_dir/step_dir_stepper_common.h"

LOG_MODULE_REGISTER(a4988, CONFIG_STEPPER_LOG_LEVEL);

#define NUM_MICRO_STEP_PINS 3
#define MSX_PIN_COUNT       3

struct a4988_config {
	const struct step_dir_stepper_common_config common;
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec sleep_pin;
	const struct gpio_dt_spec reset_pin;
	const struct gpio_dt_spec *msx_pins;
};

struct a4988_data {
	const struct step_dir_stepper_common_data common;
	enum stepper_micro_step_resolution micro_step_res;
	bool enabled;
};
static int a4988_stepper_set_micro_step_res(const struct device *dev,
					    const enum stepper_micro_step_resolution micro_step_res)
{
	struct a4988_data *data = dev->data;
	const struct a4988_config *config = dev->config;

	uint8_t msx = 0;

	if (!config->msx_pins) {
		LOG_ERR("Micro step pins not defined");
		return -ENODEV;
	}

	switch (micro_step_res) {
	case STEPPER_MICRO_STEP_1:
		msx = 0;
		break;
	case STEPPER_MICRO_STEP_2:
		msx = 1;
		break;
	case STEPPER_MICRO_STEP_4:
		msx = 2;
		break;
	case STEPPER_MICRO_STEP_8:
		msx = 3;
		break;
	case STEPPER_MICRO_STEP_16:
		msx = 7;
		break;
	default:
		LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
		return -ENOTSUP;
	}

	gpio_pin_set_dt(&config->msx_pins[0], msx & 0x01);
	gpio_pin_set_dt(&config->msx_pins[1], (msx & 0x02) >> 1);
	gpio_pin_set_dt(&config->msx_pins[2], (msx & 0x04) >> 2);

	data->micro_step_res = micro_step_res;
	return 0;
}

static int a4988_stepper_get_micro_step_res(const struct device *dev,
					    enum stepper_micro_step_resolution *micro_step_res)
{
	struct a4988_data *data = dev->data;

	*micro_step_res = data->micro_step_res;
	return 0;
}

static int a4988_stepper_enable(const struct device *dev, bool enable)
{
	const struct a4988_config *config = dev->config;
	struct a4988_data *data = dev->data;
	bool has_enable_pin = config->en_pin.port != NULL;
	bool has_sleep_pin = config->sleep_pin.port != NULL;
	bool has_reset_pin = config->reset_pin.port != NULL;

	if (has_enable_pin) {
		if(gpio_pin_set_dt(&config->en_pin, (int)enable)) {
			LOG_ERR("Failed to set enable pin");
			return -EIO;
		}
	}

	if (has_sleep_pin) {
		if(gpio_pin_set_dt(&config->sleep_pin, (int)!enable)) {
			LOG_ERR("Failed to set sleep pin");
			return -EIO;
		}
	}

	if (has_reset_pin) {
		if(gpio_pin_set_dt(&config->sleep_pin, (int)!enable)) {
			LOG_ERR("Failed to set sleep pin");
			return -EIO;
		}
	}
	
	data->enabled = enable;
	if (!enable) {
		config->common.timing_source->stop(dev);
		if(gpio_pin_set_dt(&config->common.step_pin, 0)) {
			LOG_ERR("Failed to set step pin");
			return -EIO;
		}
	}
	return 0;
}

static int a4988_init(const struct device *dev)
{
	struct a4988_data *data = dev->data;
	const struct a4988_config *config = dev->config;
	int ret;
	bool has_enable_pin = config->en_pin.port != NULL;
	bool has_sleep_pin = config->sleep_pin.port != NULL;
	bool has_reset_pin = config->reset_pin.port != NULL;

	LOG_DBG("Initializing %s gpios", dev->name);
	
	/* Configure sleep pin if it is available */
	if (has_sleep_pin) {
		if (!gpio_is_ready_dt(&config->sleep_pin)) {
			LOG_ERR("Sleep pin is not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->sleep_pin, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("%s: Failed to configure sleep_pin (error: %d)", dev->name, ret);
			return ret;
		}
	}

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

	if (config->msx_pins) {
		for (size_t i = 0; i < NUM_MICRO_STEP_PINS; i++) {
			if (!gpio_is_ready_dt(&config->msx_pins[i])) {
				LOG_ERR("Micro step ping %d is not ready", i);
				return -ENODEV;
			}

			ret = gpio_pin_configure_dt(&config->msx_pins[i], GPIO_OUTPUT_INACTIVE);
			if (ret != 0) {
				LOG_ERR("Failed to configure msx pin %d (error: %d)", i, ret);
				return ret;
			}
		}
		a4988_stepper_set_micro_step_res(dev, data->micro_step_res);
	}

	ret = step_dir_stepper_common_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize common stepper data: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(stepper, a4988_stepper_api) = {
	.enable = a4988_stepper_enable,
	.move_by = step_dir_stepper_common_move_by,
	.move_to = step_dir_stepper_common_move_to,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = step_dir_stepper_common_move_to,
	.set_max_velocity = step_dir_stepper_common_set_max_velocity,
	.run = step_dir_stepper_common_run,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = a4988_stepper_set_micro_step_res,
	.get_micro_step_res = a4988_stepper_get_micro_step_res
};

#define A4988_DEVICE(inst)                                                                           \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios), (                                       \
	static const struct gpio_dt_spec a4988_stepper_msx_pins_##inst[] = {                     \
		DT_INST_FOREACH_PROP_ELEM_SEP(                                                     \
			inst, msx_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)                              \
		),                                                                                 \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(a4988_stepper_msx_pins_##inst) == MSX_PIN_COUNT,                      \
		"Two microstep config pins needed");                                               \
	)) \
                                                                                                     \
	static const struct a4988_config a4988_config_##inst = {                                     \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                         \
		.sleep_pin = GPIO_DT_SPEC_INST_GET_OR(inst, sleep_gpios, {0}),                       \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                             \
		.reset_pin = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                             \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),				   \
		(.msx_pins = a4988_stepper_msx_pins_##inst)) };       \
                                                                                                     \
	static struct a4988_data a4988_data_##inst = {                                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                           \
		.micro_step_res = DT_INST_PROP(inst, micro_step_res),                                \
	};                                                                                           \
                                                                                                     \
	DEVICE_DT_INST_DEFINE(inst, a4988_init, NULL, &a4988_data_##inst, &a4988_config_##inst,      \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY, &a4988_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(A4988_DEVICE)
