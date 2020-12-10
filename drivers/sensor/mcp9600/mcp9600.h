/*
 *
 * Copyright (c) 2020 SER Consulting LLC, Steven Riedl
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MCP9600_MCP9600_H_
#define ZEPHYR_DRIVERS_SENSOR_MCP9600_MCP9600_H_

#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <drivers/gpio.h>

#define MCP9600_REG_TEMP_HOT            0x00
// 16 bits
#define MCP9600_REG_TEMP_DIFF           0x01
#define MCP9600_REG_TEMP_COLD           0x02
#define MCP9600_REG_RAW_ADC             0x03
// 24 bits
#define MCP9600_REG_STATUS              0x04
// 8 bits
#define MCP9600_REG_TC_CONFIG       0x05
#define MCP9600_REG_DEV_CONFIG      0x06
#define MCP9600_REG_A1_CONFIG       0x07
// 8 bits
#define MCP9600_REG_A2_CONFIG       0x08
#define MCP9600_REG_A3_CONFIG       0x09
#define MCP9600_REG_A4_CONFIG       0x0A
#define MCP9600_A1_HYST             0x0B
// 8 bits
#define MCP9600_A2_HYST             0x0C
#define MCP9600_A3_HYST             0x0D
#define MCP9600_A4_HYST             0x0E
#define MCP9600_A1_LIMIT            0x0F
// 16 bits
#define MCP9600_A2_LIMIT            0x10
#define MCP9600_A3_LIMIT            0x11
#define MCP9600_A4_LIMIT            0x12
#define MCP9600_REG_ID_REVISION     0x13
// 16 bits

#define MCP9600_CFG_ALERT_ENA       BIT(0)
#define MCP9600_CFG_ALERT_MODE_INT      BIT(1)
#define MCP9600_CFG_ALERT_HI_LO     BIT(2)
#define MCP9600_CFG_ALERT_RIS_FAL   BIT(3)
#define MCP9600_CFG_ALERT_TH_TC     BIT(4)
#define MCP9600_CFG_INT_CLEAR           BIT(7)

struct mcp9600_data {
	const struct device *i2c_master;

	int16_t ttemp;
	uint32_t tfrac;
	int16_t dtemp;
	uint32_t dfrac;
	int16_t ctemp;
	uint32_t cfrac;

#ifdef CONFIG_MCP9600_TRIGGER
	struct device *alert_gpio;
	struct gpio_callback alert_cb;

	struct device *dev;

	struct sensor_trigger trig;
	sensor_trigger_handler_t trigger_handler;
#endif

#ifdef CONFIG_MCP9600_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_MCP9600_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct mcp9600_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
#ifdef CONFIG_MCP9600_TRIGGER
	uint8_t alert_pin;
	uint8_t alert_flags;
	const char *alert_controller;
#endif /* CONFIG_MCP9600_TRIGGER */
};

static int mcp9600_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size);

#ifdef CONFIG_MCP9600_TRIGGER
int mcp9600_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);
int mcp9600_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int mcp9600_setup_interrupt(const struct device *dev);
#endif  /* CONFIG_MCP9600_TRIGGER */

#endif  /* ZEPHYR_DRIVERS_SENSOR_MCP9600_MCP9600_H_ */
