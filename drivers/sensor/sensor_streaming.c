/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

__weak int sensor_submit_work(struct k_work *work)
{
	__ASSERT_NO_MSG(work != NULL);
	return k_work_submit(work);
}
