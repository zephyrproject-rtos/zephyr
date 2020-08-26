/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_
#define ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_

#include <device.h>
#include <kernel.h>
#include <drivers/gpio.h>

#define SHT3XD_CMD_FETCH                0xE000
#define SHT3XD_CMD_ART                  0x2B32
#define SHT3XD_CMD_READ_STATUS          0xF32D
#define SHT3XD_CMD_CLEAR_STATUS         0x3041

#define SHT3XD_CMD_WRITE_TH_HIGH_SET    0x611D
#define SHT3XD_CMD_WRITE_TH_HIGH_CLEAR  0x6116
#define SHT3XD_CMD_WRITE_TH_LOW_SET     0x610B
#define SHT3XD_CMD_WRITE_TH_LOW_CLEAR   0x6100

#if CONFIG_SHT3XD_REPEATABILITY_LOW
	#define SHT3XD_REPEATABILITY_IDX        0
#elif CONFIG_SHT3XD_REPEATABILITY_MEDIUM
	#define SHT3XD_REPEATABILITY_IDX        1
#elif CONFIG_SHT3XD_REPEATABILITY_HIGH
	#define SHT3XD_REPEATABILITY_IDX        2
#endif

#if CONFIG_SHT3XD_MPS_05
	#define SHT3XD_MPS_IDX          0
#elif CONFIG_SHT3XD_MPS_1
	#define SHT3XD_MPS_IDX          1
#elif CONFIG_SHT3XD_MPS_2
	#define SHT3XD_MPS_IDX          2
#elif CONFIG_SHT3XD_MPS_4
	#define SHT3XD_MPS_IDX          3
#elif CONFIG_SHT3XD_MPS_10
	#define SHT3XD_MPS_IDX          4
#endif

#define SHT3XD_CLEAR_STATUS_WAIT_USEC   1000

struct sht3xd_config {
	char *bus_name;
#ifdef CONFIG_SHT3XD_TRIGGER
	char *alert_gpio_name;
#endif /* CONFIG_SHT3XD_TRIGGER */

	uint8_t base_address;
#ifdef CONFIG_SHT3XD_TRIGGER
	uint8_t alert_pin;
	uint8_t alert_flags;
#endif /* CONFIG_SHT3XD_TRIGGER */
};

struct sht3xd_data {
	struct device *dev;
	struct device *bus;

	uint16_t t_sample;
	uint16_t rh_sample;

#ifdef CONFIG_SHT3XD_TRIGGER
	struct device *alert_gpio;
	struct gpio_callback alert_cb;

	uint16_t t_low;
	uint16_t t_high;
	uint16_t rh_low;
	uint16_t rh_high;

	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_SHT3XD_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_SHT3XD_TRIGGER */
};

static inline uint8_t sht3xd_i2c_address(struct device *dev)
{
	const struct sht3xd_config *dcp = dev->config;

	return dcp->base_address;
}

static inline struct device *sht3xd_i2c_device(struct device *dev)
{
	const struct sht3xd_data *ddp = dev->data;

	return ddp->bus;
}

#ifdef CONFIG_SHT3XD_TRIGGER
int sht3xd_write_command(struct device *dev, uint16_t cmd);

int sht3xd_write_reg(struct device *dev, uint16_t cmd, uint16_t val);

int sht3xd_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int sht3xd_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int sht3xd_init_interrupt(struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_SHT3XD_SHT3XD_H_ */
