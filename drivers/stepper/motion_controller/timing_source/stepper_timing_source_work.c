/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *					 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_timing_source.h"

#include "zephyr/kernel.h"

static k_timeout_t stepper_movement_delay(const uint64_t interval_ns)
{
	if (interval_ns == 0) {
		return K_FOREVER;
	}

	return K_NSEC(interval_ns);
}

static void stepper_work_step_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	const struct stepper_timing_source_work_data *work_data =
		CONTAINER_OF(dwork, struct stepper_timing_source_work_data, dwork);

	const struct stepper_timing_source *timing_source = work_data->timing_source;

	timing_source->callback(timing_source->user_data);
}

int step_work_timing_source_init(struct stepper_timing_source *timing_source)
{
	struct stepper_timing_source_work_data *data = timing_source->data;

	k_work_init_delayable(&data->dwork, stepper_work_step_handler);

	return 0;
}

int step_work_timing_source_start(const struct stepper_timing_source *timing_source,
				  const uint64_t interval)
{
	struct stepper_timing_source_work_data *data = timing_source->data;

	return k_work_reschedule(&data->dwork, stepper_movement_delay(interval));
}

int step_work_timing_source_stop(const struct stepper_timing_source *timing_source)
{
	struct stepper_timing_source_work_data *data = timing_source->data;

	return k_work_cancel_delayable(&data->dwork);
}

const struct stepper_timing_source_api stepper_timing_source_work_api = {
	.init = step_work_timing_source_init,
	.start = step_work_timing_source_start,
	.stop = step_work_timing_source_stop,
};
