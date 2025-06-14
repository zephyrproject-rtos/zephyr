/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/rtk/rtk.h>

static K_SEM_DEFINE(semlock, 1, 1);

void rtk_publish_data(const struct rtk_data *data)
{
	(void)k_sem_take(&semlock, K_FOREVER);

	STRUCT_SECTION_FOREACH(rtk_data_callback, callback) {
		callback->callback(callback->dev, data);
	}

	k_sem_give(&semlock);
}
