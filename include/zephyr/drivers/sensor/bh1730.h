/*
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BH1730_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BH1730_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for BH1730
 */
enum sensor_attribute_bh1730 {
	/**
	 * Sensor integration time in ms
	 *
	 * 3 - 688ms
	 */
	SENSOR_ATTR_BH1730_SENSOR_INTEGRATION_TIME = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BH1730_H_ */
