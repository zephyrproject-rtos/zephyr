/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bq2589x, CONFIG_CHARGER_LOG_LEVEL);

/* Register 0x00 - Input Current Limit */
#define BQ2589X_REG_INPUT_CURRENT 0x00

#define BQ2589X_IINLIM_MASK GENMASK(5, 0)

#define BQ2589X_IINLIM_OFFSET_UA 100000
#define BQ2589X_IINLIM_STEP_UA   50000
#define BQ2589X_IINLIM_MIN_UA    100000
#define BQ2589X_IINLIM_MAX_UA    3250000

/* Register 0x03 - Charge Control */
#define BQ2589X_REG_CHARGE_CTRL 0x03

#define BQ2589X_CHG_CONFIG BIT(4)

/* Register 0x04 - Fast Charge Current */
#define BQ2589X_REG_CHARGE_CURRENT 0x04

#define BQ2589X_ICHG_MASK GENMASK(6, 0)

#define BQ2589X_ICHG_STEP_UA 64000

/* Register 0x05 - Pre-Charge and Termination Current */
#define BQ2589X_REG_PRECHG_TERM_CURRENT 0x05

#define BQ2589X_IPRECHG_MASK GENMASK(7, 4)
#define BQ2589X_ITERM_MASK   GENMASK(3, 0)

#define BQ2589X_IPRECHG_OFFSET_UA 64000
#define BQ2589X_IPRECHG_STEP_UA   64000
#define BQ2589X_IPRECHG_MIN_UA    64000
#define BQ2589X_IPRECHG_MAX_UA    1024000
#define BQ2589X_ITERM_OFFSET_UA   64000
#define BQ2589X_ITERM_STEP_UA     64000
#define BQ2589X_ITERM_MIN_UA      64000
#define BQ2589X_ITERM_MAX_UA      1024000

/* Register 0x06 - Charge Voltage */
#define BQ2589X_REG_CHARGE_VOLTAGE 0x06

#define BQ2589X_VREG_MASK GENMASK(7, 2)

#define BQ2589X_VREG_OFFSET_UV 3840000
#define BQ2589X_VREG_STEP_UV   16000
#define BQ2589X_VREG_MIN_UV    3840000
#define BQ2589X_VREG_MAX_UV    4608000

/* Register 0x07 - Termination and Timer Control */
#define BQ2589X_REG_TERM_TIMER 0x07

#define BQ2589X_WATCHDOG_MASK GENMASK(5, 4)

#define BQ2589X_WATCHDOG_DISABLE 0x0

/* Register 0x0B - System Status */
#define BQ2589X_REG_STATUS 0x0B

#define BQ2589X_VBUS_STAT_MASK GENMASK(7, 5)
#define BQ2589X_CHRG_STAT_MASK GENMASK(4, 3)
#define BQ2589X_PG_STAT        BIT(2)

#define BQ2589X_VBUS_STAT_OTG          0x7
#define BQ2589X_CHRG_STAT_NOT_CHARGING 0x0
#define BQ2589X_CHRG_STAT_PRECHARGE    0x1
#define BQ2589X_CHRG_STAT_FAST_CHARGE  0x2
#define BQ2589X_CHRG_STAT_DONE         0x3

/* Register 0x0C - Fault */
#define BQ2589X_REG_FAULT 0x0C

#define BQ2589X_WATCHDOG_FAULT  BIT(7)
#define BQ2589X_CHRG_FAULT_MASK GENMASK(5, 4)
#define BQ2589X_BAT_FAULT       BIT(3)
#define BQ2589X_NTC_FAULT_MASK  GENMASK(2, 0)

#define BQ2589X_CHRG_FAULT_NORMAL  0x0
#define BQ2589X_CHRG_FAULT_INPUT   0x1
#define BQ2589X_CHRG_FAULT_THERMAL 0x2
#define BQ2589X_CHRG_FAULT_TIMER   0x3

#define BQ2589X_NTC_FAULT_NORMAL 0x0
#define BQ2589X_NTC_FAULT_WARM   0x2
#define BQ2589X_NTC_FAULT_COOL   0x3
#define BQ2589X_NTC_FAULT_COLD   0x5
#define BQ2589X_NTC_FAULT_HOT    0x6

