/*
 * Copyright (c) 2022 Leica Geosystems AG
 *
 * Copyright 2022 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sbs_sbs_gauge_new_api

#include "sbs_gauge.h"

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sbs_gauge);

static int sbs_cmd_reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *val)
{
	const struct sbs_gauge_config *cfg;
	uint8_t i2c_data[2];
	int status;

	cfg = dev->config;
	status = i2c_burst_read_dt(&cfg->i2c, reg_addr, i2c_data, ARRAY_SIZE(i2c_data));
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}

	*val = sys_get_le16(i2c_data);

	return 0;
}

static int sbs_cmd_reg_write(const struct device *dev, uint8_t reg_addr, uint16_t val)
{
	const struct sbs_gauge_config *config = dev->config;
	uint8_t buf[2];

	sys_put_le16(val, buf);

	return i2c_burst_write_dt(&config->i2c, reg_addr, buf, sizeof(buf));
}

static int sbs_cmd_buffer_read(const struct device *dev, uint8_t reg_addr, char *buffer,
			      const uint8_t buffer_size)
{
	const struct sbs_gauge_config *cfg;
	int status;

	cfg = dev->config;
	status = i2c_burst_read_dt(&cfg->i2c, reg_addr, buffer, buffer_size);
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return status;
	}

	return 0;
}

static int sbs_gauge_get_prop(const struct device *dev, struct fuel_gauge_get_property *prop)
{
	int rc = 0;
	uint16_t val = 0;

	switch (prop->property_type) {
	case FUEL_GAUGE_AVG_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AVG_CURRENT, &val);
		prop->value.avg_current = val * 1000;
		break;
	case FUEL_GAUGE_CYCLE_COUNT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CYCLE_COUNT, &val);
		prop->value.cycle_count = val;
		break;
	case FUEL_GAUGE_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CURRENT, &val);
		prop->value.current = val * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_FULL_CAPACITY, &val);
		prop->value.full_charge_capacity = val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_CAPACITY, &val);
		prop->value.remaining_capacity = val * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_RUNTIME2EMPTY, &val);
		prop->value.runtime_to_empty = val;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AVG_TIME2FULL, &val);
		prop->value.runtime_to_full = val;
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_MANUFACTURER_ACCESS, &val);
		prop->value.sbs_mfr_access_word = val;
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ASOC, &val);
		prop->value.absolute_state_of_charge = val;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_RSOC, &val);
		prop->value.relative_state_of_charge = val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_TEMP, &val);
		prop->value.temperature = val;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_VOLTAGE, &val);
		prop->value.voltage = val * 1000;
		break;
	case FUEL_GAUGE_SBS_MODE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_BATTERY_MODE, &val);
		prop->value.sbs_mode = val;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CHG_CURRENT, &val);
		prop->value.chg_current = val;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CHG_VOLTAGE, &val);
		prop->value.chg_voltage = val;
		break;
	case FUEL_GAUGE_STATUS:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_FLAGS, &val);
		prop->value.fg_status = val;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_NOM_CAPACITY, &val);
		prop->value.design_cap = val;
		break;
	case FUEL_GAUGE_DESIGN_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_DESIGN_VOLTAGE, &val);
		prop->value.design_volt = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AR, &val);
		prop->value.sbs_at_rate = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ARTTF, &val);
		prop->value.sbs_at_rate_time_to_full = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ARTTE, &val);
		prop->value.sbs_at_rate_time_to_empty = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_OK:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AROK, &val);
		prop->value.sbs_at_rate_ok = val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_CAPACITY_ALARM, &val);
		prop->value.sbs_remaining_capacity_alarm = val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_TIME_ALARM, &val);
		prop->value.sbs_remaining_time_alarm = val;
		break;
	default:
		rc = -ENOTSUP;
	}

	prop->status = rc;

	return rc;
}

static int sbs_gauge_do_battery_cutoff(const struct device *dev)
{
	int rc;
	const struct sbs_gauge_config *cfg = dev->config;

	if (cfg->cutoff_cfg == NULL) {
		return -ENOTSUP;
	}

	for (int i = 0; i < cfg->cutoff_cfg->payload_size; i++) {
		rc = sbs_cmd_reg_write(dev, cfg->cutoff_cfg->reg, cfg->cutoff_cfg->payload[i]);
		if (rc != 0) {
			return rc;
		}
	}

	return rc;
}

static int sbs_gauge_set_prop(const struct device *dev, struct fuel_gauge_set_property *prop)
{
	int rc = 0;
	uint16_t val = 0;

	switch (prop->property_type) {

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_MANUFACTURER_ACCESS,
				       prop->value.sbs_mfr_access_word);
		prop->value.sbs_mfr_access_word = val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_REM_CAPACITY_ALARM,
				       prop->value.sbs_remaining_capacity_alarm);
		prop->value.sbs_remaining_capacity_alarm = val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_REM_TIME_ALARM,
				       prop->value.sbs_remaining_time_alarm);
		prop->value.sbs_remaining_time_alarm = val;
		break;
	case FUEL_GAUGE_SBS_MODE:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_BATTERY_MODE, prop->value.sbs_mode);
		prop->value.sbs_mode = val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_AR, prop->value.sbs_at_rate);
		prop->value.sbs_at_rate = val;
		break;
	default:
		rc = -ENOTSUP;
	}

	prop->status = rc;

	return rc;
}

static int sbs_gauge_get_buffer_prop(const struct device *dev,
				    struct fuel_gauge_get_buffer_property *prop, void *dst,
				    size_t dst_len)
{
	int rc = 0;

	switch (prop->property_type) {
	case FUEL_GAUGE_MANUFACTURER_NAME:
		if (dst_len == sizeof(struct sbs_gauge_manufacturer_name)) {
			rc = sbs_cmd_buffer_read(dev, SBS_GAUGE_CMD_MANUFACTURER_NAME, (char *)dst,
						dst_len);
		} else {
			rc = -EINVAL;
		}
		break;
	case FUEL_GAUGE_DEVICE_NAME:
		if (dst_len == sizeof(struct sbs_gauge_device_name)) {
			rc = sbs_cmd_buffer_read(dev, SBS_GAUGE_CMD_DEVICE_NAME, (char *)dst,
						dst_len);
		} else {
			rc = -EINVAL;
		}
		break;
	case FUEL_GAUGE_DEVICE_CHEMISTRY:
		if (dst_len == sizeof(struct sbs_gauge_device_chemistry)) {
			rc = sbs_cmd_buffer_read(dev, SBS_GAUGE_CMD_DEVICE_CHEMISTRY, (char *)dst,
						dst_len);
		} else {
			rc = -EINVAL;
		}
		break;
	default:
		rc = -ENOTSUP;
	}

	prop->status = rc;
	return rc;
}

static int sbs_gauge_get_props(const struct device *dev, struct fuel_gauge_get_property *props,
			       size_t len)
{
	int err_count = 0;

	for (int i = 0; i < len; i++) {
		int ret = sbs_gauge_get_prop(dev, props + i);

		err_count += ret ? 1 : 0;
	}

	err_count = (err_count == len) ? -1 : err_count;

	return err_count;
}

static int sbs_gauge_set_props(const struct device *dev, struct fuel_gauge_set_property *props,
			       size_t len)
{
	int err_count = 0;

	for (int i = 0; i < len; i++) {
		int ret = sbs_gauge_set_prop(dev, props + i);

		err_count += ret ? 1 : 0;
	}

	err_count = (err_count == len) ? -1 : err_count;

	return err_count;
}

/**
 * @brief initialize the fuel gauge
 *
 * @return 0 for success
 */
