/*
 * Copyright 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp2101_charger

#include <errno.h>

#include "zephyr/device.h"
#include "zephyr/drivers/charger.h"
#include "zephyr/drivers/i2c.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/util.h"
#include "zephyr/logging/log.h"

#include "charger_axp2101.h"

LOG_MODULE_REGISTER(charger_axp2101, CONFIG_CHARGER_LOG_LEVEL);

/* Convert "milli" values to "micro" ones */
#define M2U(x)	(x * 1000)
#define U2M(x)	(x / 1000)

struct axp2101_config {
	struct i2c_dt_spec i2c;
};

struct axp2101_data {
	unsigned int cc_current_ma;
	unsigned int cc_voltage_mv;
	bool vbackup_enable;
};

static const uint32_t constant_charge_current_lut[] = {
	[0] = 0,
	[1] = 0,
	[2] = 0,
	[3] = 0,
	[4] = 100,
	[5] = 125,
	[6] = 150,
	[7] = 175,
	[8] = 200,
	[9] = 300,
	[10] = 400,
	[11] = 500,
	[12] = 600,
	[13] = 700,
	[14] = 800,
	[15] = 900,
	[16] = 1000,
};

static const uint32_t constant_charge_voltage_lut[] = {
	[0] = 0,
	[1] = 4000,
	[2] = 4100,
	[3] = 4200,
	[4] = 4350,
	[5] = 4400,
};

static int get_index_in_lut(uint32_t value, const uint32_t *lut, size_t lut_len)
{
	int i;

	for (i = 0; i < (int) lut_len; i++) {
		if (value == lut[i]) {
			return i;
		}
	}

	return -1;
}

static enum charger_online is_charger_online(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, PMU_STATUS1, &tmp);
	if (ret) {
		return ret;
	}
	val->online = (tmp == 1) ? CHARGER_ONLINE_FIXED : CHARGER_ONLINE_OFFLINE;

	return 0;
}

static int get_constant_charge_current_ua(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, ICC_CHARGER_SETTING, &tmp);
	if (ret) {
		return ret;
	}

	val->const_charge_current_ua = M2U(constant_charge_current_lut[tmp]);

	return 0;
}

static int set_constant_charge_current_ua(const struct device *dev,
					  const union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	struct axp2101_data *data = dev->data;
	uint32_t tmp = U2M(val->const_charge_current_ua);
	int lut_index;

	lut_index = get_index_in_lut(tmp, constant_charge_current_lut,
				     ARRAY_SIZE(constant_charge_current_lut));
	if (lut_index < 0) {
		return -EINVAL;
	}

	data->cc_current_ma = constant_charge_current_lut[lut_index];

	return i2c_reg_write_byte_dt(&config->i2c, ICC_CHARGER_SETTING, (uint8_t) lut_index);
}

static int get_pre_charge_current_ua(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp = 0;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, IPRECH_CHARGER_SETTING, &tmp);
	if (ret) {
		return ret;
	}
	val->precharge_current_ua = M2U(25 * ((uint32_t) val));

	return 0;
}

static int get_termination_current_ua(const struct device *dev,
				      union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp = 0;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, ITERM_CHARGER_SETTING, &tmp);
	if (ret) {
		return ret;
	}

	if (!IS_BIT_SET(tmp, 4)) {
		val->charge_term_current_ua = 0;
	} else {
		tmp = tmp & GENMASK(3, 0);
		val->charge_term_current_ua = M2U(25 * ((uint32_t) val));
	}

	return 0;
}

static int get_constant_charge_voltage_uv(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, CV_CHARGER_VOLTAGE, &tmp);
	if (ret) {
		return ret;
	}

	if (tmp > 5) {
		return -EINVAL;
	}

	val->const_charge_voltage_uv = M2U(constant_charge_voltage_lut[tmp]);

	return 0;
}

