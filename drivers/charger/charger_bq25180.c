/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BQ25180 Datasheet: https://www.ti.com/lit/gpn/bq25180
 */

#define DT_DRV_COMPAT ti_bq25180

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bq25180, CONFIG_CHARGER_LOG_LEVEL);

#define BQ25180_STAT0 0x00
#define BQ25180_STAT1 0x01
#define BQ25180_FLAG0 0x02
#define BQ25180_VBAT_CTRL 0x03
#define BQ25180_ICHG_CTRL 0x04
#define BQ25180_IC_CTRL 0x07
#define BQ25180_SHIP_RST 0x09
#define BQ25180_MASK_ID 0x0c

#define BQ25180_ICHG_CHG_DIS BIT(7)
#define BQ25180_ICHG_MSK GENMASK(6, 0)
#define BQ25180_WATCHDOG_SEL_1_MSK GENMASK(1, 0)
#define BQ25180_WATCHDOG_DISABLE 0x03
#define BQ25180_DEVICE_ID_MSK GENMASK(3, 0)
#define BQ25180_DEVICE_ID 0x00
#define BQ25180_SHIP_RST_EN_RST_SHIP_MSK GENMASK(6, 5)
#define BQ25180_SHIP_RST_EN_RST_SHIP_ADAPTER 0x20
#define BQ25180_SHIP_RST_EN_RST_SHIP_BUTTON 0x40

/* Charging current limits */
#define BQ25180_CURRENT_MIN_MA 5
#define BQ25180_CURRENT_MAX_MA 1000

struct bq25180_config {
	struct i2c_dt_spec i2c;
	uint32_t initial_current_microamp;
};

/*
 * For ICHG <= 35mA = ICHGCODE + 5mA
 * For ICHG > 35mA = 40 + ((ICHGCODE-31)*10)mA.
 * Maximum programmable current = 1000mA
 *
 * Return: value between 0 and 127, negative on error.
 */
static int bq25180_ma_to_ichg(uint32_t current_ma, uint8_t *ichg)
{
	if (!IN_RANGE(current_ma, BQ25180_CURRENT_MIN_MA, BQ25180_CURRENT_MAX_MA)) {
		LOG_WRN("charging current out of range: %dmA, "
			"clamping to the nearest limit", current_ma);
	}
	current_ma = CLAMP(current_ma, BQ25180_CURRENT_MIN_MA, BQ25180_CURRENT_MAX_MA);

	if (current_ma <= 35) {
		*ichg = current_ma - 5;
		return 0;
	}

	*ichg = (current_ma - 40) / 10 + 31;

	return 0;
}

static uint32_t bq25180_ichg_to_ma(uint8_t ichg)
{
	ichg &= BQ25180_ICHG_MSK;

	if (ichg <= 30) {
		return (ichg + 5);
	}

	return (ichg - 31) * 10 + 40;
}

static int bq25183_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq25180_config *cfg = dev->config;
	uint8_t value = enable ? 0 : BQ25180_ICHG_CHG_DIS;
	int ret;

	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ25180_ICHG_CTRL,
				     BQ25180_ICHG_CHG_DIS, value);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq25180_set_charge_current(const struct device *dev,
				      uint32_t const_charge_current_ua)
{
	const struct bq25180_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = bq25180_ma_to_ichg(const_charge_current_ua / 1000, &val);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ25180_ICHG_CTRL,
				     BQ25180_ICHG_MSK, val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bq25180_get_charge_current(const struct device *dev,
				      uint32_t *const_charge_current_ua)
{
	const struct bq25180_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ25180_ICHG_CTRL, &val);
	if (ret < 0) {
		return ret;
	}

	*const_charge_current_ua = bq25180_ichg_to_ma(val) * 1000;

	return 0;
}

static int bq25180_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25180_get_charge_current(dev, &val->const_charge_current_ua);
	default:
		return -ENOTSUP;
	}
}

static int bq25180_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq25180_set_charge_current(dev, val->const_charge_current_ua);
	default:
		return -ENOTSUP;
	}
}

static const struct charger_driver_api bq25180_api = {
	.get_property = bq25180_get_prop,
	.set_property = bq25180_set_prop,
	.charge_enable = bq25183_charge_enable,
};

static int bq25180_init(const struct device *dev)
{
	const struct bq25180_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, BQ25180_MASK_ID, &val);
	if (ret < 0) {
		return ret;
	}

	val &= BQ25180_DEVICE_ID_MSK;
	if (val != BQ25180_DEVICE_ID) {
		LOG_ERR("Invalid device id: %02x", val);
		return -EINVAL;
	}

	/* Disable the watchdog */
	ret = i2c_reg_update_byte_dt(&cfg->i2c, BQ25180_IC_CTRL,
				     BQ25180_WATCHDOG_SEL_1_MSK,
				     BQ25180_WATCHDOG_DISABLE);
	if (ret < 0) {
		return ret;
	}

	if (cfg->initial_current_microamp > 0) {
		bq25180_set_charge_current(dev, cfg->initial_current_microamp);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define CHARGER_BQ25180_INIT(inst)								\
	static const struct bq25180_config bq25180_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.initial_current_microamp = DT_INST_PROP(					\
				inst, constant_charge_current_max_microamp),			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, bq25180_init, NULL, NULL,					\
			      &bq25180_config_##inst, POST_KERNEL,				\
			      CONFIG_CHARGER_INIT_PRIORITY,					\
			      &bq25180_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_BQ25180_INIT)
