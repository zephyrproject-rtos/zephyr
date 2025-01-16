/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_timing_source.h"
#include "step_dir_stepper_common.h"

static k_timeout_t stepper_movement_delay(const struct device *dev)
{
	const struct step_dir_stepper_common_data *data = dev->data;

	if (data->microstep_interval_ns == 0) {
		return K_FOREVER;
	}

	return K_NSEC(data->microstep_interval_ns);
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct step_dir_stepper_common_data *data =
		CONTAINER_OF(dwork, struct step_dir_stepper_common_data, stepper_dwork);

	stepper_handle_timing_signal(data->dev);
}

int step_work_timing_source_init(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);

	return 0;
}

int step_work_timing_source_update(const struct device *dev, const uint64_t microstep_interval_ns)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(microstep_interval_ns);
	return 0;
}

int step_work_timing_source_start(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	return k_work_reschedule(&data->stepper_dwork, stepper_movement_delay(dev));
}

int step_work_timing_source_stop(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

	return k_work_cancel_delayable(&data->stepper_dwork);
}

bool step_work_timing_source_needs_reschedule(const struct device *dev)
{
	ARG_UNUSED(dev);
	return true;
}

bool step_work_timing_source_is_running(const struct device *dev)
{
	struct step_dir_stepper_common_data *data = dev->data;

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
