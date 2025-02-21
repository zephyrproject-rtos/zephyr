/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq24190

#include <errno.h>

#include "bq24190.h"

#include "zephyr/device.h"
#include "zephyr/drivers/charger.h"
#include "zephyr/drivers/i2c.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/util.h"
#include "zephyr/logging/log.h"
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(ti_bq24190);

struct bq24190_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec ce_gpio;
};

struct bq24190_data {
	uint8_t ss_reg;
	unsigned int ichg_ua;
	unsigned int vreg_uv;
	enum charger_status state;
	enum charger_online online;
};

static int bq24190_register_reset(const struct device *dev)
{
	const struct bq24190_config *const config = dev->config;
	int ret, limit = BQ24190_RESET_MAX_TRIES;
	uint8_t val;

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ24190_REG_POC, BQ24190_REG_POC_RESET_MASK,
				     BQ24190_REG_POC_RESET_MASK);
	if (ret) {
		return ret;
	}

	/*
	 * No explicit reset timing characteristcs are provided in the datasheet.
	 * Instead, poll every 100µs for 100 attempts to see if the reset request
	 * bit has cleared.
	 */
	do {
		ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_POC, &val);
		if (ret) {
			return ret;
		}

		if (!(val & BQ24190_REG_POC_RESET_MASK)) {
			return 0;
		}

		k_usleep(100);
	} while (--limit);

	return -EIO;
}

static int bq24190_charger_get_charge_type(const struct device *dev,
					   enum charger_charge_type *charge_type)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;
	int ret;

	*charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_POC, &v);
	if (ret) {
		return ret;
	}

	v = FIELD_GET(BQ24190_REG_POC_CHG_CONFIG_MASK, v);

	if (!v) {
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
	} else {
		ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_CCC, &v);
		if (ret) {
			return ret;
		}

		v = FIELD_GET(BQ24190_REG_CCC_FORCE_20PCT_MASK, v);

		if (v) {
			*charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
		} else {
			*charge_type = CHARGER_CHARGE_TYPE_FAST;
		}
	}

	return 0;
}

static int bq24190_charger_get_health(const struct device *dev, enum charger_health *health)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_F, &v);
	if (ret) {
		return ret;
	}

	if (v & BQ24190_REG_F_NTC_FAULT_MASK) {
		switch (v >> BQ24190_REG_F_NTC_FAULT_SHIFT & 0x7) {
		case BQ24190_NTC_FAULT_TS1_COLD:
		case BQ24190_NTC_FAULT_TS2_COLD:
		case BQ24190_NTC_FAULT_TS1_TS2_COLD:
			*health = CHARGER_HEALTH_COLD;
			break;
		case BQ24190_NTC_FAULT_TS1_HOT:
		case BQ24190_NTC_FAULT_TS2_HOT:
		case BQ24190_NTC_FAULT_TS1_TS2_HOT:
			*health = CHARGER_HEALTH_HOT;
			break;
		default:
			*health = CHARGER_HEALTH_UNKNOWN;
		}
	} else if (v & BQ24190_REG_F_BAT_FAULT_MASK) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
	} else if (v & BQ24190_REG_F_CHRG_FAULT_MASK) {
		switch (v >> BQ24190_REG_F_CHRG_FAULT_SHIFT & 0x3) {
		case BQ24190_CHRG_FAULT_INPUT_FAULT:
			/*
			 * This could be over-voltage or under-voltage
			 * and there's no way to tell which.  Instead
			 * of looking foolish and returning 'OVERVOLTAGE'
			 * when its really under-voltage, just return
			 * 'UNSPEC_FAILURE'.
			 */
			*health = CHARGER_HEALTH_UNSPEC_FAILURE;
			break;
		case BQ24190_CHRG_FAULT_TSHUT:
			*health = CHARGER_HEALTH_OVERHEAT;
			break;
		case BQ24190_CHRG_SAFETY_TIMER:
			*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
			break;
		default: /* prevent compiler warning */
			*health = CHARGER_HEALTH_UNKNOWN;
		}
	} else if (v & BQ24190_REG_F_BOOST_FAULT_MASK) {
		/*
		 * This could be over-current or over-voltage but there's
		 * no way to tell which.  Return 'OVERVOLTAGE' since there
		 * isn't an 'OVERCURRENT' value defined that we can return
		 * even if it was over-current.
		 */
		*health = CHARGER_HEALTH_OVERVOLTAGE;
	} else {
		*health = CHARGER_HEALTH_GOOD;
	}

	return 0;
}

