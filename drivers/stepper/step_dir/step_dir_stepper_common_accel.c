/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_common_accel.h"
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(step_dir_stepper_accel, CONFIG_STEPPER_LOG_LEVEL);

#define PSEC_PER_NSEC 1000

/** number of picoseconds per micorsecond */
#define PSEC_PER_USEC (PSEC_PER_NSEC * NSEC_PER_USEC)

/** number of picoseconds per second */
#define PSEC_PER_SEC ((uint64_t)PSEC_PER_USEC * (uint64_t)USEC_PER_SEC)

#define NSEC_SQUARED (uint64_t)NSEC_PER_SEC *NSEC_PER_SEC

STEP_DIR_TIMING_SOURCE_STRUCT_CHECK(struct step_dir_stepper_common_accel_data);

static int step_dir_stepper_accel_phase_steps_adjusting(const struct device *dev,
							uint32_t *accel_steps,
							uint32_t *const_steps,
							uint32_t *decel_steps, uint32_t total_steps,
							uint64_t start_interval,
							uint64_t const_interval, uint32_t index)
{
	/* Catch edge case*/
	if (total_steps == 1) {
		*accel_steps = 1;
		*const_steps = 0;
		*decel_steps = 0;
		return 0;
	}

	/* Decelerate at start */
	if (const_interval > start_interval && start_interval != 0) {
		/* If stop cant be reached from start velocity with straight deceleration, throw
		 * error */
		if (*decel_steps + (index - *accel_steps) > total_steps) {
			LOG_ERR("%s: Total step count is to low, it is %u, but needs to be at "
				"least %u.",
				dev->name, total_steps, *decel_steps + (index - *accel_steps));
			return -EINVAL;
		}
		/* Else simply set last value */
		else {
			*const_steps = total_steps - *decel_steps - (index - *accel_steps);
		}
	}
	/* Accalerate at start*/
	else {
		/* If enough steps available, simply set last value */
		if (*decel_steps + (*accel_steps - index) <= total_steps) {
			*const_steps = total_steps - *decel_steps - (*accel_steps - index);
		} else {
			/* If stop cant be reached from start velocity with straight deceleration,
			 * throw error */
			if (index > total_steps) {
				LOG_ERR("%s: Index %u greater than total steps %u", dev->name,
					index, total_steps);
				return -EINVAL;
			} else {
				uint32_t remain_steps = total_steps - index;
				*accel_steps = index + (remain_steps / 2);
				*decel_steps = *accel_steps;
				if (remain_steps % 2 == 1) {
					*const_steps = 1;
				}
			}
		}
	}

	return 0;
}

static int step_dir_stepper_accel_calculate_acceleration(const struct device *dev, uint32_t steps,
							 uint64_t microstep_interval, bool run)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	/* Algorithm uses sec for acceleration time, adjusts acceleration for current microstep
	 * resolution
	 */
	uint64_t nsec_squared = (uint64_t)NSEC_PER_SEC * NSEC_PER_SEC;

	/* Split total steps into steps for the three phases */
	uint32_t accel_steps = 0;

	if ((data->current_interval >= microstep_interval || data->current_interval == 0) &&
	    data->acceleration != 0) {
		accel_steps =
			DIV_ROUND_UP(nsec_squared, (2 * data->acceleration * microstep_interval *
						    microstep_interval)); /* steps */
	} else if (data->acceleration != 0) {
		accel_steps = nsec_squared / (2 * data->acceleration * microstep_interval *
					      microstep_interval); /* steps */
	}

	uint32_t decel_steps = accel_steps; /* steps */
	uint32_t const_steps = 0;           /* steps */
	uint32_t step_index = 0;

	/* Determine position of current velocity on the acceleration ramp*/
	if (data->current_interval != 0) {
		step_index = (nsec_squared) / (2 * data->acceleration * data->current_interval *
					       data->current_interval);
	}

	if (!run && data->acceleration != 0) {
		int ret = step_dir_stepper_accel_phase_steps_adjusting(
			dev, &accel_steps, &const_steps, &decel_steps, steps,
			data->current_interval, microstep_interval, step_index);
		if (ret != 0) {
			return ret;
		}
	} else {
		const_steps = 10; /* Dummy value for correct behaviour */
	}

	if (data->microstep_interval_ns > data->current_interval && data->current_interval != 0) {
		uint32_t temp = accel_steps;
		accel_steps = step_index;
		step_index = temp;
	}

	/* Configure interrupt data */
	data->accel_steps = accel_steps;
	data->const_steps = const_steps;
	data->decel_steps = decel_steps;
	data->step_index = step_index;

	return 0;
}

