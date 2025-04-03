/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include "step_dir_stepper_timing_source.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(step_dir_stepper);

#define PSEC_PER_NSEC (uint64_t)1000

/** number of picoseconds per micorsecond */
#define PSEC_PER_USEC (PSEC_PER_NSEC * (uint64_t)NSEC_PER_USEC)

/** number of picoseconds per second */
#define PSEC_PER_SEC ((uint64_t)PSEC_PER_USEC * (uint64_t)USEC_PER_SEC)

#define NSEC_SQUARED (uint64_t)NSEC_PER_SEC *NSEC_PER_SEC

struct step_counter_accel_update_data {
	uint64_t microstep_interval_ns;
	uint64_t start_microstep_interval;
	uint32_t acceleration;
};

/* Integer sqrt algorithm using a binary digit-by-digit algorithm */
uint32_t sqrt_64(uint64_t num)
{
	uint32_t sq_root = 0;
	uint32_t add_bit = 1;
	uint32_t a;
	uint64_t a_2;
	add_bit <<= 31;

	while (add_bit > 0) {
		a = sq_root + add_bit;
		a_2 = a;
		a_2 *= a;
		if (num >= a_2) {
			sq_root = a;
		}
		add_bit >>= 1;
	}
	return sq_root;
}

static uint64_t sqrt_index_diff(uint32_t index_1, uint32_t index_2, uint64_t root_factor)
{
	/* Use root_factor to increase accuracy while still using integers, note that the result is
	 * increased by root_factor
	 */
	return sqrt_64(index_1 * root_factor * root_factor) -
	       sqrt_64(index_2 * root_factor * root_factor);
}

static void step_counter_accel_timing_source_positioning_constant(const struct device *dev,
								  void *user_data)
{
	ARG_UNUSED(dev);
	struct step_counter_accel_data *data = user_data;

	data->handler(data->dev);
}

static void step_counter_accel_timing_source_positioning_acceleration(const struct device *dev,
								      void *user_data)
{
	struct step_counter_accel_data *data = user_data;
	uint64_t new_time_int;

	if (data->flip_state && !data->dual_edge) {
		data->handler(data->dev);
		data->flip_state = !data->flip_state;

	} else {
		/* Use Approximation once error is small enough*/
		if (data->pulse_index >= data->accurate_steps && data->accurate_steps != 0) {
			/*
			 * Iterative algorithm using Taylor expansion, see 'Generate stepper-motor
			 * speed profiles in real time' (2005) by David Austin
			 * Added 0.5 to n equivalent for better acceleration behaviour
			 */
			uint64_t t_n_1 = data->current_time_int;
			uint64_t adjust = 2 * t_n_1 / (4 * data->pulse_index + 3);
			new_time_int = t_n_1 - adjust;
		} else {
			/* Use accurate (but expensive) calculation when error would be large
			 * Added 0.5 to n equivalent for better acceleration behaviour
			 */
			new_time_int = data->base_time_int *
				       sqrt_index_diff(data->pulse_index + 1, data->pulse_index,
						       data->root_factor) /
				       data->root_factor;
		}
		data->counter_top_cfg.ticks = DIV_ROUND_UP((data->frequency / MSEC_PER_SEC) *
								   (new_time_int / PSEC_PER_NSEC),
							   NSEC_PER_MSEC);
		if (!data->dual_edge) {
			data->counter_top_cfg.ticks = data->counter_top_cfg.ticks / 2;
		}
		data->current_time_int = new_time_int;
		data->current_interval = new_time_int / PSEC_PER_NSEC;

		data->handler(data->dev);
		counter_set_top_value(data->counter, &data->counter_top_cfg);
		data->pulse_index++;
		data->flip_state = !data->flip_state;
	}
}

static void step_counter_accel_timing_source_positioning_deceleration(const struct device *dev,
								      void *user_data)
{
	struct step_counter_accel_data *data = user_data;
	uint64_t new_time_int;

	if (data->flip_state && !data->dual_edge) {
		data->handler(data->dev);
		data->flip_state = !data->flip_state;

	} else {
		/* Use Approximation as long as error is small enough */
		if (data->pulse_index >= data->accurate_steps && data->accurate_steps != 0) {
			/*
			 * Iterative algorithm using Taylor expansion, see 'Generate stepper-motor
			 * speed profiles in real time' (2005) by David Austin
			 * Added 0.5 to n equivalent for better acceleration behaviour
			 */
			uint64_t t_n = data->current_time_int;
			uint64_t adjust = 2 * t_n / (4 * data->pulse_index + 1);
			new_time_int = t_n + adjust;
		} else {
			/* Use accurate (but expensive) calculation when error would be large
			 * Added 0.5 to n equivalent for better acceleration behaviour
			 */
			new_time_int = data->base_time_int *
				       sqrt_index_diff(data->pulse_index, data->pulse_index - 1,
						       data->root_factor) /
				       data->root_factor;
		}
		data->counter_top_cfg.ticks = DIV_ROUND_UP((data->frequency / MSEC_PER_SEC) *
								   (new_time_int / PSEC_PER_NSEC),
							   NSEC_PER_MSEC);
		if (!data->dual_edge) {
			data->counter_top_cfg.ticks = data->counter_top_cfg.ticks / 2;
		}
		data->current_time_int = new_time_int;
		data->current_interval = new_time_int / PSEC_PER_NSEC;
		data->handler(data->dev);
		counter_set_top_value(data->counter, &data->counter_top_cfg);
		if (data->pulse_index != 0) {
			data->pulse_index--;
		}
		data->flip_state = !data->flip_state;
	}
}