static int bq24190_charger_get_online(const struct device *dev, enum charger_online *online)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t pg_stat, batfet_disable;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_SS, &pg_stat);
	if (ret) {
		return ret;
	}

	pg_stat = FIELD_GET(BQ24190_REG_SS_PG_STAT_MASK, pg_stat);

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_MOC, &batfet_disable);
	if (ret) {
		return ret;
	}

	batfet_disable = FIELD_GET(BQ24190_REG_MOC_BATFET_DISABLE_MASK, batfet_disable);

	if (pg_stat && !batfet_disable) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int bq24190_charger_get_status(const struct device *dev, enum charger_status *status)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t ss_reg, chrg_fault;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_F, &chrg_fault);
	if (ret) {
		return ret;
	}

	chrg_fault = FIELD_GET(BQ24190_REG_F_CHRG_FAULT_MASK, chrg_fault);

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_SS, &ss_reg);
	if (ret) {
		return ret;
	}

	/*
	 * The battery must be discharging when any of these are true:
	 * - there is no good power source;
	 * - there is a charge fault.
	 * Could also be discharging when in "supplement mode" but
	 * there is no way to tell when its in that mode.
	 */
	if (!(ss_reg & BQ24190_REG_SS_PG_STAT_MASK) || chrg_fault) {
		*status = CHARGER_STATUS_DISCHARGING;
	} else {
		ss_reg = FIELD_GET(BQ24190_REG_SS_CHRG_STAT_MASK, ss_reg);

		switch (ss_reg) {
		case BQ24190_CHRG_STAT_NOT_CHRGING:
			*status = CHARGER_STATUS_NOT_CHARGING;
			break;
		case BQ24190_CHRG_STAT_PRECHRG:
		case BQ24190_CHRG_STAT_FAST_CHRG:
			*status = CHARGER_STATUS_CHARGING;
			break;
		case BQ24190_CHRG_STAT_CHRG_TERM:
			*status = CHARGER_STATUS_FULL;
			break;
		default:
			return -EIO;
		}
	}

	return 0;
}

static int bq24190_charger_get_constant_charge_current(const struct device *dev,
						       uint32_t *current_ua)
{
	const struct bq24190_config *const config = dev->config;
	bool frc_20pct;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_CCC, &v);
	if (ret) {
		return ret;
	}

	frc_20pct = v & BQ24190_REG_CCC_FORCE_20PCT_MASK;

	v = FIELD_GET(BQ24190_REG_CCC_ICHG_MASK, v);

	*current_ua = (v * BQ24190_REG_CCC_ICHG_STEP_UA) + BQ24190_REG_CCC_ICHG_OFFSET_UA;

	if (frc_20pct) {
		*current_ua /= 5;
	}

	return 0;
}

static int bq24190_charger_get_precharge_current(const struct device *dev, uint32_t *current_ua)
{
	const struct bq24190_config *const config = dev->config;
	bool frc_20pct;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_CCC, &v);
	if (ret) {
		return ret;
	}

	frc_20pct = v & BQ24190_REG_CCC_FORCE_20PCT_MASK;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_PCTCC, &v);
	if (ret) {
		return ret;
	}

	v = FIELD_GET(BQ24190_REG_PCTCC_IPRECHG_MASK, v);

	*current_ua = (v * BQ24190_REG_PCTCC_IPRECHG_STEP_UA) + BQ24190_REG_PCTCC_IPRECHG_OFFSET_UA;

	if (frc_20pct) {
		*current_ua /= 2;
	}

	return 0;
}

static int bq24190_charger_get_charge_term_current(const struct device *dev, uint32_t *current_ua)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_PCTCC, &v);
	if (ret) {
		return ret;
	}

	v = FIELD_GET(BQ24190_REG_PCTCC_ITERM_MASK, v);

	*current_ua = (v * BQ24190_REG_PCTCC_ITERM_STEP_UA) + BQ24190_REG_PCTCC_ITERM_OFFSET_UA;

	return 0;
}

static int bq24190_get_constant_charge_voltage(const struct device *dev, uint32_t *voltage_uv)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_CVC, &v);
	if (ret < 0) {
		return ret;
	}

	v = FIELD_GET(BQ24190_REG_CVC_VREG_MASK, v);

	*voltage_uv = (v * BQ24190_REG_CVC_VREG_STEP_UV) + BQ24190_REG_CVC_VREG_OFFSET_UV;

	return 0;
}

