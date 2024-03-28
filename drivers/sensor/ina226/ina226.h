/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA226_INA226_H_
#define ZEPHYR_DRIVERS_SENSOR_INA226_INA226_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

/* Device register addresses. */
#define INA226_REG_CONFIG		0x00
#define INA226_REG_SHUNT_VOLT		0x01
#define INA226_REG_BUS_VOLT		0x02
#define INA226_REG_POWER		0x03
#define INA226_REG_CURRENT		0x04
#define INA226_REG_CALIB		0x05
#define INA226_REG_MASK			0x06
#define INA226_REG_ALERT		0x07
#define INA226_REG_MANUFACTURER_ID	0xFE
#define INA226_REG_DEVICE_ID		0xFF

/* Device register values. */
#define INA226_MANUFACTURER_ID		0x5449
#define INA226_DEVICE_ID		0x2260

struct ina226_data {
	const struct device *dev;
	int16_t current;
	uint16_t bus_voltage;
	uint16_t power;
#ifdef CONFIG_INA226_VSHUNT
	int16_t shunt_voltage;
#endif
	enum sensor_channel chan;
};

struct ina226_config {
	struct i2c_dt_spec bus;
	uint16_t config;
	uint32_t current_lsb;
	uint16_t cal;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA226_INA226_H_ */