int step_counter_accel_timing_source_update(const struct device *dev, void *update_data)
{
	struct step_counter_accel_data *data = dev->data;
	struct step_counter_accel_update_data *u_data = update_data;
	int ret;

	data->current_interval = u_data->start_microstep_interval;

	if (u_data->acceleration == 0 ||
	    u_data->microstep_interval_ns == u_data->start_microstep_interval) {
		data->base_time_int = u_data->microstep_interval_ns * PSEC_PER_NSEC;
		data->counter_top_cfg.callback =
			step_counter_accel_timing_source_positioning_constant;
	} else {

		uint64_t base_time =
			sqrt_64((uint64_t)2 * PSEC_PER_SEC / (uint64_t)u_data->acceleration) *
			(uint64_t)PSEC_PER_USEC;
		data->base_time_int = base_time; /* in ps */

		/* Determine current position on the acceleration ramp*/
		if (u_data->start_microstep_interval != 0) {
			data->pulse_index = (NSEC_SQUARED) / (2 * u_data->acceleration *
							      u_data->start_microstep_interval *
							      u_data->start_microstep_interval);
		} else {
			data->pulse_index = 0;
		}

		/* Calculate time for first step interval*/
		data->current_time_int = (data->base_time_int *
					  sqrt_index_diff(data->pulse_index + 1, data->pulse_index,
							  data->root_factor)) /
					 data->root_factor;

		if ((u_data->microstep_interval_ns < u_data->start_microstep_interval &&
		     u_data->microstep_interval_ns != 0) ||
		    u_data->start_microstep_interval == 0) {
			data->counter_top_cfg.callback =
				step_counter_accel_timing_source_positioning_acceleration;
		} else {
			data->counter_top_cfg.callback =
				step_counter_accel_timing_source_positioning_deceleration;
		}
	}

	if (u_data->start_microstep_interval == 0 && u_data->acceleration != 0) {
		/* Delay until first step, setting this value to low causes some counters to not
		 * work correctly, uses half interval of target speed*/
		data->counter_top_cfg.ticks = DIV_ROUND_UP(counter_get_frequency(data->counter) *
								   u_data->microstep_interval_ns,
							   2 * NSEC_PER_SEC);
	} else if (u_data->acceleration != 0) {
		data->counter_top_cfg.ticks =
			DIV_ROUND_UP(counter_get_frequency(data->counter) * data->current_interval,
				     NSEC_PER_SEC);
	} else {
		data->counter_top_cfg.ticks = DIV_ROUND_UP(counter_get_frequency(data->counter) *
								   u_data->microstep_interval_ns,
							   NSEC_PER_SEC);
	}

	if (!data->dual_edge) {
		data->counter_top_cfg.ticks = data->counter_top_cfg.ticks / 2;
	}

	/* Lock interrupts while modifying counter settings */
	int key = irq_lock();

	ret = counter_set_top_value(data->counter, &data->counter_top_cfg);
	data->frequency = counter_get_frequency(data->counter);

	irq_unlock(key);

	if (ret != 0) {
		LOG_ERR("%s: Failed to set counter top value (error: %d)", dev->name, ret);
		return ret;
	}

	return 0;
}

int step_counter_accel_timing_source_start(const struct device *dev)
{
	struct step_counter_accel_data *data = dev->data;
	int ret;

	ret = counter_start(data->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start counter: %d", ret);
		return ret;
	}

	data->counter_running = true;

	return 0;
}

int step_counter_accel_timing_source_stop(const struct device *dev)
{
	struct step_counter_accel_data *data = dev->data;
	int ret;

	ret = counter_stop(data->counter);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to stop counter: %d", ret);
		return ret;
	}

	data->counter_running = false;
	data->current_interval = 0;
	data->flip_state = true;

	return 0;
}

bool step_counter_accel_timing_source_needs_reschedule(const struct device *dev)
{
	ARG_UNUSED(dev);
	return false;
}

bool step_counter_accel_timing_source_is_running(const struct device *dev)
{
	struct step_counter_accel_data *data = dev->data;

	return data->counter_running;
}

uint64_t step_counter_accel_timing_source_get_current_interval(const struct device *dev)
{

	struct step_counter_accel_data *data = dev->data;

	return step_counter_accel_timing_source_is_running(dev) ? data->current_interval : 0;
}

int step_counter_accel_timing_source_register_handler(const struct device *dev,
						      step_dir_step_handler handler)
{
	struct step_counter_accel_data *data = dev->data;

	data->handler = handler;

	return 0;
}

int step_counter_accel_timing_source_init(const struct device *dev)
{
	struct step_counter_accel_data *data = dev->data;

	if (!device_is_ready(data->counter)) {
		LOG_ERR("Counter device is not ready");
		return -ENODEV;
	}

	data->counter_top_cfg.callback = step_counter_accel_timing_source_positioning_constant;
	data->counter_top_cfg.user_data = data;
	data->counter_top_cfg.flags = 0;
	data->counter_top_cfg.ticks = counter_us_to_ticks(data->counter, 1000000);
	data->flip_state = true;

	return 0;
}

const struct stepper_timing_source_api step_counter_accel_timing_source_api = {
	.init = step_counter_accel_timing_source_init,
	.update = step_counter_accel_timing_source_update,
	.start = step_counter_accel_timing_source_start,
	.stop = step_counter_accel_timing_source_stop,
	.is_running = step_counter_accel_timing_source_is_running,
	.get_current_interval = step_counter_accel_timing_source_get_current_interval,
	.register_step_handler = step_counter_accel_timing_source_register_handler,
};
