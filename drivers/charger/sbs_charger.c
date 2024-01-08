/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sbs_sbs_charger

#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sbs_charger.h"

struct sbs_charger_config {
	struct i2c_dt_spec i2c;
};

LOG_MODULE_REGISTER(sbs_charger);

static int sbs_cmd_reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *val)
{
	const struct sbs_charger_config *cfg;
	uint8_t i2c_data[2];
	int status;

	cfg = dev->config;
	status = i2c_burst_read_dt(&cfg->i2c, reg_addr, i2c_data, sizeof(i2c_data));
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}

	*val = sys_get_le16(i2c_data);

	return 0;
}

static int sbs_cmd_reg_write(const struct device *dev, uint8_t reg_addr, uint16_t val)
{
	const struct sbs_charger_config *config = dev->config;
	uint8_t buf[2];

	sys_put_le16(val, buf);

	return i2c_burst_write_dt(&config->i2c, reg_addr, buf, sizeof(buf));
}

static int sbs_cmd_reg_update(const struct device *dev, uint8_t reg_addr, uint16_t mask,
			      uint16_t val)
{
	uint16_t old_val, new_val;
	int ret;

	ret = sbs_cmd_reg_read(dev, SBS_CHARGER_REG_STATUS, &old_val);
	if (ret < 0) {
		return ret;
	}

	new_val = (old_val & ~mask) | (val & mask);
	if (new_val == old_val) {
		return 0;
	}

	return sbs_cmd_reg_write(dev, reg_addr, new_val);
}

static int sbs_charger_get_prop(const struct device *dev, const charger_prop_t prop,
				union charger_propval *val)
{
	uint16_t reg_val;
	int ret;

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		ret = sbs_cmd_reg_read(dev, SBS_CHARGER_REG_STATUS, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (reg_val & SBS_CHARGER_STATUS_AC_PRESENT) {
			val->online = CHARGER_ONLINE_FIXED;
		} else {
			val->online = CHARGER_ONLINE_OFFLINE;
		}

		return 0;
	case CHARGER_PROP_PRESENT:
		ret = sbs_cmd_reg_read(dev, SBS_CHARGER_REG_STATUS, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (reg_val & SBS_CHARGER_STATUS_BATTERY_PRESENT) {
			val->present = true;
		} else {
			val->present = false;
		}

		return 0;
	case CHARGER_PROP_STATUS:
		ret = sbs_cmd_reg_read(dev, SBS_CHARGER_REG_STATUS, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (!(reg_val & SBS_CHARGER_STATUS_BATTERY_PRESENT)) {
			val->status = CHARGER_STATUS_NOT_CHARGING;
		} else if (reg_val & SBS_CHARGER_STATUS_AC_PRESENT &&
			   !(reg_val & SBS_CHARGER_STATUS_CHARGE_INHIBITED)) {
			val->status = CHARGER_STATUS_CHARGING;
		} else {
			val->status = CHARGER_STATUS_DISCHARGING;
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int sbs_charger_set_prop(const struct device *dev, const charger_prop_t prop,
				const union charger_propval *val)
{
	uint16_t reg_val = 0;

	switch (prop) {
	case CHARGER_PROP_STATUS:
		if (val->status != CHARGER_STATUS_CHARGING) {
			reg_val = SBS_CHARGER_MODE_INHIBIT_CHARGE;
		}
		return sbs_cmd_reg_update(dev, SBS_CHARGER_REG_CHARGER_MODE,
					  SBS_CHARGER_MODE_INHIBIT_CHARGE, reg_val);
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief initialize the charger
 *
 * @return 0 for success
 */
static int sbs_charger_init(const struct device *dev)
{
	const struct sbs_charger_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct charger_driver_api sbs_charger_driver_api = {
	.get_property = &sbs_charger_get_prop,
	.set_property = &sbs_charger_set_prop,
};

#define SBS_CHARGER_INIT(inst)                                                                     \
                                                                                                   \
	static const struct sbs_charger_config sbs_charger_config_##inst = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &sbs_charger_init, NULL, NULL, &sbs_charger_config_##inst,     \
			      POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY, &sbs_charger_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SBS_CHARGER_INIT)
