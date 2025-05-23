/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_

#include <zephyr/sys/util_macro.h>

#define TMP11X_REG_TEMP		0x0
#define TMP11X_REG_CFGR		0x1
#define TMP11X_REG_HIGH_LIM		0x2
#define TMP11X_REG_LOW_LIM		0x3
#define TMP11X_REG_EEPROM_UL		0x4
#define TMP11X_REG_EEPROM1		0x5
#define TMP11X_REG_EEPROM2		0x6
#define TMP11X_REG_EEPROM3		0x7
#define TMP117_REG_TEMP_OFFSET		0x7
#define TMP11X_REG_EEPROM4		0x8
#define TMP11X_REG_DEVICE_ID		0xF

#define TMP11X_RESOLUTION		78125       /* in tens of uCelsius*/
#define TMP11X_RESOLUTION_DIV		10000000

#define TMP116_DEVICE_ID		0x1116
#define TMP117_DEVICE_ID		0x0117
#define TMP119_DEVICE_ID		0x2117

#define TMP11X_CFGR_AVG			(BIT(5) | BIT(6))
#define TMP11X_CFGR_CONV		(BIT(7) | BIT(8) | BIT(9))
#define TMP11X_CFGR_MODE		(BIT(10) | BIT(11))
#define TMP11X_CFGR_DATA_READY  BIT(13)
#define TMP11X_EEPROM_UL_UNLOCK BIT(15)
#define TMP11X_EEPROM_UL_BUSY   BIT(14)

#define TMP11X_AVG_1_SAMPLE		0
#define TMP11X_AVG_8_SAMPLES	BIT(5)
#define TMP11X_AVG_32_SAMPLES	BIT(6)
#define TMP11X_AVG_64_SAMPLES	(BIT(5) | BIT(6))
#define TMP11X_MODE_CONTINUOUS	0
#define TMP11X_MODE_SHUTDOWN	BIT(10)
#define TMP11X_MODE_ONE_SHOT	(BIT(10) | BIT(11))

struct tmp11x_data {
	uint16_t sample;
	uint16_t id;
};

struct tmp11x_dev_config {
	struct i2c_dt_spec bus;
	uint16_t odr;
	uint16_t oversampling;
};

#endif /*  ZEPHYR_DRIVERS_SENSOR_TMP11X_TMP11X_H_ */
