/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   adi_ltc4162

#include <init.h>
#include <sys/byteorder.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <stdio.h>

#include "ltc4162.h"

LOG_MODULE_REGISTER(LTC4162, CONFIG_SENSOR_LOG_LEVEL);

static int ltc4162_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	val->val2 = 0;
	struct ltc4162_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	switch (chan) {
	case SENSOR_CHAN_CHARGER_STATUS:
		val->val1 = data->chrg_status;
		break;

	case SENSOR_CHAN_CHARGER_TYPE:
		val->val1 = data->chrg_type;
		break;

	case SENSOR_CHAN_CHARGER_HEALTH:
		val->val1 = data->chrg_health;
		break;

	case SENSOR_CHAN_CHARGER_INPUT_VOLTAGE:
		val->val1 = data->in_voltage / 1000;
		val->val2 = (data->in_voltage % 1000) * 1000;
		break;

	case SENSOR_CHAN_CHARGER_INPUT_CURRENT:
		val->val1 = data->in_current / 1000;
		val->val2 = (data->in_current % 1000) * 1000;
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT:
		val->val1 = data->const_current / 10000;
		val->val2 = (data->const_current % 10000) * 10000;
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX:
		val->val1 = data->const_current_max / 10000;
		val->val2 = (data->const_current_max % 10000) * 10000;
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE:
		val->val1 = data->const_voltage / 1000;
		val->val2 = (data->const_voltage % 1000) * 1000;
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->val1 = data->const_voltage_max / 1000;
		val->val2 = (data->const_voltage_max % 1000) * 1000;
		break;

	case SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT:
		val->val1 = data->in_current_limit / 1000;
		val->val2 = (data->in_current_limit % 1000) * 1000;
		break;

	case SENSOR_CHAN_CHARGER_TEMPERATURE:
		val->val1 = data->charger_temp / 1000;
		val->val2 = (data->charger_temp % 1000) * 1000;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum charger_status ltc4162_state_decode(enum ltc4162_state value)
{
	switch (value) {
	case absorb_charge:
	case cc_cv_charge:
		return CHARGER_STATUS_CHARGING;
	case charger_suspended:
		return CHARGER_STATUS_NOT_CHARGING;
	default:
		return CHARGER_STATUS_UNKNOWN;

	}
}

static enum charge_type ltc4162_charge_status_decode(enum ltc4162_charge_status value)
{
	if (!value) {
		return CHARGER_CHARGE_TYPE_NONE;
	}

	/* constant voltage/current and input_current limit are "fast" modes */
	if (value <= iin_limit_active) {
		return CHARGER_CHARGE_TYPE_FAST;
	}

	/* Anything that's not fast will return as trickle */
	return CHARGER_CHARGE_TYPE_TRICKLE;
}

static int ltc4162_state_to_health(enum ltc4162_state value)
{
	switch (value) {
	case bat_missing_fault:
		return CHARGER_HEALTH_UNSPEC_FAILURE;
	case bat_short_fault:
		return CHARGER_HEALTH_DEAD;
	default:
		return CHARGER_HEALTH_GOOD;
	}
}

static int ltc4162_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	int ret;
	uint16_t regval;
	float temp_regval;
	uint8_t buf[2], read_addr;
	struct ltc4162_data *data = dev->data;
	const struct ltc4162_config *cfg = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	switch (chan) {
	case SENSOR_CHAN_CHARGER_STATUS:
		read_addr = LTC4162_CHARGER_STATE;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read charger state");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		data->chrg_status = ltc4162_state_decode(regval);
		break;

	case SENSOR_CHAN_CHARGER_TYPE:
		read_addr = LTC4162_CHARGE_STATUS;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to get charge state");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		data->chrg_type = ltc4162_charge_status_decode(regval);
		break;

	case SENSOR_CHAN_CHARGER_HEALTH:
		read_addr = LTC4162_CHARGER_STATE;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to get charge health");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		data->chrg_health = ltc4162_state_to_health(regval);
		break;

	case SENSOR_CHAN_CHARGER_INPUT_VOLTAGE:
		read_addr = LTC4162_VIN;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to get input voltage");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		temp_regval = regval * 1.694;
		data->in_voltage = (uint16_t)temp_regval;
		break;

	case SENSOR_CHAN_CHARGER_INPUT_CURRENT:
		read_addr = LTC4162_IIN;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read input current");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		temp_regval = (regval * 1.466) / (cfg->rsnsi);
		data->in_current = (uint16_t)temp_regval;
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT:
		read_addr = LTC4162_ICHARGE_DAC;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant current");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval &= BIT(6) - 1;
		++regval;
		temp_regval = ((float)regval / cfg->rsnsb);
		data->const_current = (temp_regval * 10000);
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX:
		read_addr = LTC4162_CHARGE_CURRENT_SETTING;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant current max");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval &= BIT(6) - 1;
		++regval;
		temp_regval = ((float)regval / cfg->rsnsb);
		data->const_current_max = (temp_regval * 10000);
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE:
		read_addr = LTC4162_VCHARGE_DAC;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant charge voltage");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval &= BIT(6) - 1;
		temp_regval = ((float)regval * 0.02857);
		temp_regval = cfg->cell_count * (temp_regval + 6);
		data->const_voltage = (temp_regval * 1000);
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE_MAX:
		read_addr = LTC4162_VCHARGE_SETTING;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant charge current");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval &= BIT(6) - 1;
		temp_regval = ((float)regval * 0.02857);
		temp_regval = cfg->cell_count * (temp_regval + 6);
		data->const_voltage_max = (temp_regval * 1000);
		break;

	case SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT:
		read_addr = LTC4162_IIN_LIMIT_DAC;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant current");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval &= BIT(6) - 1;
		++regval;
		temp_regval = ((float)regval * 0.5);
		temp_regval = (temp_regval / cfg->rsnsi);
		data->in_current_limit = (temp_regval * 1000);
		break;

	case SENSOR_CHAN_CHARGER_TEMPERATURE:
		read_addr = LTC4162_DIE_TEMPERATURE;
		ret = i2c_burst_read(cfg->i2c_dev, cfg->i2c_addr,
			read_addr, &buf[0], sizeof(buf));
		if (ret < 0) {
			LOG_ERR("Failed to read constant current");
			return -EIO;
		}

		regval = (buf[1] << 8 | buf[0]);
		regval *= 215;
		regval /= 100;
		regval -= 26440;
		data->charger_temp = regval;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ltc4162_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	int ret;
	uint32_t temp_val;
	uint8_t regval, write_addr, buf[3];
	const struct ltc4162_config *cfg = dev->config;

	switch (chan) {
	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX:
		temp_val = 10000;
		regval = (temp_val * val->val1) * (0.001 / cfg->rsnsb);

		if (regval > 31) {
			return -EINVAL;
		}

		write_addr = LTC4162_CHARGE_CURRENT_SETTING;
		buf[0] = write_addr;
		buf[1] = regval;
		buf[2] = 0;

		ret = i2c_write(cfg->i2c_dev, &buf[0], sizeof(buf), cfg->i2c_addr);
		if (ret < 0) {
			LOG_ERR("failed to update max charge current");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE_MAX:
		if (val->val1 > 63) {
			return -EINVAL;
		}

		write_addr = LTC4162_VCHARGE_SETTING;
		buf[0] = write_addr;
		buf[1] = val->val1;
		buf[2] = 0;

		ret = i2c_write(cfg->i2c_dev, &buf[0], sizeof(buf), cfg->i2c_addr);
		if (ret < 0) {
			LOG_ERR("Failed to update max charge voltage");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT:
		if (val->val1 > 63) {
			return -EINVAL;
		}

		write_addr = LTC4162_IIN_LIMIT_TARGET;
		buf[0] = write_addr;
		buf[1] = val->val1;
		buf[2] = 0;

		ret = i2c_write(cfg->i2c_dev, &buf[0], sizeof(buf), cfg->i2c_addr);
		if (ret < 0) {
			LOG_ERR("Failed to update input current limit");
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief initialize the fuel gauge
 *
 * @return 0 for success
 */
static int ltc4162_driver_init(const struct device *dev)
{
	struct ltc4162_config *cfg;

	cfg = (struct ltc4162_config *)dev->config;

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("%s device is not ready", cfg->i2c_dev->name);
		return -ENODEV;
	}

	return 0;
}

static const struct sensor_driver_api ltc4162_driver_api = {
	.sample_fetch = ltc4162_sample_fetch,
	.channel_get = ltc4162_channel_get,
	.attr_set = ltc4162_attr_set
};

#define LTC4162_INIT(index)						\
									\
	static struct ltc4162_data ltc4162_driver_##index;		\
									\
	static const struct ltc4162_config ltc4162_cfg_##index = {	\
		.rsnsb = DT_INST_PROP(index, rsnsb_micro_ohms),		\
		.rsnsi = DT_INST_PROP(index, rsnsi_micro_ohms),		\
		.cell_count = DT_INST_PROP(index, cell_count),		\
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(index)),		\
		.i2c_addr = DT_INST_REG_ADDR(index),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(index, ltc4162_driver_init,		\
			      NULL,					\
			      &ltc4162_driver_##index,			\
			      &ltc4162_cfg_##index, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &ltc4162_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC4162_INIT)
