/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_
#define ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_

#include "si1133_regs.h"

#include <zephyr/types.h>
#include <device.h>
#include <gpio.h>
#include <sensor.h>



enum si1133_channels {

	SI1133_CHANNEL_HIGH_VIS,
	SI1133_CHANNEL_LOW_VIS,
	SI1133_CHANNEL_IR,

	_SI1133_CHANNEL_LIST_END
};

#define SI1133_CHANNEL_COUNT  _SI1133_CHANNEL_LIST_END

struct si1133_coeff {

	s16_t info;
	u16_t mag;
};

struct si1133_config {

	const char *i2c_dev_name;
	u8_t i2c_slave_addr;

	int channel_count;
	struct {
		u8_t decim_rate;
		u8_t adc_select;
		u8_t sw_gain;
		u8_t hw_gain;
		u8_t post_shift;
		bool hsig;
		u8_t input_fraction;
		const struct si1133_coeff *coeff;
		int coeff_count;
	} channels[SI1133_CHANNEL_COUNT];
};

struct si1133_data {

	struct device *i2c_master;

	u8_t chn_mask;
	u8_t resp;
	struct {
		s32_t hostout;
	} channels[SI1133_CHANNEL_COUNT];
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_ */
