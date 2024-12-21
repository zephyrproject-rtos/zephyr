/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_P3T1755_P3T1755_H_
#define ZEPHYR_DRIVERS_SENSOR_P3T1755_P3T1755_H_

#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
#include <zephyr/drivers/i3c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c) */

#include <zephyr/kernel.h>

#define P3T1755_BUS_I2C (1<<0)
#define P3T1755_BUS_I3C (1<<1)
/* Registers. */
#define P3T1755_TEMPERATURE_REG (0x00U)
#define P3T1755_CONFIG_REG      (0x01U)

#define P3T1755_TEMPERATURE_REG_SHIFT (0x04U)
#define P3T1755_TEMPERATURE_SCALE     62500
#define P3T1755_TEMPERATURE_SIGN_BIT  BIT(12)
#define P3T1755_TEMPERATURE_ABS_MASK  ((uint16_t)(P3T1755_TEMPERATURE_SIGN_BIT - 1U))

#define P3T1755_CONFIG_REG_OS BIT(7)
#define P3T1755_CONFIG_REG_SD BIT(0)

struct p3t1755_io_ops {
	int (*read)(const struct device *dev, uint8_t reg, uint8_t *byte, uint8_t len);
	int (*write)(const struct device *dev, uint8_t reg, uint8_t *byte, uint8_t len);
};

union p3t1755_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct i3c_device_desc **i3c;
#endif
};

struct p3t1755_config {
	const union p3t1755_bus_cfg bus_cfg;
	const struct p3t1755_io_ops ops;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct {
		const struct device *bus;
		const struct i3c_device_id dev_id;
	} i3c;
#endif
	bool oneshot_mode;
	uint8_t inst_on_bus;
};

struct p3t1755_data {
	int16_t sample;
	uint8_t config_reg;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	struct i3c_device_desc *i3c_dev;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_P3T1755_P3T1755_H_ */
