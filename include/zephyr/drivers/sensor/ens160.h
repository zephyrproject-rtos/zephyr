/*
 * Copyright (c) 2024 Gustavo Silva
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ENS160_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ENS160_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/* Channel for Air Quality Index */
enum sensor_channel_ens160 {
	SENSOR_CHAN_ENS160_AQI = SENSOR_CHAN_PRIV_START,
};

enum sensor_attribute_ens160 {
	SENSOR_ATTR_ENS160_TEMP = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_ENS160_RH,
};

#ifdef __cplusplus
}
#endif

#endif
