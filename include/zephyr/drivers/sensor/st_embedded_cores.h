/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended Public API for STMicroelectronics embedded cores
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ST_EMBEDDED_CORES_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ST_EMBEDDED_CORES_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <zephyr/drivers/sensor.h>

enum sensor_channel_st_embedded_cores {
	SENSOR_CHAN_ST_ISPU = SENSOR_CHAN_PRIV_START,
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ST_EMBEDDED_CORES_H_ */
