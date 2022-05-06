/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for MH-Z19B CO2 Sensor
 *
 * Some capabilities and operational requirements for this sensor
 * cannot be expressed within the sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_mhz19b {
	/** Automatic Baseline Correction Self Calibration Function. */
	SENSOR_ATTR_MHZ19B_ABC = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_ */
