/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linumiz
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MC3419_H_
#define ZEPHYR_DRIVERS_SENSOR_MC3419_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Registers */
#define MC3419_REG_INT_CTRL		0x06
#define MC3419_REG_OP_MODE		0x07
#define MC3419_REG_SAMPLE_RATE		0x08
#define MC3419_REG_MOTION_CTRL		0x09
#define MC3419_REG_XOUT_L		0x0D
#define MC3419_REG_YOUT_L		0x0F
#define MC3419_REG_ZOUT_L		0x11
#define MC3419_REG_STATUS		0x13
#define MC3419_REG_INT_STATUS		0x14
#define MC3419_REG_RANGE_SELECT_CTRL	0x20
#define MC3419_REG_SAMPLE_RATE_2	0x30
#define MC3419_REG_COMM_CTRL		0x31
#define MC3419_REG_ANY_MOTION_THRES	0x43

#define MC3419_RANGE_MASK		GENMASK(6, 4)
#define MC3419_DATA_READY_MASK		BIT(7)
#define MC3419_ANY_MOTION_MASK		BIT(2)
#define MC3419_INT_CLEAR		0x00
#define MC3419_INT_ROUTE		0x10

#define MC3419_ANY_MOTION_THRESH_MAX	0x7FFF
#define MC3419_SAMPLE_SIZE		3
#define MC3419_SAMPLE_READ_SIZE		(MC3419_SAMPLE_SIZE * (sizeof(int16_t)))

#define SENSOR_GRAIN_VALUE             (61LL / 1000.0)
#define SENSOR_GRAVITY_DOUBLE          (SENSOR_G / 1000000.0)
#define MC3419_BASE_ODR_VAL		0x10

#define MC3419_TRIG_DATA_READY		0
#define MC3419_TRIG_ANY_MOTION		1
#define MC3419_TRIG_SIZE		2

enum mc3419_op_mode {
	MC3419_MODE_STANDBY = 0x00,
	MC3419_MODE_WAKE = 0x01
};

struct mc3419_odr_map {
	int16_t freq;
	int16_t mfreq;
};

enum mc3419_accl_range {
	MC3419_ACCl_RANGE_2G,
	MC3419_ACCl_RANGE_4G,
	MC3419_ACCl_RANGE_8G,
	MC3419_ACCl_RANGE_16G,
	MC3419_ACCl_RANGE_12G,
	MC3419_ACCL_RANGE_END
};

struct mc3419_config {
	struct i2c_dt_spec i2c;
#if defined(CONFIG_MC3419_TRIGGER)
	struct gpio_dt_spec int_gpio;
	bool int_cfg;
#endif
};

struct mc3419_driver_data {
	double sensitivity;
	struct k_sem sem;
	int16_t samples[MC3419_SAMPLE_SIZE];
#if defined(CONFIG_MC3419_TRIGGER)
	const struct device *gpio_dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler[MC3419_TRIG_SIZE];
	const struct sensor_trigger *trigger[MC3419_TRIG_SIZE];
#if defined(CONFIG_MC3419_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MC3419_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#else
	struct k_work work;
#endif
#endif
};

#if defined(CONFIG_MC3419_TRIGGER)
int mc3419_trigger_init(const struct device *dev);
int mc3419_configure_trigger(const struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler);
#endif

#endif
