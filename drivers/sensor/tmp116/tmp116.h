/*
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP116_TMP116_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP116_TMP116_H_

#define TMP116_REG_TEMP		0x0
#define TMP116_REG_CFGR		0x1
#define TMP116_REG_HIGH_LIM		0x2
#define TMP116_REG_LOW_LIM		0x3
#define TMP116_REG_EEPROM_UL		0x4
#define TMP116_REG_EEPROM1		0x5
#define TMP116_REG_EEPROM2		0x6
#define TMP116_REG_EEPROM3		0x7
#define TMP116_REG_EEPROM4		0x8
#define TMP116_REG_DEVICE_ID		0xF

#define TMP116_RESOLUTION		78125       /* in tens of uCelsius*/
#define TMP116_RESOLUTION_DIV		10000000

#define TMP116_DEVICE_ID		0x1116

struct tmp116_data {
	struct device *i2c;
	u16_t sample;
};

struct tmp116_dev_config {
	u16_t i2c_addr;
};

#endif /*  ZEPHYR_DRIVERS_SENSOR_TMP116_TMP116_H_ */
