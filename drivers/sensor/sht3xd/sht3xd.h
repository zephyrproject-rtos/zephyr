/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_
#define ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_

#include <device.h>
#include <kernel.h>
#include <gpio.h>

#define SHT3XD_I2C_ADDRESS		CONFIG_SHT3XD_I2C_ADDR

#define SHT3XD_CMD_FETCH		0xE000
#define SHT3XD_CMD_ART			0x2B32
#define SHT3XD_CMD_READ_STATUS		0xF32D
#define SHT3XD_CMD_CLEAR_STATUS		0x3041

#define SHT3XD_CMD_WRITE_TH_HIGH_SET	0x611D
#define SHT3XD_CMD_WRITE_TH_HIGH_CLEAR	0x6116
#define SHT3XD_CMD_WRITE_TH_LOW_SET	0x610B
#define SHT3XD_CMD_WRITE_TH_LOW_CLEAR	0x6100

#if CONFIG_SHT3XD_REPEATABILITY_LOW
	#define SHT3XD_REPEATABILITY_IDX	0
#elif CONFIG_SHT3XD_REPEATABILITY_MEDIUM
	#define SHT3XD_REPEATABILITY_IDX	1
#elif CONFIG_SHT3XD_REPEATABILITY_HIGH
	#define SHT3XD_REPEATABILITY_IDX	2
#endif

#if CONFIG_SHT3XD_MPS_05
	#define SHT3XD_MPS_IDX		0
#elif CONFIG_SHT3XD_MPS_1
	#define SHT3XD_MPS_IDX		1
#elif CONFIG_SHT3XD_MPS_2
	#define SHT3XD_MPS_IDX		2
#elif CONFIG_SHT3XD_MPS_4
	#define SHT3XD_MPS_IDX		3
#elif CONFIG_SHT3XD_MPS_10
	#define SHT3XD_MPS_IDX		4
#endif

#define SHT3XD_CLEAR_STATUS_WAIT_USEC	1000

static const u16_t sht3xd_measure_cmd[5][3] = {
	{0x202F, 0x2024, 0x2032},
	{0x212D, 0x2126, 0x2130},
	{0x222B, 0x2220, 0x2236},
	{0x2329, 0x2322, 0x2334},
	{0x272A, 0x2721, 0x2737}
};

static const int sht3xd_measure_wait[3] = {
	4000, 6000, 15000
};

struct sht3xd_data {
	struct device *i2c;
	u16_t t_sample;
	u16_t rh_sample;

#ifdef CONFIG_SHT3XD_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	u16_t t_low;
	u16_t t_high;
	u16_t rh_low;
	u16_t rh_high;

	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_SHT3XD_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_SHT3XD_TRIGGER */
};

#ifdef CONFIG_SHT3XD_TRIGGER
int sht3xd_write_command(struct sht3xd_data *drv_data, u16_t cmd);

int sht3xd_write_reg(struct sht3xd_data *drv_data, u16_t cmd,
		     u16_t val);

int sht3xd_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int sht3xd_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int sht3xd_init_interrupt(struct device *dev);
#endif

#define SYS_LOG_DOMAIN "SHT3XD"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_ */
