/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silergy_sy6974b

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sy6974b, CONFIG_CHARGER_LOG_LEVEL);

enum sy6974b_reg {
	SY6974B_REG00_IINLIM = 0x00,
	SY6974B_REG01_POC = 0x01,
	SY6974B_REG02_CCC = 0x02,
	SY6974B_REG03_PCTCC = 0x03,
	SY6974B_REG04_CVC = 0x04,
	SY6974B_REG05_COC = 0x05,
	SY6974B_REG06_IOC = 0x06,
	SY6974B_REG07_MOC = 0x07,
	SY6974B_REG08_STATUS = 0x08,
	SY6974B_REG09_FAULT = 0x09,
	SY6974B_REG0A_VIC = 0x0A,
	SY6974B_REG0B_ID = 0x0B,
};

/* REG01: Power-On Configuration */
#define SY6974B_REG01_CHG_CONFIG BIT(4)

/* REG02: Charge Current Control */
#define SY6974B_REG02_ICHG_MASK GENMASK(5, 0)
#define SY6974B_ICHG_STEP_MA    60U
#define SY6974B_ICHG_MAX_MA     3000U

/* REG04: Charge Voltage Control */
#define SY6974B_REG04_VREG_MASK  GENMASK(7, 3)
#define SY6974B_REG04_VREG_SHIFT 3
#define SY6974B_VREG_BASE_MV     3856U
#define SY6974B_VREG_STEP_MV     32U
#define SY6974B_VREG_MAX_MV      4624U

/* REG05: Charge Operation Control */
#define SY6974B_REG05_WATCHDOG_MASK GENMASK(5, 4)

/* REG08: System Status (read-only) */
#define SY6974B_REG08_CHRG_STAT_MASK      GENMASK(4, 3)
#define SY6974B_REG08_CHRG_STAT_NOT       0x00
#define SY6974B_REG08_CHRG_STAT_PRECHARGE 0x01
#define SY6974B_REG08_CHRG_STAT_FAST      0x02
#define SY6974B_REG08_CHRG_STAT_DONE      0x03
#define SY6974B_REG08_PG_STAT             BIT(2)
#define SY6974B_REG08_THERM_STAT          BIT(1)

/* REG09: Fault status (read-only) */
#define SY6974B_REG09_WATCHDOG_FAULT  BIT(7)
#define SY6974B_REG09_BOOST_FAULT     BIT(6)
#define SY6974B_REG09_CHRG_FAULT_MASK GENMASK(5, 4)
#define SY6974B_REG09_BAT_FAULT       BIT(3)
#define SY6974B_REG09_NTC_FAULT_MASK  GENMASK(2, 0)

#define SY6974B_CHRG_FAULT_INPUT        0x01
#define SY6974B_CHRG_FAULT_TSHUT        0x02
#define SY6974B_CHRG_FAULT_SAFETY_TIMER 0x03

#define SY6974B_NTC_FAULT_WARM 0x02
#define SY6974B_NTC_FAULT_COOL 0x03
#define SY6974B_NTC_FAULT_COLD 0x05
#define SY6974B_NTC_FAULT_HOT  0x06

/* REG0A: VINDPM/IINDPM and interrupt mask register */
#define SY6974B_REG0A_BUS_GD BIT(7)

/* REG0B: Part/Revision ID */
#define SY6974B_REG0B_REG_RST BIT(7)
#define SY6974B_REG0B_PN_MASK GENMASK(6, 3)
#define SY6974B_REG0B_PN_VAL  (0x8U << 3)

struct sy6974b_config {
	struct i2c_dt_spec i2c;
	uint32_t initial_current_microamp;
	uint32_t max_voltage_microvolt;
};

static int sy6974b_set_charge_current(const struct device *dev, uint32_t current_ua)
{
	const struct sy6974b_config *cfg = dev->config;
	uint32_t current_ma = current_ua / 1000U;
	uint32_t clamped_ma = CLAMP(current_ma, 0U, SY6974B_ICHG_MAX_MA);
	uint8_t ichg;

	if (clamped_ma != current_ma) {
		LOG_WRN("charge current %u mA clamped to %u mA", current_ma, clamped_ma);
	}

	ichg = clamped_ma / SY6974B_ICHG_STEP_MA;

	return i2c_reg_update_byte_dt(&cfg->i2c, SY6974B_REG02_CCC, SY6974B_REG02_ICHG_MASK, ichg);
}

static int sy6974b_get_charge_current(const struct device *dev, uint32_t *current_ua)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG02_CCC, &val);
	if (ret < 0) {
		return ret;
	}

	*current_ua = FIELD_GET(SY6974B_REG02_ICHG_MASK, val) * SY6974B_ICHG_STEP_MA * 1000U;
	return 0;
}

