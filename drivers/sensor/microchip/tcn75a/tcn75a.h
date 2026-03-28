/*
 * Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRIVERS_SENSOR_TCN75A_H_
#define _DRIVERS_SENSOR_TCN75A_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#define TCN75A_TEMP_REG	  0x0
#define TCN75A_CONFIG_REG 0x1
#define TCN75A_THYST_REG  0x2
#define TCN75A_TSET_REG	  0x3

/* TCN75A TEMP register constants */
#define TCN75A_TEMP_MSB_POS  8
#define TCN75A_TEMP_MSB_MASK 0xFF00
#define TCN75A_TEMP_LSB_MASK 0xFF
#define TCN75A_TEMP_LSB_POS  0

/* TCN75A CONFIG register constants */
#define TCN75A_CONFIG_ONEDOWN	BIT(7)
#define TCN75A_CONFIG_RES(x)    (((x) & 0x3) << 5)
#define TCN75A_CONFIG_INT_EN	0x2
#define TCN75A_CONFIG_SHUTDOWN	0x1

struct tcn75a_config {
	struct i2c_dt_spec i2c_spec;
	bool oneshot_mode;
	uint8_t resolution;
#ifdef CONFIG_TCN75A_TRIGGER
	struct gpio_dt_spec alert_gpios;
#endif
};

struct tcn75a_data {
	uint16_t temp_sample;
#ifdef CONFIG_TCN75A_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t sensor_cb;
	const struct sensor_trigger *sensor_trig;
#endif
#ifdef CONFIG_TCN75A_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
#ifdef CONFIG_TCN75A_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TCN75A_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
};

/* Helpers to convert from TCN75A temperature fixed point format
 * to sensor val2 format. When the LSB of the TCN75A temperature sample
 * is treated as an integer, the format to convert to sensor val2 is
 * FIXED_POINT_VAL * 3906.25
 */
#define TCN75A_FIXED_PT_TO_SENSOR(x) (((x)*3906) + ((x) >> 2))
/* This conversion is imprecise, but because the 4 least significant bits
 * of the temperature register aren't used, it doesn't matter.
 */
#define TCN75A_SENSOR_TO_FIXED_PT(x) ((x) / 3906)

#ifdef CONFIG_TCN75A_TRIGGER

int tcn75a_trigger_init(const struct device *dev);
int tcn75a_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    struct sensor_value *val);

int tcn75a_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val);
int tcn75a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
#endif
int tcn75a_sample_fetch(const struct device *dev, enum sensor_channel chan);

#endif /* _DRIVERS_SENSOR_TCN75A_H_ */
