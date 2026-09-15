/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L5CX_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L5CX_H_

#include "vl53l5cx_api.h"
#include "vl53l5cx_platform.h"

struct vl53l5cx_data {
	VL53L5CX_Configuration vl53l5cx;
	VL53L5CX_ResultsData result_data;
#ifdef CONFIG_I2C_RTIO
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
#endif
	const struct device *dev;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L5CX_H_ */
