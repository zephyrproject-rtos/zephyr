/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq27427

#include "bq27427.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

struct bq27427_config {
	struct i2c_dt_spec i2c;
};

static int bq27427_command_read(const struct device *dev, uint8_t command, uint16_t *value)
{
	uint8_t i2c_data[2];
	const struct bq27427_config *cfg = (const struct bq27427_config *)dev->config;

	const int status = i2c_burst_read_dt(&cfg->i2c, command, i2c_data, sizeof(i2c_data));

	if (status < 0) {
		return status;
	}

	*value = sys_get_le16(i2c_data);
	return 0;
}

static int bq27427_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	int rc = 0;
	uint16_t tmp_val;

	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT:
		rc = bq27427_command_read(dev, BQ27427_CMD_AVERAGE_CURRENT, &tmp_val);
		val->avg_current = (int16_t)tmp_val * 1000;
		break;
	case FUEL_GAUGE_FLAGS:
		rc = bq27427_command_read(dev, BQ27427_CMD_FLAGS, &tmp_val);
		val->flags = tmp_val;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = bq27427_command_read(dev, BQ27427_CMD_FULL_CHARGE_CAPACITY, &tmp_val);
		val->full_charge_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = bq27427_command_read(dev, BQ27427_CMD_REMAINING_CAPACITY, &tmp_val);
		val->remaining_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = bq27427_command_read(dev, BQ27427_CMD_STATE_OF_CHARGE, &tmp_val);
		val->relative_state_of_charge = tmp_val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = bq27427_command_read(dev, BQ27427_CMD_TEMPERATURE, &tmp_val);
		val->temperature = tmp_val * 1000;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = bq27427_command_read(dev, BQ27427_CMD_VOLTAGE, &tmp_val);
		val->voltage = tmp_val * 1000;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int bq27427_init(const struct device *dev)
{
	const struct bq27427_config *cfg = (const struct bq27427_config *)dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, bq27427_driver_api) = {
	.get_property = &bq27427_get_prop,
};

#define BQ27427_INIT(index)                                                                        \
                                                                                                   \
	static const struct bq27427_config bq27427_config_##index = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &bq27427_init, NULL, NULL, &bq27427_config_##index,           \
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY, &bq27427_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ27427_INIT)
