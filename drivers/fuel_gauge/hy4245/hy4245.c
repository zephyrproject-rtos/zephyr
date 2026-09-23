/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hycon_hy4245

#include <zephyr/kernel.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/fuel_gauge/hy4245.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "hy4245.h"

LOG_MODULE_REGISTER(HY4245, CONFIG_FUEL_GAUGE_LOG_LEVEL);

/* The gauge delivers the DFChecksum() result within 500 ms */
#define HY4245_DF_CHECKSUM_DELAY_MS 500

struct hy4245_config {
	struct i2c_dt_spec i2c;
};

/* Low level register access */

static int hy4245_read16(const struct device *dev, uint8_t cmd, uint16_t *val)
{
	uint8_t buffer[2];
	const struct hy4245_config *cfg = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&cfg->i2c, cmd, buffer, sizeof(buffer));
	if (ret != 0) {
		LOG_ERR("Unable to read register 0x%02x, error %d", cmd, ret);
		return ret;
	}

	*val = sys_get_le16(buffer);
	return 0;
}

/* Write a Control() subcommand */
static int hy4245_ctrl_write(const struct device *dev, uint16_t subcmd)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[3] = {HY4245_CMD_CTRL};

	sys_put_le16(subcmd, &cmd[1]);

	return i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
}

/* Write a Control() subcommand and read its 16 bit result */
static int hy4245_ctrl_read(const struct device *dev, uint16_t subcmd, uint16_t *val)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t cmd[3] = {HY4245_CMD_CTRL};
	uint8_t buffer[2];
	int ret;

	sys_put_le16(subcmd, &cmd[1]);

	ret = i2c_write_read_dt(&cfg->i2c, cmd, sizeof(cmd), buffer, sizeof(buffer));
	if (ret != 0) {
		LOG_ERR("Control() 0x%04x failed, error %d", subcmd, ret);
		return ret;
	}

	*val = sys_get_le16(buffer);
	return 0;
}

/* Control() based status and version information */

static int hy4245_get_data_flash_checksum(const struct device *dev, uint16_t *checksum)
{
	const struct hy4245_config *cfg = dev->config;
	uint8_t buffer[2];
	int ret;

	ret = hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_DF_CHECKSUM);
	if (ret != 0) {
		return ret;
	}

	/* The gauge needs time to calculate the checksum before it can be read */
	k_sleep(K_MSEC(HY4245_DF_CHECKSUM_DELAY_MS));

	ret = i2c_burst_read_dt(&cfg->i2c, HY4245_CMD_CTRL, buffer, sizeof(buffer));
	if (ret != 0) {
		return ret;
	}

	*checksum = sys_get_le16(buffer);
	return 0;
}

/* OperationCfgA().UPD_EN: the gauge may update its data flash while operating */
static int hy4245_get_flash_update_enabled(const struct device *dev, bool *enabled)
{
	uint16_t cfg_a;
	int ret;

	ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_OPERATION_CFG_A, &cfg_a);
	if (ret != 0) {
		return ret;
	}

	*enabled = (cfg_a & HY4245_OPERATION_CONFIG_A_UPD_EN) != 0;
	return 0;
}

static int hy4245_enable_flash_update(const struct device *dev)
{
	bool enabled;
	int ret;

	ret = hy4245_get_flash_update_enabled(dev, &enabled);
	if (ret != 0 || enabled) {
		return ret;
	}

	return hy4245_ctrl_write(dev, HY4245_SUBCMD_CTRL_SET_UPD_EN);
}

/* Fuel gauge API */

static int hy4245_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			   union fuel_gauge_prop_val *val)
{
	uint16_t raw = 0;
	bool flag = false;
	int ret;

	switch (prop) {
	case FUEL_GAUGE_TEMPERATURE_DK:
		ret = hy4245_read16(dev, HY4245_CMD_TEMPERATURE, &raw);
		val->temperature_dk = raw;
		break;
	case FUEL_GAUGE_VOLTAGE_UV:
		ret = hy4245_read16(dev, HY4245_CMD_VOLTAGE, &raw);
		val->voltage_uv = raw * 1000;
		break;
	case FUEL_GAUGE_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_CURRENT, &raw);
		val->current_ua = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY_UAH:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_REM, &raw);
		val->remaining_capacity_uah = raw * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL, &raw);
		val->full_charge_capacity_uah = raw * 1000;
		break;
	case FUEL_GAUGE_AVG_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_AVG_CURRENT, &raw);
		val->avg_current_ua = (int16_t)raw * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_EMPTY, &raw);
		val->runtime_to_empty_mins = raw;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL_MINS:
		ret = hy4245_read16(dev, HY4245_CMD_TIME_TO_FULL, &raw);
		val->runtime_to_full_mins = raw;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE_UV:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_VOLTAGE, &raw);
		val->chg_voltage_uv = raw * 1000;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT_UA:
		ret = hy4245_read16(dev, HY4245_CMD_CHRG_CURRENT, &raw);
		val->chg_current_ua = raw * 1000;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		ret = hy4245_read16(dev, HY4245_CMD_CAPACITY_FULL_AVAIL, &raw);
		val->design_cap = raw;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT:
		ret = hy4245_read16(dev, HY4245_CMD_RELATIVE_STATE_OF_CHRG, &raw);
		val->relative_state_of_charge_pct = raw;
		break;
	case FUEL_GAUGE_STATE_OF_HEALTH:
		ret = hy4245_read16(dev, HY4245_CMD_STATE_OF_HEALTH, &raw);
		val->state_of_health = raw & 0xff;
		break;
	case HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE:
		ret = hy4245_get_flash_update_enabled(dev, &flag);
		val->custom_bool = flag;
		break;
	case HY4245_FUEL_GAUGE_CONTROL_STATUS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_STATUS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_OPERATION_CONFIG_A:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_OPERATION_CFG_A, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_FLAGS:
		ret = hy4245_read16(dev, HY4245_CMD_FLAGS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_SAFETY_STATUS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_SAFETY_STATUS, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_LIFETIME_OVER_TEMPERATURE_MINS:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_LIFETIME_OVER_TEMP, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM:
		ret = hy4245_get_data_flash_checksum(dev, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_DATA_FLASH_VERSION:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_DF_VERSION, &raw);
		val->custom_uint = raw;
		break;
	case HY4245_FUEL_GAUGE_FIRMWARE_VERSION:
		ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_FW_VERSION, &raw);
		val->custom_uint = raw;
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int hy4245_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			   union fuel_gauge_prop_val val)
{
	int ret;

	switch (prop) {
	case HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE:
		if (!val.custom_bool) {
			return -ENOTSUP;
		}

		ret = hy4245_enable_flash_update(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int hy4245_init(const struct device *dev)
{
	const struct hy4245_config *cfg = dev->config;
	uint16_t chip_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = hy4245_ctrl_read(dev, HY4245_SUBCMD_CTRL_CHIPID, &chip_id);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != HY4245_CHIPID) {
		LOG_ERR("unknown chip id %x", chip_id);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(fuel_gauge, hy4245_driver_api) = {
	.get_property = &hy4245_get_prop,
	.set_property = &hy4245_set_prop,
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
