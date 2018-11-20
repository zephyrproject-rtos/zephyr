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

	SI1133_CHANNEL0,
	SI1133_CHANNEL1,
	SI1133_CHANNEL2,
	SI1133_CHANNEL3,

	_SI1133_CHANNEL_LIST_END
};

#define SI1133_CHANNEL_COUNT  _SI1133_CHANNEL_LIST_END

struct si1133_config {

	const char *i2c_dev_name;
	u8_t i2c_slave_addr;

#ifdef CONFIG_SI1133_TRIGGER
	const char *gpio_trig_dev_name;
	u32_t gpio_trig_pin;
#endif

	int channel_count;
	struct {
		u8_t decim_rate;
		u8_t adc_select;
		u8_t sw_gain;
		u8_t hw_gain;
		u8_t post_shift;
		bool hsig;
	} channels[SI1133_CHANNEL_COUNT];
};

struct si1133_data {

	struct device *i2c_master;

	u8_t chn_mask;
	u8_t resp;
	struct {
		s32_t hostout;	
	} channels[SI1133_CHANNEL_COUNT];

#ifdef CONFIG_SI1133_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger trigger_drdy;
	sensor_trigger_handler_t handler_drdy;

#ifdef CONFIG_SI1133_TRIGGER_OWN_THREAD
	struct k_sem sem;
#elif CONFIG_SI1133_TRIGGER_GLOBAL_THREAD
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_SI1133_TRIGGER */
};

#ifdef CONFIG_SI1133_TRIGGER

int si1133_trigger_set(struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler);

int si1133_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_ */
