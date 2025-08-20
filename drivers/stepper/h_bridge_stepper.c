/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_h_bridge_stepper

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/stepper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(h_bridge_stepper, CONFIG_STEPPER_LOG_LEVEL);

#define MAX_MICRO_STEP_RES STEPPER_MICRO_STEP_2
#define NUM_CONTROL_PINS   4

static const uint8_t
	half_step_lookup_table[NUM_CONTROL_PINS * MAX_MICRO_STEP_RES][NUM_CONTROL_PINS] = {
		{1u, 1u, 0u, 0u}, {0u, 1u, 0u, 0u}, {0u, 1u, 1u, 0u}, {0u, 0u, 1u, 0u},
		{0u, 0u, 1u, 1u}, {0u, 0u, 0u, 1u}, {1u, 0u, 0u, 1u}, {1u, 0u, 0u, 0u}};

struct h_bridge_stepper_config {
	const struct gpio_dt_spec en_pin;
	const struct gpio_dt_spec *control_pins;
	bool invert_direction;
};

struct h_bridge_stepper_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	uint8_t step_gap;
	uint8_t coil_charge;
	struct k_work_delayable stepper_dwork;
	int32_t actual_position;
	uint64_t delay_in_ns;
	int32_t step_count;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

static int stepper_motor_set_coil_charge(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	const struct h_bridge_stepper_config *config = dev->config;
	int ret;

	for (int i = 0; i < NUM_CONTROL_PINS; i++) {
		ret = gpio_pin_set_dt(&config->control_pins[i],
				      half_step_lookup_table[data->coil_charge][i]);
		if (ret < 0) {
			LOG_ERR("Failed to set control pin %d: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

static void increment_coil_charge(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (data->coil_charge == NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap) {
		data->coil_charge = 0;
	} else {
		data->coil_charge = data->coil_charge + data->step_gap;
	}
}

static void decrement_coil_charge(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (data->coil_charge == 0) {
		data->coil_charge = NUM_CONTROL_PINS * MAX_MICRO_STEP_RES - data->step_gap;
	} else {
		data->coil_charge = data->coil_charge - data->step_gap;
	}
}

static void update_coil_charge(const struct device *dev)
{
	const struct h_bridge_stepper_config *config = dev->config;
	struct h_bridge_stepper_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		config->invert_direction ? decrement_coil_charge(dev) : increment_coil_charge(dev);
		data->actual_position++;
	} else if (data->direction == STEPPER_DIRECTION_NEGATIVE) {
		config->invert_direction ? increment_coil_charge(dev) : decrement_coil_charge(dev);
		data->actual_position--;
	}
}

static void update_remaining_steps(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (data->step_count > 0) {
		data->step_count--;
	} else if (data->step_count < 0) {
		data->step_count++;
	}
}

static void update_direction_from_step_count(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (data->step_count > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (data->step_count < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	} else {
		LOG_ERR("Step count is zero");
	}
}

static void position_mode_task(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	int ret;

	update_remaining_steps(dev);
	ret = stepper_motor_set_coil_charge(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set coil charge: %d", ret);
		return;
	}

	update_coil_charge(dev);
	if (data->step_count) {
		(void)k_work_reschedule(&data->stepper_dwork, K_NSEC(data->delay_in_ns));
	} else {
		if (data->callback) {
			data->callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED,
				       data->event_cb_user_data);
		}
		(void)k_work_cancel_delayable(&data->stepper_dwork);
	}
}

static void velocity_mode_task(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	int ret;

	ret = stepper_motor_set_coil_charge(dev);
	if (ret < 0) {
		LOG_ERR("Failed to set coil charge: %d", ret);
		return;
	}

	update_coil_charge(dev);
	(void)k_work_reschedule(&data->stepper_dwork, K_NSEC(data->delay_in_ns));
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct h_bridge_stepper_data *data =
		CONTAINER_OF(dwork, struct h_bridge_stepper_data, stepper_dwork);

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			position_mode_task(data->dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			velocity_mode_task(data->dev);
			break;
		default:
			LOG_WRN("Unsupported run mode %d", data->run_mode);
			break;
		}
	}
}

static int h_bridge_stepper_move_by(const struct device *dev, int32_t micro_steps)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (data->delay_in_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	if (micro_steps == 0) {
		(void)k_work_cancel_delayable(&data->stepper_dwork);
		if (data->callback) {
			data->callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED,
				       data->event_cb_user_data);
		}
		return 0;
	}
	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		update_direction_from_step_count(dev);
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int h_bridge_stepper_set_reference_position(const struct device *dev, int32_t position)
{
	struct h_bridge_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
	}
	return 0;
}

static int h_bridge_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	struct h_bridge_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int h_bridge_stepper_move_to(const struct device *dev, int32_t micro_steps)
{
	struct h_bridge_stepper_data *data = dev->data;
	int32_t steps_to_move;

	K_SPINLOCK(&data->lock) {
		steps_to_move = micro_steps - data->actual_position;
	}
	return h_bridge_stepper_move_by(dev, steps_to_move);
}

static int h_bridge_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	struct h_bridge_stepper_data *data = dev->data;

	*is_moving = k_work_delayable_is_pending(&data->stepper_dwork);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

static int h_bridge_stepper_set_microstep_interval(const struct device *dev,
						   uint64_t microstep_interval_ns)
{
	struct h_bridge_stepper_data *data = dev->data;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval is invalid.");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->delay_in_ns = microstep_interval_ns;
	}
	LOG_DBG("Setting Motor step interval to %llu", microstep_interval_ns);
	return 0;
}

static int h_bridge_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	struct h_bridge_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		(void)k_work_reschedule(&data->stepper_dwork, K_NO_WAIT);
	}
	return 0;
}