static int step_dir_stepper_accel_set_direction(const struct device *dev)
{
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	struct step_dir_stepper_common_accel_data *data = dev->data;
	int ret;

	switch (data->direction) {
	case STEPPER_DIRECTION_POSITIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 1);
		break;
	case STEPPER_DIRECTION_NEGATIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 0);
		break;
	default:
		LOG_ERR("Unsupported direction: %d", data->direction);
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Failed to set direction: %d", ret);
		return ret;
	}
	return 0;
}

static inline int step_dir_stepper_accel_perform_step(const struct device *dev)
{
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	struct step_dir_stepper_common_accel_data *data = dev->data;
	int ret;

	ret = gpio_pin_toggle_dt(&config->step_pin);
	if (ret < 0) {
		LOG_ERR("Failed to toggle step pin: %d", ret);
		return ret;
	}
	data->step_pin_low = !data->step_pin_low;

	if (!data->step_pin_low || config->dual_edge) {
		if (data->direction == STEPPER_DIRECTION_POSITIVE) {
			data->actual_position++;
		} else {
			data->actual_position--;
		}
	} 
	if (data->step_pin_low || config->dual_edge) {
		data->step_index++;
	}

	/* After acceleration is finished, switch to constant velocity */
	if (data->step_index == data->accel_steps && data->acceleration != 0) {
		data->timing_data.start_microstep_interval = data->microstep_interval_ns;
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns;
		config->timing_source->update(dev, &data->timing_data);

		/* Enter Deceleration phase*/
	} else if (data->step_index == data->accel_steps + data->const_steps &&
		   data->run_mode == STEPPER_RUN_MODE_POSITION && data->acceleration != 0) {
		/* Use slightly larger microstep interval to signal deceleration to the timing
		 * source.
		 */
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns + 1;
		config->timing_source->update(dev, &data->timing_data);
	}

	return 0;
}

static void stepper_accel_trigger_callback(const struct device *dev, enum stepper_event event)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
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
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
static void stepper_accel_work_event_handler(struct k_work *work)
{
	struct step_dir_stepper_common_accel_data *data =
		CONTAINER_OF(work, struct step_dir_stepper_common_accel_data, event_callback_work);
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
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

static void accel_update_remaining_steps(struct step_dir_stepper_common_accel_data *data)
{
	if (data->step_count > 0) {
		data->step_count--;
	} else if (data->step_count < 0) {
		data->step_count++;
	}
}

static int accel_update_direction_from_step_count(const struct device *dev)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;

	if (data->step_count > 0) {
		if (data->direction == STEPPER_DIRECTION_POSITIVE ||
		    !config->timing_source->is_running(dev) || data->acceleration == 0) {
			data->direction = STEPPER_DIRECTION_POSITIVE;
		} else {
			LOG_ERR("Cant change direction while moving");
			return -ENOTSUP;
		}

	} else if (data->step_count < 0) {
		if (data->direction == STEPPER_DIRECTION_NEGATIVE ||
		    !config->timing_source->is_running(dev) || data->acceleration == 0) {
			data->direction = STEPPER_DIRECTION_NEGATIVE;
		} else {
			LOG_ERR("Cant change direction while moving");
			return -ENOTSUP;
		}
	} else {
		LOG_ERR("Step count is zero");
		return -EINVAL;
	}
	return 0;
}

