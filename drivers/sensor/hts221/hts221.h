/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_
#define ZEPHYR_DRIVERS_SENSOR_HTS221_HTS221_H_

#include <device.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>

#define HTS221_AUTOINCREMENT_ADDR	BIT(7)

#define HTS221_REG_WHO_AM_I		0x0F
#define HTS221_CHIP_ID			0xBC

#define HTS221_REG_CTRL1		0x20
#define HTS221_PD_BIT			BIT(7)
#define HTS221_BDU_BIT			BIT(2)
#define HTS221_ODR_SHIFT		0

#define HTS221_REG_CTRL3		0x22
#define HTS221_DRDY_EN			BIT(2)

#define HTS221_REG_DATA_START		0x28
#define HTS221_REG_CONVERSION_START	0x30

struct hts221_data {
	struct device *i2c;
	int16_t rh_sample;
	int16_t t_sample;

	uint8_t h0_rh_x2;
	uint8_t h1_rh_x2;
	uint16_t t0_degc_x8;
	uint16_t t1_degc_x8;
	int16_t h0_t0_out;
	int16_t h1_t0_out;
	int16_t t0_out;
	int16_t t1_out;

#ifdef CONFIG_HTS221_TRIGGER
	struct device *dev;
	struct device *drdy_dev;
	struct gpio_callback drdy_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_HTS221_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem drdy_sem;
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_HTS221_TRIGGER */
};

struct hts221_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
#ifdef CONFIG_HTS221_TRIGGER
	gpio_pin_t drdy_pin;
	gpio_flags_t drdy_flags;
	const char *drdy_controller;
#endif /* CONFIG_HTS221_TRIGGER */
};

#ifdef CONFIG_HTS221_TRIGGER
int hts221_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int hts221_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_HTS221__ */