/* Register 0x0D - Input Voltage Limit */
#define BQ2589X_REG_VINDPM 0x0D

#define BQ2589X_FORCE_VINDPM BIT(7)
#define BQ2589X_VINDPM_MASK  GENMASK(6, 0)

#define BQ2589X_VINDPM_OFFSET_UV 2600000
#define BQ2589X_VINDPM_STEP_UV   100000
#define BQ2589X_VINDPM_MIN_UV    3900000
#define BQ2589X_VINDPM_MAX_UV    15300000

/* Register 0x14 - Device Information */
#define BQ2589X_REG_DEVICE_INFO 0x14

#define BQ2589X_PN_MASK      GENMASK(5, 3)
#define BQ2589X_DEV_REV_MASK GENMASK(1, 0)

#define BQ25896_PART_NUMBER 0x0
#define SY6970_PART_NUMBER  0x1

struct bq2589x_config {
	struct i2c_dt_spec i2c;
	uint32_t ichg_ua;
	uint32_t vreg_uv;
	uint32_t iprechg_ua;
	uint32_t iterm_ua;
	uint32_t ichg_max_ua;
	uint8_t part_no;
};

static int bq2589x_field_read(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t *value)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t tmp;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &tmp);
	if (ret < 0) {
		return ret;
	}

	*value = FIELD_GET(mask, tmp);

	return 0;
}

static int bq2589x_field_write(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
	const struct bq2589x_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, reg, mask, FIELD_PREP(mask, value));
}

static int bq2589x_identify(const struct device *dev)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t reg, part_no, dev_rev;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_DEVICE_INFO, &reg);
	if (ret < 0) {
		LOG_ERR("Failed to read device information register (%d)", ret);
		return ret;
	}

	part_no = FIELD_GET(BQ2589X_PN_MASK, reg);
	if (part_no != config->part_no) {
		LOG_ERR("Unexpected part number: 0x%x", part_no);
		return -ENODEV;
	}

	dev_rev = FIELD_GET(BQ2589X_DEV_REV_MASK, reg);
	LOG_DBG("PN=0x%x, DEV_REV=0x%x", part_no, dev_rev);

	return 0;
}

static int bq2589x_get_online(const struct device *dev, enum charger_online *online)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t status;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}

	if (((status & BQ2589X_PG_STAT) != 0U) &&
	    (FIELD_GET(BQ2589X_VBUS_STAT_MASK, status) != BQ2589X_VBUS_STAT_OTG)) {
		*online = CHARGER_ONLINE_FIXED;
	} else {
		*online = CHARGER_ONLINE_OFFLINE;
	}

	return 0;
}

static int bq2589x_get_status(const struct device *dev, enum charger_status *status)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t reg;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_STATUS, &reg);
	if (ret < 0) {
		return ret;
	}

	switch (FIELD_GET(BQ2589X_CHRG_STAT_MASK, reg)) {
	case BQ2589X_CHRG_STAT_NOT_CHARGING:
		if ((reg & BQ2589X_PG_STAT) != 0U) {
			*status = CHARGER_STATUS_NOT_CHARGING;
		} else {
			*status = CHARGER_STATUS_DISCHARGING;
		}
		break;
	case BQ2589X_CHRG_STAT_PRECHARGE:
	case BQ2589X_CHRG_STAT_FAST_CHARGE:
		*status = CHARGER_STATUS_CHARGING;
		break;
	case BQ2589X_CHRG_STAT_DONE:
		*status = CHARGER_STATUS_FULL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2589x_get_charge_type(const struct device *dev, enum charger_charge_type *charge_type)
{
	uint8_t chrg;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_STATUS, BQ2589X_CHRG_STAT_MASK, &chrg);
	if (ret < 0) {
		return ret;
	}

	switch (chrg) {
	case BQ2589X_CHRG_STAT_PRECHARGE:
		*charge_type = CHARGER_CHARGE_TYPE_TRICKLE;
		break;
	case BQ2589X_CHRG_STAT_FAST_CHARGE:
		*charge_type = CHARGER_CHARGE_TYPE_FAST;
		break;
	default:
		*charge_type = CHARGER_CHARGE_TYPE_NONE;
		break;
	}

	return 0;
}

