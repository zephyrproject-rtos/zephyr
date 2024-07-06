/*
 * Copyright (c) 2021 Innoseis B.V
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/types.h>

#define EEPROM_TMP116_SIZE (4 * sizeof(uint16_t))

int tmp116_eeprom_read(const struct device *dev, k_off_t offset, void *data,
		       size_t len);

int tmp116_eeprom_write(const struct device *dev, k_off_t offset,
			const void *data, size_t len);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP116_H_ */
