/*
 * Copyright (c) 2020 Innoseis BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP112_TMP112_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP112_TMP112_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define TMP112_REG_TEMPERATURE          0x00
#define TMP112_DATA_INVALID_BIT         (BIT(1) | BIT(2))
#define TMP112_DATA_EXTENDED            (BIT(0))
#define TMP112_DATA_EXTENDED_SHIFT      3
#define TMP112_DATA_NORMAL_SHIFT        4


#define TMP112_REG_CONFIG   0x01
#define TMP112_CONFIG_EM    BIT(4)

#define TMP112_ALERT_EN_BIT     BIT(5)
#define TMP112_CONV_RATE_SHIFT  6
#define TMP112_CONV_RATE_MASK   (BIT_MASK(2) <<  TMP112_CONV_RATE_SHIFT)
#define TMP112_CONV_RATE_025    0
#define TMP112_CONV_RATE_1000   1
#define TMP112_CONV_RATE_4      2
#define TMP112_CONV_RATE_8      3

#define TMP112_CONV_RATE(cr)    ((cr) << TMP112_CONV_RATE_SHIFT)

#define TMP112_CONV_RES_SHIFT   13
#define TMP112_CONV_RES_MASK    (BIT_MASK(2) << TMP112_CONV_RES_SHIFT)

#define TMP112_ONE_SHOT         BIT(15)

#define TMP112_REG_TLOW         0x02
#define TMP112_REG_THIGH        0x03

/* scale in micro degrees Celsius */
#define TMP112_TEMP_SCALE       62500

struct tmp112_data {
	int16_t sample;
	uint16_t config_reg;
};

struct tmp112_config {
	const struct i2c_dt_spec bus;
	uint8_t cr;
	bool extended_mode : 1;
};

#endif
