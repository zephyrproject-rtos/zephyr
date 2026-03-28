/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_INA230_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_INA230_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include "ina2xx_common.h"

#define INA230_REG_CONFIG          0x00
#define INA230_REG_SHUNT_VOLT      0x01
#define INA230_REG_BUS_VOLT        0x02
#define INA230_REG_POWER           0x03
#define INA230_REG_CURRENT         0x04
#define INA230_REG_CALIB           0x05
#define INA230_REG_MASK            0x06
#define INA230_REG_ALERT           0x07
#define INA232_REG_MANUFACTURER_ID 0x3E
#define INA236_REG_MANUFACTURER_ID 0x3E
#define INA236_REG_DEVICE_ID       0x3F

struct ina230_data {
	const struct device *dev;
	int16_t current;
	uint16_t bus_voltage;
	uint16_t power;
#ifdef CONFIG_INA230_TRIGGER
	const struct device *gpio;
	struct gpio_callback gpio_cb;
	struct k_work work;
	sensor_trigger_handler_t handler_alert;
	const struct sensor_trigger *trig_alert;
#endif /* CONFIG_INA230_TRIGGER */
};

struct ina230_config {
	struct ina2xx_config common;
	uint8_t power_scale;
	uint32_t uv_lsb;
	bool trig_enabled;
	uint16_t mask;
	const struct gpio_dt_spec alert_gpio;
	uint16_t alert_limit;
};

int ina230_trigger_mode_init(const struct device *dev);
int ina230_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_INA230_H_ */
