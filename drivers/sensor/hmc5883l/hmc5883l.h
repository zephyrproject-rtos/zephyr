/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HMC5883L_HMC5883L_H_
#define ZEPHYR_DRIVERS_SENSOR_HMC5883L_HMC5883L_H_

#include <device.h>
#include <misc/util.h>
#include <zephyr/types.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "HMC5883L"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define HMC5883L_I2C_ADDR		0x1E

#define HMC5883L_REG_CONFIG_A		0x00
#define HMC5883L_ODR_SHIFT		2

#define HMC5883L_REG_CONFIG_B		0x01
#define HMC5883L_GAIN_SHIFT		5

#define HMC5883L_REG_MODE		0x02
#define HMC5883L_MODE_CONTINUOUS	0

#define HMC5883L_REG_DATA_START		0x03

#define HMC5883L_REG_CHIP_ID		0x0A
#define HMC5883L_CHIP_ID_A		'H'
#define HMC5883L_CHIP_ID_B		'4'
#define HMC5883L_CHIP_ID_C		'3'

static const char * const hmc5883l_odr_strings[] = {
	"0.75", "1.5", "3", "7.5", "15", "30", "75"
};

static const char * const hmc5883l_fs_strings[] = {
	"0.88", "1.3", "1.9", "2.5", "4", "4.7", "5.6", "8.1"
};

static const u16_t hmc5883l_gain[] = {
	1370, 1090, 820, 660, 440, 390, 330, 230
};

struct hmc5883l_data {
	struct device *i2c;
	s16_t x_sample;
	s16_t y_sample;
	s16_t z_sample;
	u8_t gain_idx;

#ifdef CONFIG_HMC5883L_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_HMC5883L_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_HMC5883L_TRIGGER */
};

#ifdef CONFIG_HMC5883L_TRIGGER
int hmc5883l_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int hmc5883l_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_HMC5883L__ */
