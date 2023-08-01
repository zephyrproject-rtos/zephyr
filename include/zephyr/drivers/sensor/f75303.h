/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_F75303_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_F75303_H_

#include <zephyr/drivers/sensor.h>

/* F75303 specific channels */
enum sensor_channel_f75303 {
	SENSOR_CHAN_F75303_REMOTE1 = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_F75303_REMOTE2,
};

#endif