static int set_constant_charge_voltage_uv(const struct device *dev,
					  const union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint32_t tmp = U2M(val->const_charge_voltage_uv);
	int lut_index;

	lut_index = get_index_in_lut(tmp, constant_charge_voltage_lut,
				     ARRAY_SIZE(constant_charge_voltage_lut));
	if (lut_index < 0) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, CV_CHARGER_VOLTAGE, (uint8_t) lut_index);
}

static int get_status(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, PMU_STATUS2, &tmp);
	if (ret) {
		return ret;
	}

	tmp = tmp & CHARGING_STATUS;
	if ((tmp >= 0) && (tmp <= 3)) {
		val->status = CHARGER_STATUS_CHARGING;
	} else if (tmp == 4) {
		val->status = CHARGER_STATUS_FULL;
	} else if (tmp == 5) {
		val->status = CHARGER_STATUS_NOT_CHARGING;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int get_charge_type(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, PMU_STATUS2, &tmp);
	if (ret) {
		return ret;
	}

	tmp = tmp & CHARGING_STATUS;
	if ((tmp == 0) || (tmp == 1)) {
		val->charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
	} else if ((tmp >= 2) && (tmp <= 4)) {
		val->charge_type = CHARGER_CHARGE_TYPE_STANDARD;
	} else if (tmp == 5) {
		val->charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int axp2101_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	int ret;

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		ret = is_charger_online(dev, val);
		break;
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		ret = get_constant_charge_current_ua(dev, val);
		break;
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		ret = get_constant_charge_voltage_uv(dev, val);
		break;
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		ret = get_pre_charge_current_ua(dev, val);
		break;
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		ret = get_termination_current_ua(dev, val);
		break;
	case CHARGER_PROP_CHARGE_TYPE:
		ret = get_charge_type(dev, val);
		break;
	case CHARGER_PROP_STATUS:
		ret = get_status(dev, val);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int axp2101_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	int ret;

	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		ret = set_constant_charge_current_ua(dev, val);
		break;
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		ret = set_constant_charge_voltage_uv(dev, val);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int axp2101_charge_enable(const struct device *dev, const bool enable)
{
	const struct axp2101_config *const config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, CHARGER_CONTROL,
				      CELL_BATTERY_CHARGE_ENABLE,
				      (enable) ? CELL_BATTERY_CHARGE_ENABLE : 0);
}

static int axp2101_init(const struct device *dev)
{
	const struct axp2101_config *const config = dev->config;
	struct axp2101_data *data = dev->data;
	union charger_propval val;
	int ret;

	if (data->vbackup_enable) {
		ret = i2c_reg_write_byte_dt(&config->i2c, CHARGER_CONTROL,
					    BUTTON_BATTERY_CHARGE_ENABLE);
		if (ret) {
			return ret;
		}
	}

	val.const_charge_current_ua = M2U(data->cc_current_ma);
	ret = set_constant_charge_current_ua(dev, &val);
	if (ret < 0) {
		return ret;
	}

	val.const_charge_voltage_uv = M2U(data->cc_voltage_mv);
	ret = set_constant_charge_voltage_uv(dev, &val);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("AXP2101 initialized");

	return 0;
}

static DEVICE_API(charger, axp2101_driver_api) = {
	.get_property = axp2101_get_prop,
	.set_property = axp2101_set_prop,
	.charge_enable = axp2101_charge_enable,
};

#define AXP2101_INIT(inst)                                                                         \
	static const struct axp2101_config axp2101_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),                   \
	};                                                                                         \
	static struct axp2101_data axp2101_data_##inst = {                                         \
		.cc_current_ma = DT_INST_PROP(inst, constant_charge_current_ma),                   \
		.cc_voltage_mv = DT_INST_PROP(inst, constant_charge_voltage_mv),                   \
		.vbackup_enable = DT_INST_PROP_OR(inst, vbackup_enable, false),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, axp2101_init, NULL, &axp2101_data_##inst,                      \
			      &axp2101_config_##inst, POST_KERNEL,                                 \
			      CONFIG_CHARGER_AXP2101_INIT_PRIORITY,                                \
			      &axp2101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXP2101_INIT)
