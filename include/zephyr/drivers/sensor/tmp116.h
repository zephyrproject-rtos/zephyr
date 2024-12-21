/*
 * Copyright (c) 2021 Innoseis B.V
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <sys/types.h>

enum sensor_attribute_tmp_116 {
	/** Turn on power saving/one shot mode */
	SENSOR_ATTR_TMP116_ONE_SHOT_MODE = SENSOR_ATTR_PRIV_START,
	/** Shutdown the sensor */
	SENSOR_ATTR_TMP116_SHUTDOWN_MODE,
	/** Turn on continuous conversion */
	SENSOR_ATTR_TMP116_CONTINUOUS_CONVERSION_MODE,
};

#define EEPROM_TMP116_SIZE (4 * sizeof(uint16_t))

int tmp116_eeprom_read(const struct device *dev, off_t offset, void *data,
		       size_t len);

int tmp116_eeprom_write(const struct device *dev, off_t offset,
			const void *data, size_t len);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_ */