static int h_bridge_stepper_set_micro_step_res(const struct device *dev,
					       enum stepper_micro_step_resolution micro_step_res)
{
	struct h_bridge_stepper_data *data = dev->data;
	int err = 0;

	K_SPINLOCK(&data->lock) {
		switch (micro_step_res) {
		case STEPPER_MICRO_STEP_1:
		case STEPPER_MICRO_STEP_2:
			data->step_gap = MAX_MICRO_STEP_RES >> (micro_step_res - 1);
			break;
		default:
			LOG_ERR("Unsupported micro step resolution %d", micro_step_res);
			err = -ENOTSUP;
		}
	}
	return err;
}

static int h_bridge_stepper_get_micro_step_res(const struct device *dev,
					       enum stepper_micro_step_resolution *micro_step_res)
{
	struct h_bridge_stepper_data *data = dev->data;
	*micro_step_res = MAX_MICRO_STEP_RES >> (data->step_gap - 1);
	return 0;
}

static int h_bridge_stepper_set_event_callback(const struct device *dev,
					       stepper_event_callback_t callback, void *user_data)
{
	struct h_bridge_stepper_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->callback = callback;
		data->event_cb_user_data = user_data;
	}
	return 0;
}

static int h_bridge_stepper_enable(const struct device *dev)
{
	const struct h_bridge_stepper_config *config = dev->config;
	struct h_bridge_stepper_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		if (config->en_pin.port != NULL) {
			err = gpio_pin_set_dt(&config->en_pin, 1);
		} else {
			LOG_DBG("No en_pin detected");
			err = -ENOTSUP;
		}
	}
	return err;
}

static int h_bridge_stepper_disable(const struct device *dev)
{
	const struct h_bridge_stepper_config *config = dev->config;
	struct h_bridge_stepper_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		if (config->en_pin.port != NULL) {
			err = gpio_pin_set_dt(&config->en_pin, 0);
		} else {
			LOG_DBG("No en_pin detected, power stages will not be turned off if "
				"stepper is in motion");
			err = -ENOTSUP;
		}
	}
	return err;
}

static int h_bridge_stepper_stop(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	int err;

	K_SPINLOCK(&data->lock) {
		err = k_work_cancel_delayable(&data->stepper_dwork);

		if (data->callback && !err) {
			data->callback(data->dev, STEPPER_EVENT_STOPPED, data->event_cb_user_data);
		}
	}
	return err;
}

static int h_bridge_stepper_init(const struct device *dev)
{
	struct h_bridge_stepper_data *data = dev->data;
	const struct h_bridge_stepper_config *config = dev->config;
	int err;

	data->dev = dev;
	LOG_DBG("Initializing %s h_bridge_stepper with %d pin", dev->name, NUM_CONTROL_PINS);
	for (uint8_t n_pin = 0; n_pin < NUM_CONTROL_PINS; n_pin++) {
		(void)gpio_pin_configure_dt(&config->control_pins[n_pin], GPIO_OUTPUT_INACTIVE);
	}

	if (config->en_pin.port != NULL) {
		if (!gpio_is_ready_dt(&config->en_pin)) {
			LOG_ERR("Enable Pin is not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->en_pin, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: Failed to configure en_pin (error: %d)", dev->name, err);
			return err;
		}
	}

	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);
	return 0;
}

static DEVICE_API(stepper, h_bridge_stepper_api) = {
	.enable = h_bridge_stepper_enable,
	.disable = h_bridge_stepper_disable,
	.set_micro_step_res = h_bridge_stepper_set_micro_step_res,
	.get_micro_step_res = h_bridge_stepper_get_micro_step_res,
	.set_reference_position = h_bridge_stepper_set_reference_position,
	.get_actual_position = h_bridge_stepper_get_actual_position,
	.set_event_callback = h_bridge_stepper_set_event_callback,
	.set_microstep_interval = h_bridge_stepper_set_microstep_interval,
	.move_by = h_bridge_stepper_move_by,
	.move_to = h_bridge_stepper_move_to,
	.run = h_bridge_stepper_run,
	.stop = h_bridge_stepper_stop,
	.is_moving = h_bridge_stepper_is_moving,
};

#define H_BRIDGE_STEPPER_DEFINE(inst)                                                              \
	static const struct gpio_dt_spec h_bridge_stepper_motor_control_pins_##inst[] = {          \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),          \
	};                                                                                         \
	BUILD_ASSERT(ARRAY_SIZE(h_bridge_stepper_motor_control_pins_##inst) == 4,                  \
		     "h_bridge stepper driver currently supports only 4 wire configuration");      \
	static const struct h_bridge_stepper_config h_bridge_stepper_config_##inst = {             \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                           \
		.invert_direction = DT_INST_PROP(inst, invert_direction),                          \
		.control_pins = h_bridge_stepper_motor_control_pins_##inst};                       \
	static struct h_bridge_stepper_data h_bridge_stepper_data_##inst = {                       \
		.step_gap = MAX_MICRO_STEP_RES >> (DT_INST_PROP(inst, micro_step_res) - 1),        \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, micro_step_res) <= STEPPER_MICRO_STEP_2,                   \
		     "h_bridge stepper driver supports up to 2 micro steps");                      \
	DEVICE_DT_INST_DEFINE(inst, h_bridge_stepper_init, NULL, &h_bridge_stepper_data_##inst,    \
			      &h_bridge_stepper_config_##inst, POST_KERNEL,                        \
			      CONFIG_STEPPER_INIT_PRIORITY, &h_bridge_stepper_api);

DT_INST_FOREACH_STATUS_OKAY(H_BRIDGE_STEPPER_DEFINE)
