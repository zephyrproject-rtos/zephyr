/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_
#define ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_H_

#include <zephyr/types.h>
#include <device.h>

struct si1133_data {

	struct device *i2c_master;
	u16_t i2c_slave_addr;

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