static int bq2589x_get_health(const struct device *dev, enum charger_health *health)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t fault;
	int ret;

	/* REG0C latches faults until it is read */
	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2589X_REG_FAULT, &fault);
	if (ret < 0) {
		return ret;
	}

	if ((fault & BQ2589X_BAT_FAULT) != 0U) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
		return 0;
	}

	switch (FIELD_GET(BQ2589X_NTC_FAULT_MASK, fault)) {
	case BQ2589X_NTC_FAULT_NORMAL:
		break;
	case BQ2589X_NTC_FAULT_WARM:
		*health = CHARGER_HEALTH_WARM;
		return 0;
	case BQ2589X_NTC_FAULT_COOL:
		*health = CHARGER_HEALTH_COOL;
		return 0;
	case BQ2589X_NTC_FAULT_COLD:
		*health = CHARGER_HEALTH_COLD;
		return 0;
	case BQ2589X_NTC_FAULT_HOT:
		*health = CHARGER_HEALTH_HOT;
		return 0;
	default:
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		return 0;
	}

	if ((fault & BQ2589X_WATCHDOG_FAULT) != 0U) {
		*health = CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE;
		return 0;
	}

	switch (FIELD_GET(BQ2589X_CHRG_FAULT_MASK, fault)) {
	case BQ2589X_CHRG_FAULT_NORMAL:
		*health = CHARGER_HEALTH_GOOD;
		break;
	case BQ2589X_CHRG_FAULT_INPUT:
		*health = CHARGER_HEALTH_UNSPEC_FAILURE;
		break;
	case BQ2589X_CHRG_FAULT_THERMAL:
		*health = CHARGER_HEALTH_OVERHEAT;
		break;
	case BQ2589X_CHRG_FAULT_TIMER:
		*health = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2589x_get_constant_charge_current(const struct device *dev, uint32_t *current_ua)
{
	uint8_t ichg;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_CHARGE_CURRENT, BQ2589X_ICHG_MASK, &ichg);
	if (ret < 0) {
		return ret;
	}

	*current_ua = ichg * BQ2589X_ICHG_STEP_UA;

	return 0;
}

static int bq2589x_set_constant_charge_current(const struct device *dev, uint32_t current_ua)
{
	const struct bq2589x_config *config = dev->config;

	if (current_ua > config->ichg_max_ua) {
		LOG_WRN("Charge current %u uA out of range, clamping", current_ua);
		current_ua = config->ichg_max_ua;
	}

	return bq2589x_field_write(dev, BQ2589X_REG_CHARGE_CURRENT, BQ2589X_ICHG_MASK,
				   current_ua / BQ2589X_ICHG_STEP_UA);
}

static int bq2589x_get_precharge_current(const struct device *dev, uint32_t *current_ua)
{
	uint8_t iprechg;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_PRECHG_TERM_CURRENT, BQ2589X_IPRECHG_MASK,
				 &iprechg);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2589X_IPRECHG_OFFSET_UA + (iprechg * BQ2589X_IPRECHG_STEP_UA);

	return 0;
}

static int bq2589x_set_precharge_current(const struct device *dev, uint32_t current_ua)
{
	if (!IN_RANGE(current_ua, BQ2589X_IPRECHG_MIN_UA, BQ2589X_IPRECHG_MAX_UA)) {
		LOG_WRN("Precharge current %u uA out of range, clamping", current_ua);
		current_ua = CLAMP(current_ua, BQ2589X_IPRECHG_MIN_UA, BQ2589X_IPRECHG_MAX_UA);
	}

	return bq2589x_field_write(dev, BQ2589X_REG_PRECHG_TERM_CURRENT, BQ2589X_IPRECHG_MASK,
				   (current_ua - BQ2589X_IPRECHG_OFFSET_UA) /
					   BQ2589X_IPRECHG_STEP_UA);
}

static int bq2589x_get_termination_current(const struct device *dev, uint32_t *current_ua)
{
	uint8_t iterm;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_PRECHG_TERM_CURRENT, BQ2589X_ITERM_MASK, &iterm);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2589X_ITERM_OFFSET_UA + (iterm * BQ2589X_ITERM_STEP_UA);

	return 0;
}