static int sy6974b_set_charge_voltage(const struct device *dev, uint32_t voltage_uv)
{
	const struct sy6974b_config *cfg = dev->config;
	uint32_t voltage_mv = voltage_uv / 1000U;
	uint32_t clamped_mv = CLAMP(voltage_mv, SY6974B_VREG_BASE_MV, SY6974B_VREG_MAX_MV);
	uint8_t vreg;

	if (clamped_mv != voltage_mv) {
		LOG_WRN("charge voltage %u mV clamped to %u mV", voltage_mv, clamped_mv);
	}
	voltage_mv = clamped_mv;

	vreg = ((voltage_mv - SY6974B_VREG_BASE_MV) / SY6974B_VREG_STEP_MV)
	       << SY6974B_REG04_VREG_SHIFT;

	return i2c_reg_update_byte_dt(&cfg->i2c, SY6974B_REG04_CVC, SY6974B_REG04_VREG_MASK, vreg);
}

static int sy6974b_get_charge_voltage(const struct device *dev, uint32_t *voltage_uv)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG04_CVC, &val);
	if (ret < 0) {
		return ret;
	}

	*voltage_uv = (SY6974B_VREG_BASE_MV +
		       FIELD_GET(SY6974B_REG04_VREG_MASK, val) * SY6974B_VREG_STEP_MV) *
		      1000U;
	return 0;
}

static int sy6974b_charge_enable(const struct device *dev, bool enable)
{
	const struct sy6974b_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, SY6974B_REG01_POC, SY6974B_REG01_CHG_CONFIG,
				      enable ? SY6974B_REG01_CHG_CONFIG : 0);
}

static int sy6974b_get_online(const struct device *dev, enum charger_online *online)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG0A_VIC, &val);
	if (ret < 0) {
		return ret;
	}

	*online = (val & SY6974B_REG0A_BUS_GD) ? CHARGER_ONLINE_FIXED : CHARGER_ONLINE_OFFLINE;
	return 0;
}

static int sy6974b_get_status(const struct device *dev, enum charger_status *status)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t stat08, poc;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG08_STATUS, &stat08);
	if (ret < 0) {
		return ret;
	}

	if ((stat08 & SY6974B_REG08_PG_STAT) == 0) {
		*status = CHARGER_STATUS_DISCHARGING;
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG01_POC, &poc);
	if (ret < 0) {
		return ret;
	}

	if ((poc & SY6974B_REG01_CHG_CONFIG) == 0) {
		*status = CHARGER_STATUS_NOT_CHARGING;
		return 0;
	}

	switch (FIELD_GET(SY6974B_REG08_CHRG_STAT_MASK, stat08)) {
	case SY6974B_REG08_CHRG_STAT_NOT:
		*status = CHARGER_STATUS_NOT_CHARGING;
		break;
	case SY6974B_REG08_CHRG_STAT_PRECHARGE:
	case SY6974B_REG08_CHRG_STAT_FAST:
		*status = CHARGER_STATUS_CHARGING;
		break;
	case SY6974B_REG08_CHRG_STAT_DONE:
		*status = CHARGER_STATUS_FULL;
		break;
	default:
		*status = CHARGER_STATUS_UNKNOWN;
		break;
	}

	return 0;
}

static int sy6974b_get_charge_type(const struct device *dev, enum charger_charge_type *charge_type)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t poc, stat08;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG01_POC, &poc);
	if (ret < 0) {
		return ret;
	}

	if ((poc & SY6974B_REG01_CHG_CONFIG) == 0) {
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
		return 0;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG08_STATUS, &stat08);
	if (ret < 0) {
		return ret;
	}

	switch (FIELD_GET(SY6974B_REG08_CHRG_STAT_MASK, stat08)) {
	case SY6974B_REG08_CHRG_STAT_NOT:
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
		break;
	case SY6974B_REG08_CHRG_STAT_PRECHARGE:
		*charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
		break;
	case SY6974B_REG08_CHRG_STAT_FAST:
		*charge_type = (stat08 & SY6974B_REG08_THERM_STAT) ? CHARGER_CHARGE_TYPE_ADAPTIVE
								   : CHARGER_CHARGE_TYPE_FAST;
		break;
	case SY6974B_REG08_CHRG_STAT_DONE:
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
		break;
	default:
		*charge_type = CHARGER_CHARGE_TYPE_UNKNOWN;
		break;
	}

	return 0;
}