static void accel_position_mode_task(const struct device *dev)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = data->dev->config;
	int ret = 0;

	if (data->step_count || !data->step_pin_low) {
		ret = step_dir_stepper_accel_perform_step(dev);
	}

	if ((!data->step_pin_low || config->dual_edge) && ret == 0) {
		accel_update_remaining_steps(dev->data);
	} 
	if (((data->step_pin_low && ret == 0) || config->dual_edge) && data->step_count == 0) {
		config->timing_source->stop(data->dev);
		data->current_interval = 0;
		data->timing_data.start_microstep_interval = 0;
		ret = gpio_pin_set_dt(&config->step_pin, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set step pin low: %d", ret);
		}
		data->step_pin_low = true;
		if (data->stopping) {
			stepper_accel_trigger_callback(data->dev, STEPPER_EVENT_STOPPED);
		} else {
			stepper_accel_trigger_callback(data->dev, STEPPER_EVENT_STEPS_COMPLETED);
		}
	}
}

static void accel_velocity_mode_task(const struct device *dev)
{
	(void)step_dir_stepper_accel_perform_step(dev);
}

void stepper_handle_timing_signal_accel(const struct device *dev)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		switch (data->run_mode) {
		case STEPPER_RUN_MODE_POSITION:
			accel_position_mode_task(dev);
			break;
		case STEPPER_RUN_MODE_VELOCITY:
			accel_velocity_mode_task(dev);
			break;
		default:
			LOG_WRN("Unsupported run mode: %d", data->run_mode);
			break;
		}
	}
}

int step_dir_stepper_common_accel_init(const struct device *dev)
{
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	struct step_dir_stepper_common_accel_data *data = dev->data;
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
	data->step_pin_low = true;

	ret = gpio_pin_configure_dt(&config->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	data->timing_data.acceleration = data->acceleration;
	data->timing_data.start_microstep_interval = 0;

	ret = config->timing_source->register_step_handler(dev, stepper_handle_timing_signal_accel);
	if (ret < 0) {
		LOG_ERR("Failed to register step handler: %d", ret);
		return ret;
	}

	if (config->timing_source->init) {
		ret = config->timing_source->init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize timing source: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS

	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_event),
		    CONFIG_STEPPER_STEP_DIR_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, stepper_accel_work_event_handler);
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

	return 0;
}

int step_dir_stepper_common_accel_positioning(const struct device *dev, int32_t micro_steps)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->step_pin, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set step pin low: %d", ret);
		return -EIO;
	}
	data->step_pin_low = true;

	data->current_interval = config->timing_source->get_current_interval(dev);
	data->timing_data.start_microstep_interval = data->current_interval;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	ret = step_dir_stepper_accel_calculate_acceleration(dev, abs(micro_steps),
							    data->microstep_interval_ns, false);
	if (ret != 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns;
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->step_count = micro_steps;
		config->timing_source->update(dev, &data->timing_data);
		ret = accel_update_direction_from_step_count(dev);
		if (ret == 0) {
			step_dir_stepper_accel_set_direction(dev);
			config->timing_source->start(dev);
			data->stopping = false;
		}
	}

	return ret;
}

int step_dir_stepper_common_accel_move_by(const struct device *dev, const int32_t micro_steps)
{
	int ret = 0;

	ret = step_dir_stepper_common_accel_positioning(dev, micro_steps);

	return ret;
}

int step_dir_stepper_common_accel_set_microstep_interval(const struct device *dev,
							 const uint64_t microstep_interval_ns)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	int ret = 0;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval cannot be zero");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->microstep_interval_ns = microstep_interval_ns;
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns;
	}

	if (data->run_mode == STEPPER_RUN_MODE_VELOCITY && config->timing_source->is_running(dev)) {
		ret = step_dir_stepper_common_accel_run(dev, data->direction);
	} else if (data->run_mode == STEPPER_RUN_MODE_POSITION &&
		   config->timing_source->is_running(dev)) {
		ret = step_dir_stepper_common_accel_move_by(dev, data->step_count);
	}

	return ret;
}

int step_dir_stepper_common_accel_set_reference_position(const struct device *dev,
							 const int32_t value)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->actual_position = value;
	}

	return 0;
}

int step_dir_stepper_common_accel_get_actual_position(const struct device *dev, int32_t *value)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		*value = data->actual_position;
	}

	return 0;
}