static int sbs_gauge_init(const struct device *dev)
{
	const struct sbs_gauge_config *cfg;

	cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct fuel_gauge_driver_api sbs_gauge_driver_api = {
	.get_property = &sbs_gauge_get_props,
	.set_property = &sbs_gauge_set_props,
	.get_buffer_property = &sbs_gauge_get_buffer_prop,
	.battery_cutoff = &sbs_gauge_do_battery_cutoff,
};

/* Concatenates index to battery config to create unique cfg variable name per instance. */
#define _SBS_GAUGE_BATT_CUTOFF_CFG_VAR_NAME(index) sbs_gauge_batt_cutoff_cfg_##index

/* Declare and define the battery config struct */
#define _SBS_GAUGE_CONFIG_DEFINE(index)                                                            \
	static const struct sbs_gauge_battery_cutoff_config _SBS_GAUGE_BATT_CUTOFF_CFG_VAR_NAME(   \
		index) = {                                                                         \
		.reg = DT_INST_PROP(index, battery_cutoff_reg_addr),                               \
		.payload = DT_INST_PROP(index, battery_cutoff_payload),                            \
		.payload_size = DT_INST_PROP_LEN(index, battery_cutoff_payload),                   \
	};

/* Conditionally defined battery config based on battery cutoff support */
#define SBS_GAUGE_CONFIG_DEFINE(index)                                                             \
	COND_CODE_1(DT_INST_PROP_OR(index, battery_cutoff_support, false),                         \
		    (_SBS_GAUGE_CONFIG_DEFINE(index)), (;))

/* Conditionally get the battery config variable name or NULL based on battery cutoff support */
#define SBS_GAUGE_GET_BATTERY_CONFIG_NAME(index)                                                   \
	COND_CODE_1(DT_INST_PROP_OR(index, battery_cutoff_support, false),                         \
		    (&_SBS_GAUGE_BATT_CUTOFF_CFG_VAR_NAME(index)), (NULL))

#define SBS_GAUGE_INIT(index)                                                                      \
	SBS_GAUGE_CONFIG_DEFINE(index);                                                            \
	static const struct sbs_gauge_config sbs_gauge_config_##index = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.cutoff_cfg = SBS_GAUGE_GET_BATTERY_CONFIG_NAME(index)};                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &sbs_gauge_init, NULL, NULL, &sbs_gauge_config_##index,       \
			      POST_KERNEL, CONFIG_FUEL_GAUGE_INIT_PRIORITY,                        \
			      &sbs_gauge_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_INIT)

#define CUTOFF_PAYLOAD_SIZE_ASSERT(inst)                                                           \
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(inst, battery_cutoff_payload, 0) <=                       \
		     SBS_GAUGE_CUTOFF_PAYLOAD_MAX_SIZE);
DT_INST_FOREACH_STATUS_OKAY(CUTOFF_PAYLOAD_SIZE_ASSERT)