static int bq2589x_set_termination_current(const struct device *dev, uint32_t current_ua)
{
	if (!IN_RANGE(current_ua, BQ2589X_ITERM_MIN_UA, BQ2589X_ITERM_MAX_UA)) {
		LOG_WRN("Termination current %u uA out of range, clamping", current_ua);
		current_ua = CLAMP(current_ua, BQ2589X_ITERM_MIN_UA, BQ2589X_ITERM_MAX_UA);
	}

	return bq2589x_field_write(dev, BQ2589X_REG_PRECHG_TERM_CURRENT, BQ2589X_ITERM_MASK,
				   (current_ua - BQ2589X_ITERM_OFFSET_UA) / BQ2589X_ITERM_STEP_UA);
}

static int bq2589x_get_constant_charge_voltage(const struct device *dev, uint32_t *voltage_uv)
{
	uint8_t vreg;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_CHARGE_VOLTAGE, BQ2589X_VREG_MASK, &vreg);
	if (ret < 0) {
		return ret;
	}

	*voltage_uv =
		MIN(BQ2589X_VREG_OFFSET_UV + (vreg * BQ2589X_VREG_STEP_UV), BQ2589X_VREG_MAX_UV);

	return 0;
}

static int bq2589x_set_constant_charge_voltage(const struct device *dev, uint32_t voltage_uv)
{
	if (!IN_RANGE(voltage_uv, BQ2589X_VREG_MIN_UV, BQ2589X_VREG_MAX_UV)) {
		LOG_WRN("Charge voltage %u uV out of range, clamping", voltage_uv);
		voltage_uv = CLAMP(voltage_uv, BQ2589X_VREG_MIN_UV, BQ2589X_VREG_MAX_UV);
	}

	return bq2589x_field_write(dev, BQ2589X_REG_CHARGE_VOLTAGE, BQ2589X_VREG_MASK,
				   (voltage_uv - BQ2589X_VREG_OFFSET_UV) / BQ2589X_VREG_STEP_UV);
}

static int bq2589x_get_input_current_limit(const struct device *dev, uint32_t *current_ua)
{
	uint8_t iinlim;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_INPUT_CURRENT, BQ2589X_IINLIM_MASK, &iinlim);
	if (ret < 0) {
		return ret;
	}

	*current_ua = BQ2589X_IINLIM_OFFSET_UA + (iinlim * BQ2589X_IINLIM_STEP_UA);

	return 0;
}

static int bq2589x_set_input_current_limit(const struct device *dev, uint32_t current_ua)
{
	if (!IN_RANGE(current_ua, BQ2589X_IINLIM_MIN_UA, BQ2589X_IINLIM_MAX_UA)) {
		LOG_WRN("Input current %u uA out of range, clamping", current_ua);
		current_ua = CLAMP(current_ua, BQ2589X_IINLIM_MIN_UA, BQ2589X_IINLIM_MAX_UA);
	}

	return bq2589x_field_write(dev, BQ2589X_REG_INPUT_CURRENT, BQ2589X_IINLIM_MASK,
				   (current_ua - BQ2589X_IINLIM_OFFSET_UA) /
					   BQ2589X_IINLIM_STEP_UA);
}

static int bq2589x_get_vindpm(const struct device *dev, uint32_t *voltage_uv)
{
	uint8_t vindpm;
	int ret;

	ret = bq2589x_field_read(dev, BQ2589X_REG_VINDPM, BQ2589X_VINDPM_MASK, &vindpm);
	if (ret < 0) {
		return ret;
	}

	*voltage_uv = MAX(BQ2589X_VINDPM_OFFSET_UV + (vindpm * BQ2589X_VINDPM_STEP_UV),
			  BQ2589X_VINDPM_MIN_UV);

	return 0;
}

