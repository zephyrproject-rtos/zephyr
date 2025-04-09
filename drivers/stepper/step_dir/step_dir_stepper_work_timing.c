/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_timing_source.h"
#include "step_dir_stepper_common.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(step_dir_stepper);

struct step_work_update_data {
	uint64_t microstep_interval_ns;
};

static k_timeout_t stepper_movement_delay(const struct device *dev)
{
	const struct step_work_data *data = dev->data;

	if (data->microstep_interval_ns == 0) {
		return K_FOREVER;
	}

	return K_NSEC(data->microstep_interval_ns);
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct step_work_data *data = CONTAINER_OF(dwork, struct step_work_data, stepper_dwork);

	data->handler(data->dev);

	k_work_reschedule(&data->stepper_dwork, stepper_movement_delay(data->dev));
}

int step_work_timing_source_init(const struct device *dev)
{
	struct step_work_data *data = dev->data;

	k_work_init_delayable(&data->stepper_dwork, stepper_work_step_handler);

	return 0;
}

int step_work_timing_source_update(const struct device *dev, void *update_data)
{
	struct step_work_update_data *u_data = update_data;
	struct step_work_data *data = dev->data;

	data->microstep_interval_ns = u_data->microstep_interval_ns;
	return k_work_reschedule(&data->stepper_dwork, stepper_movement_delay(dev));
}

int step_work_timing_source_start(const struct device *dev)
{
	struct step_work_data *data = dev->data;

	return k_work_reschedule(&data->stepper_dwork, stepper_movement_delay(dev));
}

int step_work_timing_source_stop(const struct device *dev)
{
	struct step_work_data *data = dev->data;

	return k_work_cancel_delayable(&data->stepper_dwork);
}

bool step_work_timing_source_is_running(const struct device *dev)
{
	struct step_work_data *data = dev->data;

	return k_work_delayable_is_pending(&data->stepper_dwork);
}

uint64_t step_work_timing_source_get_current_interval(const struct device *dev)
{

	struct step_work_data *data = dev->data;

	return step_work_timing_source_is_running(dev) ? data->microstep_interval_ns : 0;
}

int step_work_timing_register_handler(const struct device *dev, step_dir_step_handler handler)
{
	struct step_work_data *data = dev->data;

	data->handler = handler;

	return 0;
}

const struct stepper_timing_source_api step_work_timing_source_api = {
	.init = step_work_timing_source_init,
	.update = step_work_timing_source_update,
	.start = step_work_timing_source_start,
	.stop = step_work_timing_source_stop,
	.is_running = step_work_timing_source_is_running,
	.get_current_interval = step_work_timing_source_get_current_interval,
	.register_step_handler = step_work_timing_register_handler,
};
