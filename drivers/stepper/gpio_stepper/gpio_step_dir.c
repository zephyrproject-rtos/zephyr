/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gpio_step_dir_controller

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/gpio.h>

#include <gpio_stepper_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_step_dir_controller, CONFIG_STEPPER_LOG_LEVEL);

struct zephyr_gpio_step_dir_controller_config {
	struct gpio_stepper_common_config common;
	const struct gpio_dt_spec step_pin;
	const struct gpio_dt_spec dir_pin;
	uint32_t step_width_ns;
	bool dual_edge;
};

struct zephyr_gpio_step_dir_controller_data {
	struct gpio_stepper_common_data common;
	atomic_t step_high;
};

GPIO_STEPPER_STRUCT_CHECK(struct zephyr_gpio_step_dir_controller_config,
			  struct zephyr_gpio_step_dir_controller_data);

static inline int update_dir_pin(const struct device *dev)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;
	int ret;

	switch (data->common.direction) {
	case STEPPER_DIRECTION_POSITIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 1 ^ config->common.invert_direction);
		break;
	case STEPPER_DIRECTION_NEGATIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 0 ^ config->common.invert_direction);
		break;
	default:
		LOG_ERR("Unsupported direction: %d", data->common.direction);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Failed to set direction: %d", ret);
		return ret;
	}

	return ret;
}

void stepper_handle_timing_signal(const struct device *dev)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;
	int ret;

	atomic_val_t step_pin_status = atomic_xor(&data->step_high, 1) ^ 1;

	ret = gpio_pin_set_dt(&config->step_pin, atomic_get(&step_pin_status));
	if (ret < 0) {
		LOG_ERR("Failed to set step pin: %d", ret);
		return;
	}

	if (!atomic_get(&step_pin_status) || config->dual_edge) {
		if (data->common.direction == STEPPER_DIRECTION_POSITIVE) {
			atomic_inc(&data->common.actual_position);
		} else {
			atomic_dec(&data->common.actual_position);
		}
	}

	switch (data->common.run_mode) {
	case STEPPER_RUN_MODE_POSITION:
		if (!data->step_high || config->dual_edge) {
			gpio_stepper_common_update_remaining_steps(dev);
		}
		gpio_stepper_common_position_mode_task(dev);
		break;
	case STEPPER_RUN_MODE_VELOCITY:
		gpio_stepper_common_velocity_mode_task(dev);
		break;
	default:
		LOG_WRN("Unsupported run mode: %d", data->common.run_mode);
		break;
	}
}

static int start_stepping(const struct device *dev)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;
	int ret;

	ret = config->common.timing_source->update(
		dev, config->dual_edge ? data->common.microstep_interval_ns
				       : data->common.microstep_interval_ns / 2);
	if (ret < 0) {
		LOG_ERR("Failed to update timing source: %d", ret);
		return ret;
	}

	ret = config->common.timing_source->start(dev);
	if (ret < 0) {
		LOG_ERR("Failed to start timing source: %d", ret);
		return ret;
	}

	stepper_handle_timing_signal(dev);
	return 0;
}

static int gpio_step_dir_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	int ret;

	if (data->common.microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	if (micro_steps == 0) {
		gpio_stepper_trigger_callback(data->common.dev, STEPPER_EVENT_STEPS_COMPLETED);
		config->common.timing_source->stop(dev);
		return 0;
	}

	K_SPINLOCK(&data->common.lock) {
		data->common.run_mode = STEPPER_RUN_MODE_POSITION;
		atomic_set(&data->common.step_count, micro_steps);
		gpio_stepper_common_update_direction_from_step_count(dev);
		ret = update_dir_pin(dev);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}

		ret = start_stepping(dev);
	}

	return ret;
}

