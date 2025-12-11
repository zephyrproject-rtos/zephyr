/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/gnss/rtk/rtk.h>

void gnss_rtk_publish_data(const struct gnss_rtk_data *data)
{
	static K_SEM_DEFINE(publish_lock, 1, 1);

	(void)k_sem_take(&publish_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(gnss_rtk_data_callback, callback) {
		callback->callback(callback->dev, data);
	}

	k_sem_give(&publish_lock);
}
