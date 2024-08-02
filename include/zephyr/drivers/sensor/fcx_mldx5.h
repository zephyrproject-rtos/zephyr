/*
 * Copyright (c) 2024, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_fcx_mldx5 {
	SENSOR_ATTR_FCX_MLDX5_STATUS = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_FCX_MLDX5_RESET,
};

enum fcx_mldx5_status {
	FCX_MLDX5_STATUS_STANDBY = 2,
	FCX_MLDX5_STATUS_RAMP_UP = 3,
	FCX_MLDX5_STATUS_RUN = 4,
	FCX_MLDX5_STATUS_ERROR = 5,
	/* Unknown is not sent by the sensor */
	FCX_MLDX5_STATUS_UNKNOWN,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_ */
