/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_FIXTURE_H_
#define TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_FIXTURE_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>

struct bmi160_fixture {
	const struct device *dev_spi;
	const struct device *dev_i2c;
	const struct emul *emul_spi;
	const struct emul *emul_i2c;
};

#endif /* TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_FIXTURE_H_ */
