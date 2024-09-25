/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP007_TMP007_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP007_TMP007_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define TMP007_REG_CONFIG		0x02
#define TMP007_ALERT_EN_BIT		BIT(8)

#define TMP007_REG_TOBJ			0x03
#define TMP007_DATA_INVALID_BIT		BIT(0)

#define TMP007_REG_STATUS		0x04
#define TMP007_DATA_READY_INT_BIT	BIT(14)
#define TMP007_TOBJ_TH_HIGH_INT_BIT	BIT(13)
#define TMP007_TOBJ_TH_LOW_INT_BIT	BIT(12)
#define TMP007_TOBJ_TH_INT_BITS		\
	(TMP007_TOBJ_TH_HIGH_INT_BIT | TMP007_TOBJ_TH_LOW_INT_BIT)

#define TMP007_REG_TOBJ_TH_HIGH		0x06
#define TMP007_REG_TOBJ_TH_LOW		0x07

/* scale in micro degrees Celsius */
#define TMP007_TEMP_SCALE		31250
#define TMP007_TEMP_TH_SCALE		500000

struct tmp007_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_TMP007_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

struct tmp007_data {
	int16_t sample;

#ifdef CONFIG_TMP007_TRIGGER
	struct gpio_callback gpio_cb;
	const struct device *dev;

	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;

#if defined(CONFIG_TMP007_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TMP007_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_TMP007_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_TMP007_TRIGGER */
};

#ifdef CONFIG_TMP007_TRIGGER
int tmp007_reg_read(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t *val);

int tmp007_reg_write(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t val);

int tmp007_reg_update(const struct i2c_dt_spec *i2c, uint8_t reg,
		      uint16_t mask, uint16_t val);

int tmp007_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int tmp007_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int tmp007_init_interrupt(const struct device *dev);
#endif

#endif /* _SENSOR_TMP007_ */
