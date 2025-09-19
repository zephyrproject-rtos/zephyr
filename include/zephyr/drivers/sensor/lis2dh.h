/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_

#include <zephyr/drivers/sensor.h>

enum lis2dh_self_test {
	LIS2DH_SELF_TEST_DISABLE = 0,
	LIS2DH_SELF_TEST_POSITIVE = 1,
	LIS2DH_SELF_TEST_NEGATIVE = 2,
};

enum sensor_attribute_lis2dh {
	SENSOR_ATTR_LIS2DH_SELF_TEST = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_ */
