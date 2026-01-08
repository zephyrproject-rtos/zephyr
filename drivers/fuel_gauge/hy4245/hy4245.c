/*
 * Copyright (c) 2025, Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hycon_hy4245

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(HY4245);

#define HY4245_CHIPID				0x4245

#define HY4245_CMD_CTRL				0x00
#define HY4245_CMD_TEMPERATURE			0x06
#define HY4245_CMD_VOLTAGE			0x08
#define HY4245_CMD_CURRENT			0x0c
#define HY4245_CMD_CAPACITY_REM			0x10
#define HY4245_CMD_CAPACITY_FULL		0x12
#define HY4245_CMD_AVG_CURRENT			0x14
#define HY4245_CMD_TIME_TO_EMPTY		0x16
#define HY4245_CMD_TIME_TO_FULL			0x18
#define HY4245_CMD_CHRG_VOLTAGE			0x30
#define HY4245_CMD_CHRG_CURRENT			0x32
#define HY4245_CMD_CAPACITY_FULL_AVAIL		0x78
#define HY4245_CMD_RELATIVE_STATE_OF_CHRG	0x2c

#define HY4245_SUBCMD_CTRL_CHIPID		0x55

struct hy4245_config {
	struct i2c_dt_spec i2c;
};

static int hy4245_read16(const struct device *dev, uint8_t cmd, uint16_t *val)
{
	uint8_t buffer[2];
	const struct hy4245_config *cfg = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, cmd, buffer, sizeof(buffer));
	if (ret != 0) {
		LOG_ERR("Unable to read register, error %d", ret);
		return ret;
	}

	*val = sys_get_le16(buffer);
	return 0;
}

static int hy4245_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			   union fuel_gauge_prop_val *val)
{
	int ret;
	uint16_t raw;

	switch (prop) {
	case FUEL_GAUGE_TEMPERATURE:
		ret = hy4245_read16(dev, HY4245_CMD_TEMPERATURE, &raw);
		val->temperature = raw;
		break;
	case FUEL_GAUGE_VOLTAGE:
		ret = hy4245_read16(dev, HY4245_CMD_VOLTAGE, &raw);
		val->voltage = raw * 1000;
		break;
	case FUEL_GAUGE_CURRENT:
		ret = hy4245_read16(dev, HY4245_CMD_CURRENT, &raw);
		val->current = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_REM, &raw);
		val->remaining_capacity = raw * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL, &raw);
		val->full_charge_capacity = raw * 1000;
		break;
	case FUEL_GAUGE_AVG_CURRENT:
		ret = hy4245_read16(dev, HY4245_CMD_AVG_CURRENT, &raw);
		val->avg_current = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_EMPTY, &raw);
		val->runtime_to_empty = raw;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_FULL, &raw);
		val->runtime_to_full = raw;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_VOLTAGE, &raw);
		val->chg_voltage = raw * 1000;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_CURRENT, &raw);
		val->chg_current = raw * 1000;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL_AVAIL, &raw);
		val->design_cap = raw;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		ret = hy4245_read16(dev, HY4245_CMD_RELATIVE_STATE_OF_CHRG, &raw);
		val->relative_state_of_charge = raw;
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int hy4245_init(const struct device *dev)
{
	int ret;
	const struct hy4245_config *cfg;
	uint8_t cmd[3] = {HY4245_CMD_CTRL,  HY4245_SUBCMD_CTRL_CHIPID, 0};
	uint16_t chip_id;

	cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = i2c_write_read_dt(&cfg->i2c, cmd, sizeof(cmd),
				&chip_id, sizeof(chip_id));
	if (ret != 0) {
		LOG_ERR("Unable to read register, error %d", ret);
		return ret;
	}

	chip_id = sys_get_le16((const uint8_t *)&chip_id);
	if (chip_id != HY4245_CHIPID) {
		LOG_ERR("unknown chip id %x", chip_id);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, hy4245_driver_api) = {
	.get_property = &hy4245_get_prop,
};

#define HY4245_INIT(index)								\
											\
	static const struct hy4245_config hy4245_config_##index = {			\
		.i2c = I2C_DT_SPEC_INST_GET(index),					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(index, &hy4245_init, NULL, NULL, &hy4245_config_##index,	\
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY, &hy4245_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HY4245_INIT)
