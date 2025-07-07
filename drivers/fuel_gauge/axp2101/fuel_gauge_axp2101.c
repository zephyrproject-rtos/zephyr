/*
 * Copyright (c) 2025, Felix Moessbauer
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Note: The functions that query the raw values via i2c are named similar
 * to the ones in the reference implementation in the XPowersLib. This also
 * applies to the register names (defines).
 *
 * Datasheet:
 * https://github.com/lewisxhe/XPowersLib/blob/a7d06b98c1136c8fee7854b1d29a9f012b2aba83/datasheet/AXP2101_Datasheet_V1.0_en.pdf
 */

#define DT_DRV_COMPAT x_powers_axp2101_fuel_gauge

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fuel_gauge_axp2101, CONFIG_FUEL_GAUGE_LOG_LEVEL);

/* clang-format off */
/* registers */
#define AXP2101_STATUS1			0x00
	#define BAT_PRESENT_MASK	BIT(3)
#define AXP2101_CHARGE_GAUGE_WDT_CTRL	0x18
	#define GAUGE_ENABLE_MASK	BIT(3)
#define AXP2101_ADC_DATA_VBAT_H		0x34
	#define GAUGE_VBAT_H_MASK	0x1F
#define AXP2101_ADC_DATA_VBAT_L		0x35
#define AXP2101_BAT_DET_CTRL		0x68
	#define BAT_TYPE_DET_MASK	BIT(0)
#define AXP2101_BAT_PERCENT_DATA	0xA4

/* internal feature flags */
#define GAUGE_FEATURE_BAT_DET		BIT(0)
#define GAUGE_FEATURE_GAUGE		BIT(1)
#define GAUGE_FEATURE_ALL		(GAUGE_FEATURE_BAT_DET | GAUGE_FEATURE_GAUGE)
/* clang-format on */

struct axp2101_config {
	struct i2c_dt_spec i2c;
};

struct axp2101_data {
	uint8_t features;
};

static int enable_fuel_gauge(const struct device *dev)
{
	const struct axp2101_config *cfg = dev->config;
	struct axp2101_data *data = dev->data;
	int ret = 0;

	ret = i2c_reg_update_byte_dt(&cfg->i2c, AXP2101_CHARGE_GAUGE_WDT_CTRL, GAUGE_ENABLE_MASK,
				     GAUGE_ENABLE_MASK);
	if (ret < 0) {
		data->features &= ~GAUGE_FEATURE_GAUGE;
	}
	return ret;
}

static int enable_batt_detection(const struct device *dev)
{
	const struct axp2101_config *cfg = dev->config;
	struct axp2101_data *data = dev->data;
	int ret = 0;

	ret = i2c_reg_update_byte_dt(&cfg->i2c, AXP2101_BAT_DET_CTRL, BAT_TYPE_DET_MASK,
				     BAT_TYPE_DET_MASK);
	if (ret < 0) {
		data->features &= ~GAUGE_FEATURE_BAT_DET;
	}
	return ret;
}

static int is_battery_connect(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct axp2101_config *cfg = dev->config;
	struct axp2101_data *data = dev->data;
	uint8_t tmp;
	int ret;

	if ((data->features & GAUGE_FEATURE_BAT_DET) == 0) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, AXP2101_STATUS1, &tmp);
	if (ret < 0) {
		return ret;
	}

	val->present_state = tmp & BAT_PRESENT_MASK;
	return 0;
}

static int get_battery_percent(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct axp2101_config *cfg = dev->config;
	struct axp2101_data *data = dev->data;
	uint8_t tmp;
	int ret;

	if ((data->features & GAUGE_FEATURE_GAUGE) == 0) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, AXP2101_BAT_PERCENT_DATA, &tmp);
	if (ret < 0) {
		return ret;
	}

	val->relative_state_of_charge = tmp;
	return 0;
}

static int get_bat_voltage(const struct device *dev, union fuel_gauge_prop_val *val)
{
	const struct axp2101_config *cfg = dev->config;
	struct axp2101_data *data = dev->data;
	uint8_t h5, l8;
	int ret;

	if ((data->features & GAUGE_FEATURE_GAUGE) == 0) {
		return -ENOTSUP;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, AXP2101_ADC_DATA_VBAT_H, &h5);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, AXP2101_ADC_DATA_VBAT_L, &l8);
	if (ret < 0) {
		return ret;
	}

	val->voltage = (((h5 & GAUGE_VBAT_H_MASK) << 8) | l8) * 1000;
	return 0;
}

static int axp2101_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val *val)
{
	switch (prop) {
	case FUEL_GAUGE_PRESENT_STATE:
	case FUEL_GAUGE_CONNECT_STATE:
		return is_battery_connect(dev, val);
	case FUEL_GAUGE_VOLTAGE:
		return get_bat_voltage(dev, val);
	case FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE:
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		return get_battery_percent(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int axp2101_init(const struct device *dev)
{
	struct axp2101_data *data = dev->data;
	const struct axp2101_config *cfg;
	int ret = 0;

	cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = enable_fuel_gauge(dev);
	if (ret < 0) {
		LOG_WRN("Failed to enable fuel gauge");
		data->features &= ~GAUGE_FEATURE_GAUGE;
	}

	ret = enable_batt_detection(dev);
	if (ret < 0) {
		LOG_WRN("Failed to enable battery detection");
		data->features &= ~GAUGE_FEATURE_BAT_DET;
	}
	return 0;
}

static DEVICE_API(fuel_gauge, axp2101_api) = {
	.get_property = axp2101_get_prop,
};

#define AXP2101_INIT(inst)                                                                         \
	static const struct axp2101_config axp2101_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),                   \
	};                                                                                         \
	static struct axp2101_data axp2101_data_##inst = {                                         \
		.features = GAUGE_FEATURE_ALL,                                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &axp2101_init, NULL, &axp2101_data_##inst,                     \
			      &axp2101_config_##inst, POST_KERNEL,                                 \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &axp2101_api);

DT_INST_FOREACH_STATUS_OKAY(AXP2101_INIT)
