/*
 * Copyright 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT x_powers_axp2101_charger

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(charger_axp2101, CONFIG_CHARGER_LOG_LEVEL);

#define AXP2101_PMU_STATUS1			0x00
	#define VBUS_GOOD_INDICATION		BIT(5)

#define AXP2101_PMU_STATUS2			0x01
	#define CHARGING_STATUS			GENMASK(2, 0)
	#define TRICKLE_CHARGE			0x0
	#define PRE_CHARGE			0x1
	#define CONSTANT_CURRENT		0x2
	#define CONSTANT_VOLTAGE		0x3
	#define CHARGE_DONE			0x4
	#define NOT_CHARGING			0x5

#define AXP2101_CHARGER_CONTROL			0x18
	#define BUTTON_BATTERY_CHARGE_ENABLE	BIT(2)
	#define CELL_BATTERY_CHARGE_ENABLE	BIT(1)

#define AXP2101_IPRECH_CHARGER_SETTING		0x61
	#define PRE_CHARGE_CURRENT_STEP_UA	25000

#define AXP2101_ICC_CHARGER_SETTING		0x62
#define AXP2101_ITERM_CHARGER_SETTING		0x63
	#define CHARGE_TERMINATION_ENABLE	BIT(4)
	#define TERMINATION_CURRENT_LIMIT	GENMASK(3, 0)
	#define TERMINATION_CURRENT_STEP_UA	25000

#define AXP2101_CV_CHARGER_VOLTAGE		0x64

struct axp2101_config {
	struct i2c_dt_spec i2c;
	bool vbackup_enable;
};

struct axp2101_data {
	uint32_t cc_current_ua;
	uint32_t cc_voltage_uv;
	uint32_t termination_current_ua;
};

static const uint32_t constant_charge_current_lut[] = {
	[0] = 0,
	[1] = 0,
	[2] = 0,
	[3] = 0,
	[4] = 100000,
	[5] = 125000,
	[6] = 150000,
	[7] = 175000,
	[8] = 200000,
	[9] = 300000,
	[10] = 400000,
	[11] = 500000,
	[12] = 600000,
	[13] = 700000,
	[14] = 800000,
	[15] = 900000,
	[16] = 1000000,
};

static const uint32_t constant_charge_voltage_lut[] = {
	[0] = 0,
	[1] = 4000000,
	[2] = 4100000,
	[3] = 4200000,
	[4] = 4350000,
	[5] = 4400000,
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

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_PMU_STATUS1, &tmp);
	if (ret < 0) {
		return ret;
	}

	val->online = (tmp & VBUS_GOOD_INDICATION) ?
		      CHARGER_ONLINE_FIXED : CHARGER_ONLINE_OFFLINE;

	return 0;
}

static int get_constant_charge_current_ua(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_ICC_CHARGER_SETTING, &tmp);
	if (ret < 0) {
		return ret;
	}

	val->const_charge_current_ua = constant_charge_current_lut[tmp];

	return 0;
}

static int set_constant_charge_current_ua(const struct device *dev,
					  const union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	struct axp2101_data *data = dev->data;
	int lut_index;

	lut_index = get_index_in_lut(val->const_charge_current_ua,
				     constant_charge_current_lut,
				     ARRAY_SIZE(constant_charge_current_lut));
	if (lut_index < 0) {
		return -EINVAL;
	}

	data->cc_current_ua = constant_charge_current_lut[lut_index];

	return i2c_reg_write_byte_dt(&config->i2c, AXP2101_ICC_CHARGER_SETTING,
				     (uint8_t) lut_index);
}

static int get_pre_charge_current_ua(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_IPRECH_CHARGER_SETTING, &tmp);
	if (ret < 0) {
		return ret;
	}
	val->precharge_current_ua = PRE_CHARGE_CURRENT_STEP_UA * ((uint32_t) tmp);

	return 0;
}

static int get_termination_current_ua(const struct device *dev,
				      union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_ITERM_CHARGER_SETTING, &tmp);
	if (ret < 0) {
		return ret;
	}

	if ((tmp & CHARGE_TERMINATION_ENABLE) == 0) {
		val->charge_term_current_ua = 0;
	} else {
		tmp = tmp & TERMINATION_CURRENT_LIMIT;
		val->charge_term_current_ua = TERMINATION_CURRENT_STEP_UA * ((uint32_t) tmp);
	}

	return 0;
}

static int set_termination_current_ua(const struct device *dev,
				      const union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint32_t mask, tmp = 0;

	tmp = val->charge_term_current_ua;
	if ((tmp > 200000) || ((tmp % TERMINATION_CURRENT_STEP_UA) != 0)) {
		return -EINVAL;
	}

	if (tmp != 0) {
		tmp = (uint8_t) (tmp / TERMINATION_CURRENT_STEP_UA);
		tmp |= CHARGE_TERMINATION_ENABLE;
	}

	mask = TERMINATION_CURRENT_LIMIT | CHARGE_TERMINATION_ENABLE;

	return i2c_reg_update_byte_dt(&config->i2c, AXP2101_ITERM_CHARGER_SETTING,
				      mask, tmp);
}

static int get_constant_charge_voltage_uv(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_CV_CHARGER_VOLTAGE, &tmp);
	if (ret < 0) {
		return ret;
	}

	if (tmp > ARRAY_SIZE(constant_charge_voltage_lut)) {
		return -EINVAL;
	}

	val->const_charge_voltage_uv = constant_charge_voltage_lut[tmp];

	return 0;
}

static int set_constant_charge_voltage_uv(const struct device *dev,
					  const union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	int lut_index;

	lut_index = get_index_in_lut(val->const_charge_voltage_uv,
				     constant_charge_voltage_lut,
				     ARRAY_SIZE(constant_charge_voltage_lut));
	if (lut_index < 0) {
		return -EINVAL;
	}

	return i2c_reg_write_byte_dt(&config->i2c, AXP2101_CV_CHARGER_VOLTAGE, (uint8_t) lut_index);
}

static int get_status(const struct device *dev, union charger_propval *val)
{
	const struct axp2101_config *const config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_PMU_STATUS2, &tmp);
	if (ret < 0) {
		return ret;
	}

	tmp = tmp & CHARGING_STATUS;
	if ((tmp >= TRICKLE_CHARGE) && (tmp <= CONSTANT_VOLTAGE)) {
		val->status = CHARGER_STATUS_CHARGING;
	} else if (tmp == CHARGE_DONE) {
		val->status = CHARGER_STATUS_FULL;
	} else if (tmp == NOT_CHARGING) {
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

	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_PMU_STATUS2, &tmp);
	if (ret < 0) {
		return ret;
	}

	tmp = tmp & CHARGING_STATUS;
	if ((tmp == TRICKLE_CHARGE) || (tmp == PRE_CHARGE)) {
		val->charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
	} else if ((tmp >= CONSTANT_CURRENT) && (tmp <= CHARGE_DONE)) {
		val->charge_type = CHARGER_CHARGE_TYPE_STANDARD;
	} else if (tmp == NOT_CHARGING) {
		val->charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int axp2101_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return is_charger_online(dev, val);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return get_constant_charge_current_ua(dev, val);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return get_constant_charge_voltage_uv(dev, val);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return get_pre_charge_current_ua(dev, val);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return get_termination_current_ua(dev, val);
	case CHARGER_PROP_CHARGE_TYPE:
		return get_charge_type(dev, val);
	case CHARGER_PROP_STATUS:
		return get_status(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int axp2101_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return set_constant_charge_current_ua(dev, val);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return set_constant_charge_voltage_uv(dev, val);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return set_termination_current_ua(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int axp2101_charge_enable(const struct device *dev, const bool enable)
{
	const struct axp2101_config *const config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_CONTROL,
				      CELL_BATTERY_CHARGE_ENABLE,
				      (enable) ? CELL_BATTERY_CHARGE_ENABLE : 0);
}

static int axp2101_init(const struct device *dev)
{
	const struct axp2101_config *const config = dev->config;
	struct axp2101_data *data = dev->data;
	union charger_propval val;
	int ret;

	if (config->vbackup_enable) {
		ret = i2c_reg_update_byte_dt(&config->i2c, AXP2101_CHARGER_CONTROL,
					     BUTTON_BATTERY_CHARGE_ENABLE,
					     BUTTON_BATTERY_CHARGE_ENABLE);
		if (ret < 0) {
			return ret;
		}
	}

	val.const_charge_current_ua = data->cc_current_ua;
	ret = set_constant_charge_current_ua(dev, &val);
	if (ret < 0) {
		return ret;
	}

	val.const_charge_voltage_uv = data->cc_voltage_uv;
	ret = set_constant_charge_voltage_uv(dev, &val);
	if (ret < 0) {
		return ret;
	}

	val.charge_term_current_ua = data->termination_current_ua;
	ret = set_termination_current_ua(dev, &val);
	if (ret < 0) {
		return ret;
	}

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
		.vbackup_enable = DT_INST_PROP(inst, vbackup_enable),                              \
	};                                                                                         \
	static struct axp2101_data axp2101_data_##inst = {                                         \
		.cc_current_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),         \
		.cc_voltage_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),        \
		.termination_current_ua = DT_INST_PROP_OR(inst, charge-term-current-microamp,      \
							  125000),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, axp2101_init, NULL, &axp2101_data_##inst,                      \
			      &axp2101_config_##inst, POST_KERNEL,                                 \
			      CONFIG_CHARGER_INIT_PRIORITY,                                        \
			      &axp2101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXP2101_INIT)
