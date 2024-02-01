/*
 * Copyright 2023 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max20335_charger

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/linear_range.h>

#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(max20335_charger);

#define MAX20335_REG_STATUS_A 0x02
#define MAX20335_REG_ILIMCNTL 0x09
#define MAX20335_REG_CHG_CNTL_A 0x0A
#define MAX20335_CHGCNTLA_BAT_REG_CFG_MASK GENMASK(4, 1)
#define MAX20335_ILIMCNTL_MASK GENMASK(1, 0)
#define MAX20335_STATUS_A_CHG_STAT_MASK GENMASK(2, 0)
#define MAX20335_CHRG_EN_MASK BIT(0)
#define MAX20335_CHRG_EN BIT(0)
#define MAX20335_REG_CVC_VREG_MIN_UV 4050000U
#define MAX20335_REG_CVC_VREG_STEP_UV 50000U
#define MAX20335_REG_CVC_VREG_MIN_IDX 0x0U
#define MAX20335_REG_CVC_VREG_MAX_IDX 0x0CU

struct charger_max20335_config {
	struct i2c_dt_spec bus;
	uint32_t max_ichg_ua;
	uint32_t max_vreg_uv;
};

enum {
	MAX20335_CHARGER_OFF,
	MAX20335_CHARGING_SUSPENDED_DUE_TO_TEMPERATURE,
	MAX20335_PRE_CHARGE_IN_PROGRESS,
	MAX20335_FAST_CHARGE_IN_PROGRESS_1,
	MAX20335_FAST_CHARGE_IN_PROGRESS_2,
	MAX20335_MAINTAIN_CHARGE_IN_PROGRESS,
	MAX20335_MAIN_CHARGER_TIMER_DONE,
	MAX20335_CHARGER_FAULT_CONDITION,
};

static const struct linear_range charger_uv_range =
	LINEAR_RANGE_INIT(MAX20335_REG_CVC_VREG_MIN_UV,
			  MAX20335_REG_CVC_VREG_STEP_UV,
			  MAX20335_REG_CVC_VREG_MIN_IDX,
			  MAX20335_REG_CVC_VREG_MAX_IDX);

static int max20335_get_charger_status(const struct device *dev, enum charger_status *status)
{
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_STATUS_A, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(MAX20335_STATUS_A_CHG_STAT_MASK, val);

	switch (val) {
	case MAX20335_CHARGER_OFF:
		__fallthrough;
	case MAX20335_CHARGING_SUSPENDED_DUE_TO_TEMPERATURE:
		__fallthrough;
	case MAX20335_CHARGER_FAULT_CONDITION:
		*status = CHARGER_STATUS_NOT_CHARGING;
		break;
	case MAX20335_PRE_CHARGE_IN_PROGRESS:
		__fallthrough;
	case MAX20335_FAST_CHARGE_IN_PROGRESS_1:
		__fallthrough;
	case MAX20335_FAST_CHARGE_IN_PROGRESS_2:
		__fallthrough;
	case MAX20335_MAINTAIN_CHARGE_IN_PROGRESS:
		*status = CHARGER_STATUS_CHARGING;
		break;
	case MAX20335_MAIN_CHARGER_TIMER_DONE:
		*status = CHARGER_STATUS_FULL;
		break;
	default:
		*status = CHARGER_STATUS_UNKNOWN;
		break;
	};

	return 0;
}

static int max20335_set_constant_charge_voltage(const struct device *dev,
						uint32_t voltage_uv)
{
	const struct charger_max20335_config *const config = dev->config;
	uint16_t idx;
	uint8_t val;
	int ret;

	if (voltage_uv > config->max_vreg_uv) {
		LOG_WRN("Exceeded max constant charge voltage!");
		return -EINVAL;
	}

	ret = linear_range_get_index(&charger_uv_range, voltage_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	val = FIELD_PREP(MAX20335_CHGCNTLA_BAT_REG_CFG_MASK, idx);

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_CHG_CNTL_A,
				      MAX20335_CHGCNTLA_BAT_REG_CFG_MASK,
				      val);
}

static int max20335_set_constant_charge_current(const struct device *dev,
						uint32_t current_ua)
{
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;

	if (current_ua > config->max_ichg_ua) {
		LOG_WRN("Exceeded max constant charge current!");
		return -EINVAL;
	}

	switch (current_ua) {
	case 0:
		val = 0x00;
		break;
	case 100000:
		val = 0x01;
		break;
	case 500000:
		val = 0x02;
		break;
	case 1000000:
		val = 0x03;
		break;
	default:
		return -ENOTSUP;
	};

	val = FIELD_PREP(MAX20335_ILIMCNTL_MASK, val);

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_ILIMCNTL,
				      MAX20335_ILIMCNTL_MASK,
				      val);
}

static int max20335_get_constant_charge_current(const struct device *dev,
						uint32_t *current_ua)
{
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_ILIMCNTL, &val);
	if (ret) {
		return ret;
	}

	switch (val) {
	case 0x00:
		*current_ua = 0;
		break;
	case 0x01:
		*current_ua = 100000;
		break;
	case 0x02:
		*current_ua = 500000;
		break;
	case 0x03:
		*current_ua = 1000000;
		break;
	default:
		return -ENOTSUP;
	};

	return 0;
}

static int max20335_get_constant_charge_voltage(const struct device *dev,
						uint32_t *current_uv)
{
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_CHG_CNTL_A, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(MAX20335_CHGCNTLA_BAT_REG_CFG_MASK, val);

	return linear_range_get_value(&charger_uv_range, val, current_uv);
}

static int max20335_set_enabled(const struct device *dev, bool enable)
{
	const struct charger_max20335_config *const config = dev->config;

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_CHG_CNTL_A,
				      MAX20335_CHRG_EN_MASK,
				      enable ? MAX20335_CHRG_EN : 0);
}

static int max20335_get_prop(const struct device *dev, charger_prop_t prop,
			     union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_STATUS:
		return max20335_get_charger_status(dev, &val->status);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return max20335_get_constant_charge_current(dev,
							    &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return max20335_get_constant_charge_voltage(dev,
							    &val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int max20335_set_prop(const struct device *dev, charger_prop_t prop,
			     const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return max20335_set_constant_charge_current(dev,
							    val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return max20335_set_constant_charge_voltage(dev,
							    val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int max20335_init(const struct device *dev)
{
	const struct charger_max20335_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	return 0;
}

static const struct charger_driver_api max20335_driver_api = {
	.get_property = max20335_get_prop,
	.set_property = max20335_set_prop,
	.charge_enable = max20335_set_enabled,
};

#define MAX20335_DEFINE(inst)									\
	static const struct charger_max20335_config charger_max20335_config_##inst = {		\
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),					\
		.max_ichg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),	\
		.max_vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &max20335_init, NULL, NULL,					\
			      &charger_max20335_config_##inst,					\
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,				\
			      &max20335_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX20335_DEFINE)
