/**
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_stepper_motion_control

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>

#include "stepper_timing_sources/stepper_timing_source.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_motion_control, CONFIG_STEPPER_LOG_LEVEL);

struct stepper_motion_control_config {
	const struct device *stepper;
	const struct timing_source_config timing_config;
	const struct stepper_timing_source_api *timing_source;
};

struct stepper_motion_control_data {
	const struct device *dev;
	struct k_spinlock lock;
	int32_t step_count;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	int32_t actual_position;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
	struct timing_source_data timing_data;
#ifdef CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS
	struct k_work event_callback_work;
	struct k_msgq event_msgq;
	uint8_t event_msgq_buffer[CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_EVENT_QUEUE_LEN *
				  sizeof(enum stepper_event)];
#endif /* CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS */
};

void stepper_trigger_callback(const struct device *dev, enum stepper_event event)
{
	struct stepper_motion_control_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS
	/* Dispatch to msgq instead of raising directly */
	int ret = k_msgq_put(&data->event_msgq, &event, K_NO_WAIT);

	if (ret != 0) {
		LOG_WRN("Failed to put event in msgq: %d", ret);
	}

	ret = k_work_submit(&data->event_callback_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit work item: %d", ret);
	}
#else
	LOG_WRN_ONCE("Event callback called from ISR context without ISR safe events enabled");
#endif /* CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS
static void stepper_work_event_handler(struct k_work *work)
{
	struct stepper_motion_control_data *data =
		CONTAINER_OF(work, struct stepper_motion_control_data, event_callback_work);
	enum stepper_event event;
	int ret;

	ret = k_msgq_get(&data->event_msgq, &event, K_NO_WAIT);
	if (ret != 0) {
		return;
	}

	/* Run the callback */
	if (data->callback != NULL) {
		data->callback(data->dev, event, data->event_cb_user_data);
	}

	/* If there are more pending events, resubmit this work item to handle them */
	if (k_msgq_num_used_get(&data->event_msgq) > 0) {
		k_work_submit(work);
	}
}
#endif /* CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS */

static void update_direction_from_step_count(const struct device *dev)
{
	struct stepper_motion_control_data *data = dev->data;

	if (data->step_count > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (data->step_count < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	} else {
		LOG_ERR("Step count is zero");
	}
}

static int z_stepper_motion_control_move_by(const struct device *dev, int32_t micro_steps)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	if (data->timing_data.microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	if (micro_steps == 0) {
		stepper_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
		config->timing_source->stop(&config->timing_config, &data->timing_data);
		return 0;
	}

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		config->timing_source->update(&config->timing_config, &data->timing_data,
					      data->timing_data.microstep_interval_ns);
		update_direction_from_step_count(dev);
		stepper_drv_set_direction(config->stepper, data->direction);
		config->timing_source->start(&config->timing_config, &data->timing_data);
	}
	return 0;
}

static int z_stepper_motion_control_move_to(const struct device *dev, int32_t micro_steps)
{
	struct stepper_motion_control_data *data = dev->data;
	int32_t steps_to_move;

	K_SPINLOCK(&data->lock) {
		steps_to_move = micro_steps - data->actual_position;
	}
	return z_stepper_motion_control_move_by(dev, steps_to_move);
}

static int z_stepper_motion_control_run(const struct device *dev,
					const enum stepper_direction direction)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		data->direction = direction;
		config->timing_source->update(&config->timing_config, &data->timing_data,
					      data->timing_data.microstep_interval_ns);
		stepper_drv_set_direction(config->stepper, direction);
		config->timing_source->start(&config->timing_config, &data->timing_data);
	}
	return 0;
}

static int z_stepper_motion_control_stop(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;
	int ret;

	ret = config->timing_source->stop(&config->timing_config, &data->timing_data);
	if (ret != 0) {
		LOG_ERR("Failed to stop timing source: %d", ret);
		return ret;
	}
	stepper_trigger_callback(dev, STEPPER_EVENT_STOPPED);

	return 0;
}

static int z_stepper_motion_control_set_reference_position(const struct device *dev,
							   int32_t position)
{
	struct stepper_motion_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = position;
	}
	return 0;
}

static int z_stepper_motion_control_get_actual_position(const struct device *dev, int32_t *position)
{
	struct stepper_motion_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*position = data->actual_position;
	}
	return 0;
}

static int z_stepper_motion_control_set_step_interval(const struct device *dev,
						      uint64_t microstep_interval_ns)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval is invalid.");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->timing_data.microstep_interval_ns = microstep_interval_ns;
		config->timing_source->update(&config->timing_config, &data->timing_data,
					      microstep_interval_ns);
	}
	LOG_DBG("Setting Motor step interval to %llu", microstep_interval_ns);
	return 0;
}

static int z_stepper_motion_control_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	*is_moving = config->timing_source->is_running(&config->timing_config, &data->timing_data);
	LOG_DBG("Motor is %s moving", *is_moving ? "" : "not");
	return 0;
}

static void update_remaining_steps(const struct device *dev)
{
	struct stepper_motion_control_data *data = dev->data;

	if (data->step_count > 0) {
		data->step_count--;
	} else if (data->step_count < 0) {
		data->step_count++;
	}
}

