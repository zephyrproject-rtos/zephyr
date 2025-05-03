/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_COMMON_H_

/**
 * @brief Stepper Motor run mode options
 */
enum stepper_run_mode {
	/** Hold Mode */
	STEPPER_RUN_MODE_HOLD = 0,
	/** Position Mode*/
	STEPPER_RUN_MODE_POSITION = 1,
	/** Velocity Mode */
	STEPPER_RUN_MODE_VELOCITY = 2,
};

struct stepper_common_data {
	const struct device *dev;
	struct k_spinlock lock;
	enum stepper_direction direction;
	enum stepper_run_mode run_mode;
	struct k_work_delayable stepper_dwork;
	int32_t actual_position;
	uint64_t delay_in_ns;
	int32_t step_count;
	bool is_enabled;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

static inline void update_remaining_steps(struct stepper_common_data *data)
{
	if (data->step_count > 0) {
		data->step_count--;
	} else if (data->step_count < 0) {
		data->step_count++;
	}
}

static inline void update_direction_from_step_count(struct stepper_common_data *data)
{
	if (data->step_count > 0) {
		data->direction = STEPPER_DIRECTION_POSITIVE;
	} else if (data->step_count < 0) {
		data->direction = STEPPER_DIRECTION_NEGATIVE;
	}
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_COMMON_H_ */