static int gpio_step_dir_set_microstep_interval(const struct device *dev,
						const uint64_t microstep_interval_ns)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval cannot be zero");
		return -EINVAL;
	}

	if (config->dual_edge && (microstep_interval_ns < config->step_width_ns)) {
		LOG_ERR("Step interval too small for configured step width");
		return -EINVAL;
	}

	if (microstep_interval_ns < 2 * config->step_width_ns) {
		LOG_ERR("Step interval too small for configured step width");
		return -EINVAL;
	}

	K_SPINLOCK(&data->common.lock) {
		data->common.microstep_interval_ns = microstep_interval_ns;
		config->common.timing_source->update(
			dev, config->dual_edge ? data->common.microstep_interval_ns
					       : data->common.microstep_interval_ns / 2);
	}

	return 0;
}

int gpio_step_dir_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	struct gpio_stepper_common_data *data = dev->data;
	int ret;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		ret = update_dir_pin(dev);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}

		ret = start_stepping(dev);
	}

	return ret;
}

int gpio_step_dir_stepper_stop(const struct device *dev)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	struct zephyr_gpio_step_dir_controller_data *data = dev->data;
	int ret;

	ret = config->common.timing_source->stop(dev);
	if (ret != 0) {
		LOG_ERR("Failed to stop timing source: %d", ret);
		return ret;
	}

	if (!config->dual_edge && atomic_cas(&data->step_high, 1, 0)) {
		gpio_pin_set_dt(&config->step_pin, 0);
		/* If we are in the high state, we need to account for that step */
		if (data->common.direction == STEPPER_DIRECTION_POSITIVE) {
			atomic_inc(&data->common.actual_position);
		} else {
			atomic_dec(&data->common.actual_position);
		}
	}

	gpio_stepper_trigger_callback(dev, STEPPER_EVENT_STOPPED);

	return 0;
}

static int gpio_step_dir_controller_init(const struct device *dev)
{
	const struct zephyr_gpio_step_dir_controller_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->step_pin) || !gpio_is_ready_dt(&config->dir_pin)) {
		LOG_ERR("GPIO pins are not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->step_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure step pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	return gpio_stepper_common_init(dev);
}

static DEVICE_API(stepper, gpio_step_dir_stepper_api) = {
	.move_by = gpio_step_dir_move_by,
	.move_to = gpio_stepper_common_move_to,
	.is_moving = gpio_stepper_common_is_moving,
	.set_reference_position = gpio_stepper_common_set_reference_position,
	.get_actual_position = gpio_stepper_common_get_actual_position,
	.set_event_callback = gpio_stepper_common_set_event_callback,
	.set_microstep_interval = gpio_step_dir_set_microstep_interval,
	.run = gpio_step_dir_stepper_run,
	.stop = gpio_step_dir_stepper_stop,
};

#define ZEPHYR_GPIO_STEP_DIR_CONTROLLER_DEFINE(inst)                                               \
	static const struct zephyr_gpio_step_dir_controller_config gpio_step_dir_config_##inst = { \
		.common = GPIO_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                           \
		.common.timing_source_cb = stepper_handle_timing_signal,                           \
		.step_pin = GPIO_DT_SPEC_INST_GET(inst, step_gpios),                               \
		.dir_pin = GPIO_DT_SPEC_INST_GET(inst, dir_gpios),                                 \
		.step_width_ns = DT_INST_PROP(inst, step_width_ns),                                \
		.dual_edge = DT_INST_PROP_OR(inst, dual_edge_step, false),                         \
	};                                                                                         \
	static struct zephyr_gpio_step_dir_controller_data gpio_step_dir_data_##inst = {           \
		.common = GPIO_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                             \
		.step_high = ATOMIC_INIT(0),                                                       \
	};                                                                                         \
	static struct zephyr_gpio_step_dir_controller_data gpio_step_dir_data_##inst;              \
	DEVICE_DT_INST_DEFINE(inst, gpio_step_dir_controller_init, NULL,                           \
			      &gpio_step_dir_data_##inst, &gpio_step_dir_config_##inst,            \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,                           \
			      &gpio_step_dir_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_GPIO_STEP_DIR_CONTROLLER_DEFINE)
