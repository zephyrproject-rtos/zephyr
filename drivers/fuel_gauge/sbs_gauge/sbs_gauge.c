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

static int sbs_gauge_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val *val)
{
	int rc = 0;
	uint16_t tmp_val = 0;

	switch (prop) {
	case FUEL_GAUGE_AVG_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AVG_CURRENT, &tmp_val);
		val->avg_current = tmp_val * 1000;
		break;
	case FUEL_GAUGE_CYCLE_COUNT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CYCLE_COUNT, &tmp_val);
		val->cycle_count = tmp_val;
		break;
	case FUEL_GAUGE_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CURRENT, &tmp_val);
		val->current = tmp_val * 1000;
		break;
	case FUEL_GAUGE_FULL_CHARGE_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_FULL_CAPACITY, &tmp_val);
		val->full_charge_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_REMAINING_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_CAPACITY, &tmp_val);
		val->remaining_capacity = tmp_val * 1000;
		break;
	case FUEL_GAUGE_RUNTIME_TO_EMPTY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_RUNTIME2EMPTY, &tmp_val);
		val->runtime_to_empty = tmp_val;
		break;
	case FUEL_GAUGE_RUNTIME_TO_FULL:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AVG_TIME2FULL, &tmp_val);
		val->runtime_to_full = tmp_val;
		break;
	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_MANUFACTURER_ACCESS, &tmp_val);
		val->sbs_mfr_access_word = tmp_val;
		break;
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ASOC, &tmp_val);
		val->absolute_state_of_charge = tmp_val;
		break;
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_RSOC, &tmp_val);
		val->relative_state_of_charge = tmp_val;
		break;
	case FUEL_GAUGE_TEMPERATURE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_TEMP, &tmp_val);
		val->temperature = tmp_val;
		break;
	case FUEL_GAUGE_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_VOLTAGE, &tmp_val);
		val->voltage = tmp_val * 1000;
		break;
	case FUEL_GAUGE_SBS_MODE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_BATTERY_MODE, &tmp_val);
		val->sbs_mode = tmp_val;
		break;
	case FUEL_GAUGE_CHARGE_CURRENT:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CHG_CURRENT, &tmp_val);
		val->chg_current = tmp_val * 1000;
		break;
	case FUEL_GAUGE_CHARGE_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_CHG_VOLTAGE, &tmp_val);
		val->chg_voltage = tmp_val * 1000;
		break;
	case FUEL_GAUGE_STATUS:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_FLAGS, &tmp_val);
		val->fg_status = tmp_val;
		break;
	case FUEL_GAUGE_DESIGN_CAPACITY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_NOM_CAPACITY, &tmp_val);
		val->design_cap = tmp_val;
		break;
	case FUEL_GAUGE_DESIGN_VOLTAGE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_DESIGN_VOLTAGE, &tmp_val);
		val->design_volt = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AR, &tmp_val);
		val->sbs_at_rate = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ARTTF, &tmp_val);
		val->sbs_at_rate_time_to_full = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_ARTTE, &tmp_val);
		val->sbs_at_rate_time_to_empty = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE_OK:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_AROK, &tmp_val);
		val->sbs_at_rate_ok = tmp_val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_CAPACITY_ALARM, &tmp_val);
		val->sbs_remaining_capacity_alarm = tmp_val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		rc = sbs_cmd_reg_read(dev, SBS_GAUGE_CMD_REM_TIME_ALARM, &tmp_val);
		val->sbs_remaining_time_alarm = tmp_val;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int sbs_gauge_do_battery_cutoff(const struct device *dev)
{
	int rc = -ENOTSUP;
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

static int sbs_gauge_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			      union fuel_gauge_prop_val val)
{
	int rc = 0;
	uint16_t tmp_val = 0;

	switch (prop) {

	case FUEL_GAUGE_SBS_MFR_ACCESS:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_MANUFACTURER_ACCESS,
				       val.sbs_mfr_access_word);
		val.sbs_mfr_access_word = tmp_val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_REM_CAPACITY_ALARM,
				       val.sbs_remaining_capacity_alarm);
		val.sbs_remaining_capacity_alarm = tmp_val;
		break;
	case FUEL_GAUGE_SBS_REMAINING_TIME_ALARM:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_REM_TIME_ALARM,
				       val.sbs_remaining_time_alarm);
		val.sbs_remaining_time_alarm = tmp_val;
		break;
	case FUEL_GAUGE_SBS_MODE:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_BATTERY_MODE, val.sbs_mode);
		val.sbs_mode = tmp_val;
		break;
	case FUEL_GAUGE_SBS_ATRATE:
		rc = sbs_cmd_reg_write(dev, SBS_GAUGE_CMD_AR, val.sbs_at_rate);
		val.sbs_at_rate = tmp_val;
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;
}

static int sbs_gauge_get_buffer_prop(const struct device *dev,
				    fuel_gauge_prop_t prop_type, void *dst,
				    size_t dst_len)
{
	int rc = 0;

	switch (prop_type) {
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

	return rc;
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
	.get_property = &sbs_gauge_get_prop,
	.set_property = &sbs_gauge_set_prop,
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
	COND_CODE_1(DT_INST_PROP(index, battery_cutoff_support),                                   \
		    (_SBS_GAUGE_CONFIG_DEFINE(index)), (;))

/* Conditionally get the battery config variable name or NULL based on battery cutoff support */
#define SBS_GAUGE_GET_BATTERY_CONFIG_NAME(index)                                                   \
	COND_CODE_1(DT_INST_PROP(index, battery_cutoff_support),                                   \
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