static int bq2589x_set_vindpm(const struct device *dev, uint32_t voltage_uv)
{
	const struct bq2589x_config *config = dev->config;
	uint8_t vindpm;

	if (!IN_RANGE(voltage_uv, BQ2589X_VINDPM_MIN_UV, BQ2589X_VINDPM_MAX_UV)) {
		LOG_WRN("Input regulation voltage %u uV out of range, clamping", voltage_uv);
		voltage_uv = CLAMP(voltage_uv, BQ2589X_VINDPM_MIN_UV, BQ2589X_VINDPM_MAX_UV);
	}

	vindpm = (voltage_uv - BQ2589X_VINDPM_OFFSET_UV) / BQ2589X_VINDPM_STEP_UV;

	/* Switch to an absolute threshold, the relative one tracks VBUS */
	return i2c_reg_write_byte_dt(&config->i2c, BQ2589X_REG_VINDPM,
				     BQ2589X_FORCE_VINDPM |
					     FIELD_PREP(BQ2589X_VINDPM_MASK, vindpm));
}

static int bq2589x_get_property(const struct device *dev, const charger_prop_t prop,
				union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq2589x_get_online(dev, &val->online);
	case CHARGER_PROP_STATUS:
		return bq2589x_get_status(dev, &val->status);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq2589x_get_charge_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return bq2589x_get_health(dev, &val->health);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2589x_get_constant_charge_current(dev, &val->const_charge_current_ua);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2589x_get_precharge_current(dev, &val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2589x_get_termination_current(dev, &val->charge_term_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2589x_get_constant_charge_voltage(dev, &val->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2589x_get_input_current_limit(dev,
						       &val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2589x_get_vindpm(dev, &val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq2589x_set_property(const struct device *dev, const charger_prop_t prop,
				const union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2589x_set_constant_charge_current(dev, val->const_charge_current_ua);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2589x_set_precharge_current(dev, val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2589x_set_termination_current(dev, val->charge_term_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2589x_set_constant_charge_voltage(dev, val->const_charge_voltage_uv);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2589x_set_input_current_limit(dev,
						       val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2589x_set_vindpm(dev, val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}
}

static int bq2589x_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq2589x_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, BQ2589X_REG_CHARGE_CTRL, BQ2589X_CHG_CONFIG,
				      enable ? BQ2589X_CHG_CONFIG : 0U);
}

static int bq2589x_init(const struct device *dev)
{
	const struct bq2589x_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = bq2589x_identify(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The driver does not service the I2C watchdog. On expiry the device
	 * would restore the default charge settings, so keep it disabled.
	 */
	ret = bq2589x_field_write(dev, BQ2589X_REG_TERM_TIMER, BQ2589X_WATCHDOG_MASK,
				  BQ2589X_WATCHDOG_DISABLE);
	if (ret < 0) {
		return ret;
	}

	ret = bq2589x_set_constant_charge_current(dev, config->ichg_ua);
	if (ret < 0) {
		return ret;
	}

	ret = bq2589x_set_constant_charge_voltage(dev, config->vreg_uv);
	if (ret < 0) {
		return ret;
	}

	if (config->iprechg_ua != 0U) {
		ret = bq2589x_set_precharge_current(dev, config->iprechg_ua);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->iterm_ua != 0U) {
		ret = bq2589x_set_termination_current(dev, config->iterm_ua);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(charger, bq2589x_api) = {
	.get_property = bq2589x_get_property,
	.set_property = bq2589x_set_property,
	.charge_enable = bq2589x_charge_enable,
};

#define BQ2589X_DEFINE(inst, name, pn, ichg_max)                                                   \
	static const struct bq2589x_config name##_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ichg_ua = DT_INST_PROP(inst, constant_charge_current_max_microamp),               \
		.vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),              \
		.iprechg_ua = DT_INST_PROP_OR(inst, precharge_current_microamp, 0),                \
		.iterm_ua = DT_INST_PROP_OR(inst, charge_term_current_microamp, 0),                \
		.ichg_max_ua = ichg_max,                                                           \
		.part_no = pn,                                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq2589x_init, NULL, NULL, &name##_config_##inst, POST_KERNEL,  \
			      CONFIG_CHARGER_INIT_PRIORITY, &bq2589x_api);

#define DT_DRV_COMPAT ti_bq25896
DT_INST_FOREACH_STATUS_OKAY_VARGS(BQ2589X_DEFINE, bq25896, BQ25896_PART_NUMBER, 3008000)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT silergy_sy6970
DT_INST_FOREACH_STATUS_OKAY_VARGS(BQ2589X_DEFINE, sy6970, SY6970_PART_NUMBER, 5056000)
#undef DT_DRV_COMPAT
