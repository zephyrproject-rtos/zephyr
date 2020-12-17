/*
 * Copyright (c) 2020 SER Consulting LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LM73C_H
#define _LM73C_H

#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

/* LM73 registers */
#define LM73_REG_TEMP   0x00
#define LM73_REG_CONF   0x01
#define LM73_REG_THIGH  0x02
#define LM73_REG_TLOW   0x03
#define LM73_REG_CTRL   0x04
#define LM73_REG_ID     0x07

#define LM73_ID         0x0190

enum temp_prec {
	LM73C_TEMP_PREC_11_BITS,
	LM73C_TEMP_PREC_12_BITS,
	LM73C_TEMP_PREC_13_BITS,
	LM73C_TEMP_PREC_14_BITS,
};

struct lm73c_data {
	const struct device *i2c_dev;
	int32_t temp;
};

struct lm73c_config {
	char *i2c_name;
	uint8_t i2c_address;
	enum temp_prec temp_bits;
};

#endif /* _LM73C_H */