static int sy6974b_get_health(const struct device *dev, enum charger_health *health)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t fault;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG09_FAULT, &fault);
	if (ret < 0) {
		return ret;
	}

	if (fault & SY6974B_REG09_NTC_FAULT_MASK) {
		switch (FIELD_GET(SY6974B_REG09_NTC_FAULT_MASK, fault)) {
		case SY6974B_NTC_FAULT_WARM:
			*health = CHARGER_HEALTH_WARM;
			return 0;
		case SY6974B_NTC_FAULT_COOL:
			*health = CHARGER_HEALTH_COOL;
			return 0;
		case SY6974B_NTC_FAULT_COLD:
			*health = CHARGER_HEALTH_COLD;
			return 0;
		case SY6974B_NTC_FAULT_HOT:
			*health = CHARGER_HEALTH_HOT;
			return 0;
		default:
			*health = CHARGER_HEALTH_UNKNOWN;
			return 0;
		}
	}

	if (fault & SY6974B_REG09_BAT_FAULT) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
		return 0;
	}

	switch (FIELD_GET(SY6974B_REG09_CHRG_FAULT_MASK, fault)) {
	case SY6974B_CHRG_FAULT_INPUT:
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		return 0;
	case SY6974B_CHRG_FAULT_TSHUT:
		*health = CHARGER_HEALTH_OVERHEAT;
		return 0;
	case SY6974B_CHRG_FAULT_SAFETY_TIMER:
		*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
		return 0;
	default:
		break;
	}

	if (fault & SY6974B_REG09_BOOST_FAULT) {
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		return 0;
	}

	if (fault & SY6974B_REG09_WATCHDOG_FAULT) {
		*health = CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE;
		return 0;
	}

	*health = CHARGER_HEALTH_GOOD;
	return 0;
}

static int sy6974b_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return sy6974b_get_online(dev, &val->online);
	case CHARGER_PROP_CHARGE_TYPE:
		return sy6974b_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return sy6974b_get_health(dev, &val->health);
	case CHARGER_PROP_STATUS:
		return sy6974b_get_status(dev, &val->status);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return sy6974b_get_charge_current(dev, &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return sy6974b_get_charge_voltage(dev, &val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int sy6974b_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return sy6974b_set_charge_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return sy6974b_set_charge_voltage(dev, val->const_charge_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(charger, sy6974b_api) = {
	.get_property = sy6974b_get_prop,
	.set_property = sy6974b_set_prop,
	.charge_enable = sy6974b_charge_enable,
};

static int sy6974b_init(const struct device *dev)
{
	const struct sy6974b_config *cfg = dev->config;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, SY6974B_REG0B_ID, &val);
	if (ret < 0) {
		LOG_ERR("failed to read part ID (rc=%d)", ret);
		return ret;
	}

	if ((val & SY6974B_REG0B_PN_MASK) != SY6974B_REG0B_PN_VAL) {
		LOG_ERR("unexpected part number: 0x%02x", val & SY6974B_REG0B_PN_MASK);
		return -ENODEV;
	}

	/* Reset to known defaults and wait for the device to settle. */
	ret = i2c_reg_update_byte_dt(&cfg->i2c, SY6974B_REG0B_ID, SY6974B_REG0B_REG_RST,
				     SY6974B_REG0B_REG_RST);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Disable the I2C watchdog so that our configuration sticks even if
	 * the host is not periodically kicking the device.
	 */
	ret = i2c_reg_update_byte_dt(&cfg->i2c, SY6974B_REG05_COC, SY6974B_REG05_WATCHDOG_MASK, 0);
	if (ret < 0) {
		return ret;
	}

	ret = sy6974b_set_charge_voltage(dev, cfg->max_voltage_microvolt);
	if (ret < 0) {
		LOG_ERR("failed to set charge voltage (rc=%d)", ret);
		return ret;
	}

	ret = sy6974b_set_charge_current(dev, cfg->initial_current_microamp);
	if (ret < 0) {
		LOG_ERR("failed to set charge current (rc=%d)", ret);
		return ret;
	}

	return 0;
}

#define SY6974B_INIT(inst)                                                                         \
	static const struct sy6974b_config sy6974b_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.initial_current_microamp =                                                        \
			DT_INST_PROP(inst, constant_charge_current_max_microamp),                  \
		.max_voltage_microvolt =                                                           \
			DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sy6974b_init, NULL, NULL, &sy6974b_config_##inst, POST_KERNEL, \
			      CONFIG_CHARGER_INIT_PRIORITY, &sy6974b_api);

DT_INST_FOREACH_STATUS_OKAY(SY6974B_INIT)
