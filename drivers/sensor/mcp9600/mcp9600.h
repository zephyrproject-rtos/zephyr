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

#define MCP9600_REG_TEMP_HOT		0x00

#define MCP9600_REG_TEMP_DIFF		0x01
#define MCP9600_REG_TEMP_COLD		0x02
#define MCP9600_REG_RAW_ADC			0x03

#define MCP9600_REG_STATUS			0x04

#define MCP9600_REG_TC_CONFIG		0x05
#define MCP9600_REG_DEV_CONFIG		0x06
#define MCP9600_REG_A1_CONFIG		0x07

#define MCP9600_REG_A2_CONFIG		0x08
#define MCP9600_REG_A3_CONFIG		0x09
#define MCP9600_REG_A4_CONFIG		0x0A
#define MCP9600_A1_HYST				0x0B

#define MCP9600_A2_HYST				0x0C
#define MCP9600_A3_HYST				0x0D
#define MCP9600_A4_HYST				0x0E
#define MCP9600_A1_LIMIT			0x0F

#define MCP9600_A2_LIMIT			0x10
#define MCP9600_A3_LIMIT			0x11
#define MCP9600_A4_LIMIT			0x12
#define MCP9600_REG_ID_REVISION		0x13

struct mcp9600_data {
	const struct device *i2c_master;
	int16_t ttemp;
};

struct mcp9600_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
};

static int mcp9600_reg_read(const struct device *dev, uint8_t start,
							uint8_t *buf, int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_MCP9600_MCP9600_H_ */
