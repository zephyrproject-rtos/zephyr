/*
 * Copyright (c) 2023 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>

static int sensor_decoders_init(void)
{
	STRUCT_SECTION_FOREACH(sensor_decoder_api, api) {
		k_object_access_all_grant(api);
	}
	return 0;
}

SYS_INIT(sensor_decoders_init, POST_KERNEL, 99);
