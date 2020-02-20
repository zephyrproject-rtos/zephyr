/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OPT3001_H_
#define ZEPHYR_DRIVERS_SENSOR_OPT3001_H_

#include <sys/util.h>

#define OPT3001_REG_RESULT 0x00
#define OPT3001_REG_CONFIG 0x01
#define OPT3001_REG_MANUFACTURER_ID 0x7E
#define OPT3001_REG_DEVICE_ID 0x7F

#define OPT3001_MANUFACTURER_ID_VALUE 0x5449
#define OPT3001_DEVICE_ID_VALUE 0x3001

#define OPT3001_CONVERSION_TIME             BIT(11)
#define OPT3001_CONVERSION_MODE_MASK        (BIT(10) | BIT(9))
#define OPT3001_CONVERSION_MODE_CONTINUOUS  (BIT(10) | BIT(9))
#define OPT3001_OVERFLOW                    BIT(8)
#define OPT3001_CONVERSION_READY            BIT(7)
#define OPT3001_FLAG_HIGH                   BIT(6)
#define OPT3001_FLAG_LOW                    BIT(5)
#define OPT3001_LATCH                       BIT(4)
#define OPT3001_POLARITY                    BIT(3)
#define OPT3001_MASK_EXPONENT               BIT(2)
#define OPT3001_FAULT_COUNT                 (BIT(1) | BIT(0))


#define OPT3001_SAMPLE_EXPONENT_SHIFT 12
#define OPT3001_MANTISSA_MASK 0xfff

#define OPT3001_STARTUP_TIME_USEC     1000

struct opt3001_data {
	struct device *i2c;
	u16_t sample;
};

enum opt3001_sensor_attributes {
	SENSOR_ATTR_FAULT_COUNT = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_LATCH,
};

#endif /* _SENSOR_OPT3001_ */
