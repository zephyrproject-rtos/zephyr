/*
 * Copyright (c) 2024 Vinicius Miguel
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAGNTEK_MT6701_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAGNTEK_MT6701_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_channel_mt6701 {
	SENSOR_CHAN_FIELD_STATUS = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_PUSH_STATUS,
	SENSOR_CHAN_LOSS_STATUS
};

#ifdef __cplusplus
}
#endif

#endif