int step_dir_stepper_common_accel_move_to(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	int ret = 0;

	ret = step_dir_stepper_common_accel_positioning(dev, value - data->actual_position);

	return ret;
}

int step_dir_stepper_common_accel_is_moving(const struct device *dev, bool *is_moving)
{
	const struct step_dir_stepper_common_accel_config *config = dev->config;

	*is_moving = config->timing_source->is_running(dev);
	return 0;
}

int step_dir_stepper_common_accel_run(const struct device *dev,
				      const enum stepper_direction direction)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	int ret;

	data->current_interval = config->timing_source->get_current_interval(dev);
	data->timing_data.start_microstep_interval = data->current_interval;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}
	ret = gpio_pin_set_dt(&config->step_pin, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set step pin low: %d", ret);
		return -EIO;
	}
	data->step_pin_low = true;

	int32_t steps = 0;

	if (data->acceleration != 0 && data->current_interval != 0) {
		steps = NSEC_SQUARED /
			(data->current_interval * data->current_interval * data->acceleration);
	}

	ret = step_dir_stepper_accel_calculate_acceleration(dev, 0, data->microstep_interval_ns,
							    true);
	if (ret != 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns;
		data->run_mode = STEPPER_RUN_MODE_VELOCITY;
		if (data->direction == direction || !config->timing_source->is_running(dev)) {
			data->direction = direction;
			config->timing_source->update(dev, &data->timing_data);
			step_dir_stepper_accel_set_direction(dev);
			config->timing_source->start(dev);
			data->stopping = false;
		} else {
			LOG_ERR("Cant change direction while moving");
			ret = -ENOTSUP;
		}
	}

	return ret;
}

int step_dir_stepper_common_accel_set_event_callback(const struct device *dev,
						     stepper_event_callback_t callback,
						     void *user_data)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;
	return 0;
}

int step_dir_stepper_common_accel_update_acceleraion(const struct device *dev,
						     uint32_t acceleration)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;

	data->acceleration = acceleration;
	data->timing_data.acceleration = data->acceleration;

	return 0;
}

int step_dir_stepper_common_accel_stop(const struct device *dev)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	int ret;

	/* If not moving: trigger only callback */
	if (!config->timing_source->is_running(dev)) {
		stepper_accel_trigger_callback(dev, STEPPER_EVENT_STOPPED);
		return 0;
	}

	/* If in const velocity mode: stop immediatly */
	if (data->acceleration == 0) {
		data->timing_data.start_microstep_interval = 0;
		ret = config->timing_source->stop(dev);
		if (ret == 0) {
			stepper_accel_trigger_callback(dev, STEPPER_EVENT_STOPPED);
			return 0;
		} else {
			return -EIO;
		}
	}

	data->current_interval = config->timing_source->get_current_interval(dev);
	data->timing_data.start_microstep_interval = data->current_interval;

	int32_t steps = 0;
	steps = NSEC_SQUARED /
		(data->current_interval * data->current_interval * data->acceleration * 2);

	ret = step_dir_stepper_accel_calculate_acceleration(dev, steps, 0, false);
	if (ret != 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		/* Use slightly larger microstep interval to signal deceleration to the timing
		 * source.
		 */
		data->timing_data.microstep_interval_ns = data->microstep_interval_ns + 1;
		data->run_mode = STEPPER_RUN_MODE_POSITION;
		data->stopping = true;
		data->step_count = steps;

		config->timing_source->update(dev, &data->timing_data);
		config->timing_source->start(dev);
	}

	return ret;
}

int step_dir_stepper_common_accel_stop_immediate(const struct device *dev)
{
	struct step_dir_stepper_common_accel_data *data = dev->data;
	const struct step_dir_stepper_common_accel_config *config = dev->config;
	int ret;

	data->timing_data.start_microstep_interval = 0;
	ret = config->timing_source->stop(dev);
	if (ret == 0) {
		stepper_accel_trigger_callback(dev, STEPPER_EVENT_STOPPED);
		return 0;
	} else {
		return -EIO;
	}
}