static int bq24190_set_constant_charge_current(const struct device *dev, uint32_t current_ua)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_CCC, &v);
	if (ret < 0) {
		return ret;
	}

	v &= BQ24190_REG_CCC_FORCE_20PCT_MASK;

	if (v) {
		current_ua *= 5;
	}

	current_ua = CLAMP(current_ua, BQ24190_REG_CCC_ICHG_MIN_UA, BQ24190_REG_CCC_ICHG_MAX_UA);

	v = (current_ua - BQ24190_REG_CCC_ICHG_OFFSET_UA) / BQ24190_REG_CCC_ICHG_STEP_UA;

	v = FIELD_PREP(BQ24190_REG_CCC_ICHG_MASK, v);

	return i2c_reg_update_byte_dt(&config->i2c, BQ24190_REG_CCC, BQ24190_REG_CCC_ICHG_MASK, v);
}

static int bq24190_set_constant_charge_voltage(const struct device *dev, uint32_t voltage_uv)
{
	const struct bq24190_config *const config = dev->config;
	uint8_t v;

	voltage_uv = CLAMP(voltage_uv, BQ24190_REG_CVC_VREG_MIN_UV, BQ24190_REG_CVC_VREG_MAX_UV);

	v = (voltage_uv - BQ24190_REG_CVC_VREG_OFFSET_UV) / BQ24190_REG_CVC_VREG_STEP_UV;

	v = FIELD_PREP(BQ24190_REG_CVC_VREG_MASK, v);

	return i2c_reg_update_byte_dt(&config->i2c, BQ24190_REG_CVC, BQ24190_REG_CVC_VREG_MASK, v);
}

static int bq24190_set_config(const struct device *dev)
{
	struct bq24190_data *data = dev->data;
	union charger_propval val;
	int ret;

	val.const_charge_current_ua = data->ichg_ua;

	ret = bq24190_set_constant_charge_current(dev, val.const_charge_current_ua);
	if (ret < 0) {
		return ret;
	}

	val.const_charge_voltage_uv = data->vreg_uv;

	return bq24190_set_constant_charge_voltage(dev, val.const_charge_voltage_uv);
}

static int bq24190_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq24190_charger_get_online(dev, &val->online);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq24190_charger_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return bq24190_charger_get_health(dev, &val->health);
	case CHARGER_PROP_STATUS:
		return bq24190_charger_get_status(dev, &val->status);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq24190_charger_get_constant_charge_current(dev,
								   &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq24190_get_constant_charge_voltage(dev, &val->const_charge_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq24190_charger_get_precharge_current(dev, &val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq24190_charger_get_charge_term_current(dev, &val->charge_term_current_ua);
	default:
		return -ENOTSUP;
	}
}

static int bq24190_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq24190_set_constant_charge_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq24190_set_constant_charge_voltage(dev, val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq24190_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq24190_config *const config = dev->config;

	if (config->ce_gpio.port != NULL) {
		if (enable == true) {
			return gpio_pin_set_dt(&config->ce_gpio, 1);
		} else {
			return gpio_pin_set_dt(&config->ce_gpio, 0);
		}
	} else {
		return -ENOTSUP;
	}
}

static int bq24190_init(const struct device *dev)
{
	const struct bq24190_config *const config = dev->config;
	struct bq24190_data *data = dev->data;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_VPRS, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(BQ24190_REG_VPRS_PN_MASK, val);

	switch (val) {
	case BQ24190_REG_VPRS_PN_24190:
	case BQ24190_REG_VPRS_PN_24192:
	case BQ24190_REG_VPRS_PN_24192I:
		break;
	default:
		LOG_ERR("Error unknown model: 0x%02x\n", val);
		return -ENODEV;
	}

	if (config->ce_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->ce_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->ce_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	} else {
		LOG_DBG("Assuming charge enable pin is pulled low");
	}

	ret = bq24190_register_reset(dev);
	if (ret) {
		return ret;
	}

	ret = bq24190_set_config(dev);
	if (ret) {
		return ret;
	}

	return i2c_reg_read_byte_dt(&config->i2c, BQ24190_REG_SS, &data->ss_reg);
}

static DEVICE_API(charger, bq24190_driver_api) = {
	.get_property = bq24190_get_prop,
	.set_property = bq24190_set_prop,
	.charge_enable = bq24190_charge_enable,
};

#define BQ24190_INIT(inst)                                                                         \
                                                                                                   \
	static const struct bq24190_config bq24190_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {}),                           \
	};                                                                                         \
                                                                                                   \
	static struct bq24190_data bq24190_data_##inst = {                                         \
		.ichg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),               \
		.vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq24190_init, NULL, &bq24190_data_##inst,                      \
			      &bq24190_config_##inst, POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY,   \
			      &bq24190_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ24190_INIT)
