/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OPT3001_H_
#define ZEPHYR_DRIVERS_SENSOR_OPT3001_H_

#include <zephyr/sys/util.h>

#define OPT3001_REG_RESULT 0x00
#define OPT3001_REG_CONFIG 0x01
#define OPT3001_REG_MANUFACTURER_ID 0x7E
#define OPT3001_REG_DEVICE_ID 0x7F

#define OPT3001_MANUFACTURER_ID_VALUE 0x5449
#define OPT3001_DEVICE_ID_VALUE 0x3001

#define OPT3001_CONVERSION_MODE_MASK (BIT(10) | BIT(9))
#define OPT3001_CONVERSION_MODE_CONTINUOUS (BIT(10) | BIT(9))

#define OPT3001_SAMPLE_EXPONENT_SHIFT 12
#define OPT3001_MANTISSA_MASK 0xfff

struct opt3001_data {
	const struct device *i2c;
	uint16_t sample;
};

#endif /* _SENSOR_OPT3001_ */