static void update_actual_position(const struct device *dev)
{
	struct stepper_motion_control_data *data = dev->data;

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		data->actual_position++;
	} else {
		data->actual_position--;
	}
}

static void position_mode_task(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	stepper_drv_step(config->stepper);
	update_remaining_steps(dev);
	update_actual_position(dev);

	if (config->timing_source->needs_reschedule(dev) && data->step_count != 0) {
		config->timing_source->start(&config->timing_config, &data->timing_data);
	} else if (data->step_count == 0) {
		config->timing_source->stop(&config->timing_config, &data->timing_data);
		stepper_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
	}
}

static void velocity_mode_task(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;

	stepper_drv_step(config->stepper);
	update_actual_position(dev);

	if (config->timing_source->needs_reschedule(dev)) {
		(void)config->timing_source->start(&config->timing_config, &data->timing_data);
	}
}

static int z_stepper_motion_control_set_event_callback(const struct device *dev,
						       stepper_event_callback_t cb, void *user_data)
{
	struct stepper_motion_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->callback = cb;
		data->event_cb_user_data = user_data;
	}
	return 0;
}

void stepper_handle_timing_signal(const struct device *dev)
{
	struct stepper_motion_control_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			position_mode_task(dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			velocity_mode_task(dev);
			break;
		default:
			LOG_WRN("Unsupported run mode: %d", data->run_mode);
			break;
		}
	}
}

static int z_stepper_motion_control_enable(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;

	return stepper_drv_enable(config->stepper);
}

static int z_stepper_motion_control_disable(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;

	return stepper_drv_disable(config->stepper);
}

static int z_stepper_motion_control_set_micro_step_res(const struct device *dev,
						       const enum stepper_micro_step_resolution res)
{
	const struct stepper_motion_control_config *config = dev->config;

	return stepper_drv_set_micro_stepper_res(config->stepper, res);
}

static int z_stepper_motion_control_get_micro_step_res(const struct device *dev,
						       enum stepper_micro_step_resolution *res)
{
	const struct stepper_motion_control_config *config = dev->config;

	return stepper_drv_get_micro_step_res(config->stepper, res);
}

static int stepper_motion_control_init(const struct device *dev)
{
	const struct stepper_motion_control_config *config = dev->config;
	struct stepper_motion_control_data *data = dev->data;
	int ret;

	data->dev = dev;

	__ASSERT_NO_MSG(config->timing_source->init != NULL);

	ret = config->timing_source->init(&config->timing_config, &data->timing_data);
	if (ret < 0) {
		LOG_ERR("Failed to initialize timing source: %d", ret);
		return ret;
	}

#ifdef CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS

	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_event),
		    CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, stepper_work_event_handler);
#endif /* CONFIG_ZEPHYR_STEPPER_MOTION_CONTROL_GENERATE_ISR_SAFE_EVENTS */

	LOG_DBG("Stepper Control initialized for stepper driver %s", config->stepper->name);
	return 0;
}

static DEVICE_API(stepper, zephyr_stepper_motion_control_api) = {
	.enable = z_stepper_motion_control_enable,
	.disable = z_stepper_motion_control_disable,
	.set_micro_step_res = z_stepper_motion_control_set_micro_step_res,
	.get_micro_step_res = z_stepper_motion_control_get_micro_step_res,
	.move_to = z_stepper_motion_control_move_to,
	.move_by = z_stepper_motion_control_move_by,
	.run = z_stepper_motion_control_run,
	.set_microstep_interval = z_stepper_motion_control_set_step_interval,
	.get_actual_position = z_stepper_motion_control_get_actual_position,
	.set_reference_position = z_stepper_motion_control_set_reference_position,
	.is_moving = z_stepper_motion_control_is_moving,
	.stop = z_stepper_motion_control_stop,
	.set_event_callback = z_stepper_motion_control_set_event_callback,
};

#define STEPPER_MOTION_CONTROL_DEFINE(inst)                                                        \
	static const struct stepper_motion_control_config stepper_motion_control_config_##inst = { \
		.stepper = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(inst), stepper)),                  \
		.timing_config.counter =                                                           \
			DEVICE_DT_GET_OR_NULL(DT_PHANDLE(DT_DRV_INST(inst), counter)),             \
		.timing_source = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, counter),                 \
			(&step_counter_timing_source_api), (&step_work_timing_source_api)),        \
	};                                                                                         \
	struct stepper_motion_control_data stepper_motion_control_data_##inst = {                  \
		.timing_data.motion_control_dev = DEVICE_DT_GET(DT_DRV_INST(inst)),                \
		.timing_data.stepper_handle_timing_signal_cb = stepper_handle_timing_signal,       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, stepper_motion_control_init, NULL,                             \
			      &stepper_motion_control_data_##inst,                                 \
			      &stepper_motion_control_config_##inst, POST_KERNEL,                  \
			      CONFIG_STEPPER_INIT_PRIORITY, &zephyr_stepper_motion_control_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_MOTION_CONTROL_DEFINE)
