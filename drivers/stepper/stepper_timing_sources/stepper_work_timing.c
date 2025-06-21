/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys_clock.h>

#include "stepper_timing_source.h"

static k_timeout_t stepper_movement_delay(const struct timing_source_data *data)
{
	if (data->microstep_interval_ns == 0) {
		return K_FOREVER;
	}

	return K_NSEC(data->microstep_interval_ns);
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct timing_source_data *data =
		CONTAINER_OF(dwork, struct timing_source_data, stepper_dwork);

	data->stepper_handle_timing_signal_cb(data->motion_control_dev);
}

int step_work_timing_source_init(const struct timing_source_config *config,
				 struct timing_source_data *data)
{
	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);

	return 0;
}

int step_work_timing_source_update(const struct timing_source_config *config,
				   struct timing_source_data *data,
				   const uint64_t microstep_interval_ns)
{
	ARG_UNUSED(config);
	ARG_UNUSED(data);
	ARG_UNUSED(microstep_interval_ns);
	return 0;
}

int step_work_timing_source_start(const struct timing_source_config *config,
				  struct timing_source_data *data)
{
	ARG_UNUSED(config);
	return k_work_reschedule(&data->stepper_dwork, stepper_movement_delay(data));
}

int step_work_timing_source_stop(const struct timing_source_config *config,
				 struct timing_source_data *data)
{
	return k_work_cancel_delayable(&data->stepper_dwork);
}

bool step_work_timing_source_needs_reschedule(const struct device *dev)
{
	ARG_UNUSED(dev);
	return true;
}

bool step_work_timing_source_is_running(const struct timing_source_config *config,
					struct timing_source_data *data)
{
	return k_work_delayable_is_pending(&data->stepper_dwork);
}

const struct stepper_timing_source_api step_work_timing_source_api = {
	.init = step_work_timing_source_init,
	.update = step_work_timing_source_update,
	.start = step_work_timing_source_start,
	.needs_reschedule = step_work_timing_source_needs_reschedule,
	.stop = step_work_timing_source_stop,
	.is_running = step_work_timing_source_is_running,
};
