/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MC3419_H_
#define ZEPHYR_DRIVERS_SENSOR_MC3419_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Registers */
#define MC3419_REG_DEV_STATUS		0x05
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
#define MC3419_REG_GPIO_CTRL		0x33
#define MC3419_REG_ANY_MOTION_THRES	0x43
#define MC3419_REG_READ_CNT		0x4B
#define MC3419_MOTION_CTRL		0x04
#define MC3419_DRDY_CTRL		0x80

#define MC3419_RANGE_MASK		GENMASK(4, 6)
#define MC3419_DATA_READY_MASK		BIT(7)
#define MC3419_ANY_MOTION_MASK		BIT(2)
#define MC3419_INT_CLEAR		0x00
#define MC3419_INT_ROUTE		0x10

#define MC3419_ANY_MOTION_THRESH_MAX	0x7FFF
#define MC3419_SAMPLE_SIZE		3
#define MC3419_SAMPLE_READ_SIZE		(MC3419_SAMPLE_SIZE * (sizeof(int16_t)))

#define SENSOR_GRAIN_VALUE             (61LL / 1000.0)
#define SENSOR_GRAVITY_DOUBLE          (SENSOR_G / 1000000.0)

enum mc3419_op_mode {
	MC3419_MODE_STANDBY = 0x00,
	MC3419_MODE_WAKE = 0x01
};

enum mc3419_accl_range {
	MC3419_ACCl_RANGE_2G,
	MC3419_ACCl_RANGE_4G,
	MC3419_ACCl_RANGE_8G,
	MC3419_ACCl_RANGE_12G,
	MC3419_ACCl_RANGE_16G,
	MC3419_ACCL_RANGE_END
};

enum mc3419_odr_rate {
	MC3419_ODR_25	= 0x10,
	MC3419_ODR_50	= 0x11,
	MC3419_ODR_62_5	= 0x12,
	MC3419_ODR_100	= 0x13,
	MC3419_ODR_125	= 0x14,
	MC3419_ODR_250	= 0x15,
	MC3419_ODR_500	= 0x16,
	MC3419_ODR_1000	= 0x17
};

struct mc3419_config {
	struct i2c_dt_spec i2c;
#if defined CONFIG_MC3419_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	int op_mode;
};

struct mc3419_driver_data {
	double sensitivity;
	struct k_sem sem;
#if defined CONFIG_MC3419_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trigger_drdy;
#endif
#if defined CONFIG_MC3419_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
#if defined CONFIG_MC3419_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MC3419_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#if defined CONFIG_MC3419_MOTION
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trigger;
#endif
	int16_t samples[MC3419_SAMPLE_SIZE];
};

static inline int mc3419_set_op_mode(const struct mc3419_config *cfg, enum mc3419_op_mode mode)
{
	return i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_OP_MODE, mode);
}

#if CONFIG_MC3419_TRIGGER
int mc3419_trigger_init(const struct device *dev);
int mc3419_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
#endif

#endif
