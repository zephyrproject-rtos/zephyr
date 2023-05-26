/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_tcs3400 {
	/** RGBC Integration Cycles */
	SENSOR_ATTR_TCS3400_INTEGRATION_CYCLES = SENSOR_ATTR_PRIV_START,
	/**
	 * If set the clear channel reports the measurement from the IR
	 * sensor.
	 */
	SENSOR_ATTR_TCS3400_IR_ENABLE,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_ */
