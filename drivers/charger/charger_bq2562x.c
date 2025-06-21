/*
 * Copyright 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq2562x

#include "charger_bq2562x.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_bq25620, CONFIG_CHARGER_LOG_LEVEL);

struct bq2562x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec ce_gpio;
	struct gpio_dt_spec int_gpio;
};

struct bq2562x_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	charger_status_notifier_t charger_status_notifier;
	charger_online_notifier_t charger_online_notifier;
	struct k_work int_routine_work;
	uint32_t constant_charge_current_max_ua;
	uint32_t constant_charge_voltage_max_uv;
	uint32_t precharge_current_ua;
	uint32_t charge_term_current_ua;

	/* TI/Chip specific */
	uint32_t min_sys_voltage_uv;
	uint32_t input_voltage_min_uv;
	uint32_t input_current_max_ua;
	uint32_t thermal_regulation_threshold;
	uint32_t switching_converter_freq;
	uint32_t switching_converter_strength;
	enum charger_status state;
	enum charger_online online;
};

enum bq2562x_id {
	BQ25620,
	BQ25622,
};

static bool bq2562x_get_charge_enable(const struct device *dev)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_ctrl_0;
	int charger_enable;
	int ce_pin = 1;
	int ret;

	if (config->ce_gpio.port != NULL) {
		ce_pin = !gpio_pin_get_dt(&config->ce_gpio);
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_1, &chrg_ctrl_0);
	if (ret) {
		return ret;
	}

	charger_enable = chrg_ctrl_0 & BQ2562X_CHRG_EN;
	if (charger_enable != 0) {
		if (config->ce_gpio.port && !ce_pin) {
			return true;
		}
	}

	return false;
}

static int bq2562x_set_charge_enable(const struct device *dev, const bool enable)
{
	const struct bq2562x_config *const config = dev->config;
	int ret;

	if (config->ce_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->ce_gpio, enable);
		if (ret) {
			return ret;
		}
	}

	return i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_1, BQ2562X_CHRG_EN,
				      enable ? BQ2562X_CHRG_EN : 0);
}

/* Charge Current Limit */
static int bq2562x_get_ichrg_curr(const struct device *dev, uint32_t *current_ua)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t ichg[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_CHRG_I_LIM_LSB, ichg, ARRAY_SIZE(ichg));
	if (ret) {
		return ret;
	}

	*current_ua = ((ichg[1] << 8) | ichg[0]) >> BQ2562X_ICHG_I_SHIFT;
	*current_ua = *current_ua * BQ2562X_ICHG_I_STEP_UA;

	return 0;
}

static int bq2562x_set_ichrg_curr(const struct device *dev, int chrg_curr)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	int chrg_curr_max = data->constant_charge_current_max_ua;
	uint8_t ichg[2] = {0};
	int ret;

	chrg_curr = CLAMP(chrg_curr, BQ2562X_ICHG_I_MIN_UA, chrg_curr_max);

	(void)bq2562x_set_charge_enable(dev, 0);

	chrg_curr = ((chrg_curr / BQ2562X_ICHG_I_STEP_UA) << BQ2562X_ICHG_I_SHIFT);
	ichg[1] = (chrg_curr >> 8) & BQ2562X_ICHG_MSB_MSK;
	ichg[0] = chrg_curr & BQ2562X_ICHG_LSB_MSK;

	ret = i2c_burst_write_dt(&config->i2c, BQ2562X_CHRG_I_LIM_LSB, ichg, ARRAY_SIZE(ichg));

	(void)bq2562x_set_charge_enable(dev, 1);

	return ret;
}

/* Charge Voltage Limit */
static int bq2562x_get_chrg_volt(const struct device *dev, uint32_t *voltage_uv)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_volt[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_CHRG_V_LIM_LSB, chrg_volt,
				ARRAY_SIZE(chrg_volt));
	if (ret) {
		return ret;
	}

	*voltage_uv = ((chrg_volt[1] << 8) | chrg_volt[0]) >> BQ2562X_VREG_V_SHIFT;
	*voltage_uv = *voltage_uv * BQ2562X_VREG_V_STEP_UV;

	return 0;
}

static int bq2562x_set_chrg_volt(const struct device *dev, uint32_t chrg_volt)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	uint32_t chrg_volt_max = data->constant_charge_voltage_max_uv;
	uint8_t volt[2] = {0};

	chrg_volt = CLAMP(chrg_volt, BQ2562X_VREG_V_MIN_UV, chrg_volt_max);

	chrg_volt = (chrg_volt / BQ2562X_VREG_V_STEP_UV) << BQ2562X_VREG_V_SHIFT;
	volt[1] = (chrg_volt >> 8) & BQ2562X_VREG_MSB_MSK;
	volt[0] = chrg_volt & BQ2562X_VREG_LSB_MSK;

	return i2c_burst_write_dt(&config->i2c, BQ2562X_CHRG_V_LIM_LSB, volt, ARRAY_SIZE(volt));
}

/* Input Current Limit */
static int bq2562x_get_input_curr_lim(const struct device *dev, uint32_t *current_ua)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t ilim[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_INPUT_I_LIM_LSB, ilim, ARRAY_SIZE(ilim));
	if (ret) {
		return ret;
	}

	*current_ua = ((ilim[1] << 8) | ilim[0]) >> BQ2562X_IINDPM_I_SHIFT;
	*current_ua = *current_ua * BQ2562X_IINDPM_I_STEP_UA;

	return 0;
}

static int bq2562x_set_input_curr_lim(const struct device *dev, int iindpm)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t ilim[2] = {0};

	iindpm = CLAMP(iindpm, BQ2562X_IINDPM_I_MIN_UA, BQ2562X_IINDPM_I_MAX_UA);
	iindpm = (iindpm / BQ2562X_IINDPM_I_STEP_UA) << BQ2562X_IINDPM_I_SHIFT;

	ilim[0] = iindpm & BQ2562X_IINDPM_LSB_MSK;
	ilim[1] = (iindpm >> 8) & BQ2562X_IINDPM_MSB_MSK;

	return i2c_burst_write_dt(&config->i2c, BQ2562X_INPUT_I_LIM_LSB, ilim, ARRAY_SIZE(ilim));
}

/* Input Voltage Limit */
static int bq2562x_get_input_volt_lim(const struct device *dev, uint32_t *voltage_uv)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t vlim[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_INPUT_V_LIM_LSB, vlim, ARRAY_SIZE(vlim));
	if (ret) {
		return ret;
	}

	*voltage_uv = ((vlim[1] << 8) | vlim[0]) >> BQ2562X_VINDPM_V_SHIFT;
	*voltage_uv = *voltage_uv * BQ2562X_VINDPM_V_STEP_UV;

	return 0;
}

static int bq2562x_set_input_volt_lim(const struct device *dev, int vindpm)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t vlim[2] = {0};

	vindpm = CLAMP(vindpm, BQ2562X_VINDPM_V_MIN_UV, BQ2562X_VINDPM_V_MAX_UV);
	vindpm = (vindpm / BQ2562X_VINDPM_V_STEP_UV) << BQ2562X_VINDPM_V_SHIFT;

	vlim[1] = (vindpm >> 8) & BQ2562X_VINDPM_MSB_MSK;
	vlim[0] = vindpm & BQ2562X_VINDPM_LSB_MSK;

	return i2c_burst_write_dt(&config->i2c, BQ2562X_INPUT_V_LIM_LSB, vlim, ARRAY_SIZE(vlim));
}

/* Minimal System Voltage */
static int bq2562x_set_min_sys_volt(const struct device *dev, int vsysmin)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t vlim[2] = {0};

	vsysmin = CLAMP(vsysmin, BQ2562X_VSYSMIN_V_MIN_UV, BQ2562X_VSYSMIN_V_MAX_UV);
	vsysmin = (vsysmin / BQ2562X_VSYSMIN_V_SHIFT_UV) << BQ2562X_VSYSMIN_V_SHIFT;

	vlim[1] = (vsysmin >> 8) & BQ2562X_VSYSMIN_V_MSB_MSK;
	vlim[0] = vsysmin & BQ2562X_VSYSMIN_V_LSB_MSK;

	return i2c_burst_write_dt(&config->i2c, BQ2562X_MIN_SYS_V_LSB, vlim, ARRAY_SIZE(vlim));
}

/* Pre-charge Control */
static int bq2562x_get_prechrg_curr(const struct device *dev, uint32_t *current_ua)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t prechrg_curr[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_PRECHRG_CTRL_LSB, prechrg_curr,
				ARRAY_SIZE(prechrg_curr));
	if (ret) {
		return ret;
	}

	*current_ua = ((prechrg_curr[1] << 8) | prechrg_curr[0]) >> BQ2562X_PRECHRG_I_SHIFT;
	*current_ua = *current_ua * BQ2562X_PRECHRG_I_STEP_UA;

	return 0;
}

static int bq2562x_set_prechrg_curr(const struct device *dev, int pre_current)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t prechrg_curr[2] = {0};
	int ret;

	pre_current = CLAMP(pre_current, BQ2562X_PRECHRG_I_MIN_UA, BQ2562X_PRECHRG_I_MAX_UA);

	(void)bq2562x_set_charge_enable(dev, 0);

	pre_current = (pre_current / BQ2562X_PRECHRG_I_STEP_UA) << BQ2562X_PRECHRG_I_SHIFT;
	prechrg_curr[1] = (pre_current >> 8) & BQ2562X_PRECHRG_I_MSB_MSK;
	prechrg_curr[0] = pre_current & BQ2562X_PRECHRG_I_LSB_MSK;

	ret = i2c_burst_write_dt(&config->i2c, BQ2562X_PRECHRG_CTRL_LSB, prechrg_curr,
				 ARRAY_SIZE(prechrg_curr));

	(void)bq2562x_set_charge_enable(dev, 1);

	return ret;
}

/* Termination Control */
static int bq2562x_get_term_curr(const struct device *dev, uint32_t *current_ua)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t iterm[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_TERM_CTRL_LSB, iterm, ARRAY_SIZE(iterm));
	if (ret) {
		return ret;
	}

	*current_ua = ((iterm[1] << 8) | iterm[0]) >> BQ2562X_TERMCHRG_I_SHIFT;
	*current_ua = *current_ua * BQ2562X_TERMCHRG_I_STEP_UA;

	return 0;
}

static int bq2562x_set_term_curr(const struct device *dev, int term_current)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t iterm[2] = {0};

	term_current = CLAMP(term_current, BQ2562X_TERMCHRG_I_MIN_UA, BQ2562X_TERMCHRG_I_MAX_UA);
	term_current = (term_current / BQ2562X_TERMCHRG_I_STEP_UA) << BQ2562X_TERMCHRG_I_SHIFT;

	iterm[1] = (term_current >> 8) & BQ2562X_TERMCHRG_I_MSB_MSK;
	iterm[0] = term_current & BQ2562X_TERMCHRG_I_LSB_MSK;

	return i2c_burst_write_dt(&config->i2c, BQ2562X_TERM_CTRL_LSB, iterm, ARRAY_SIZE(iterm));
}

static int bq2562x_get_vbat_adc(const struct device *dev, uint32_t *vbat)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t vbat_adc[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_ADC_VBAT_LSB, vbat_adc, ARRAY_SIZE(vbat_adc));
	if (ret) {
		return ret;
	}

	*vbat = ((vbat_adc[1] << 8) | vbat_adc[0]) >> BQ2562X_ADC_VBAT_SHIFT;
	*vbat = *vbat * BQ2562X_ADC_VBAT_STEP_UV;

	return 0;
}

static int bq2562x_get_vbus_adc(const struct device *dev, uint32_t *volt)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t vbus_adc[2] = {0};
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_ADC_VBUS_LSB, vbus_adc, ARRAY_SIZE(vbus_adc));
	if (ret) {
		return ret;
	}

	*volt = ((vbus_adc[1] << 8) | vbus_adc[0]) >> BQ2562X_ADC_VBUS_SHIFT;
	*volt = *volt * BQ2562X_ADC_VBUS_STEP_UV;

	return 0;
}

static int bq2562x_get_ibat_adc(const struct device *dev, int32_t *ibat)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t ibat_adc[2] = {0};
	uint16_t temp;
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_ADC_IBAT_LSB, ibat_adc, ARRAY_SIZE(ibat_adc));
	if (ret) {
		return ret;
	}

	temp = sys_get_le16(ibat_adc);
	if (temp & BIT(15)) {
		temp = ~temp + 1;
		*ibat = (temp >> BQ2562X_ADC_IBAT_SHIFT) * BQ2562X_ADC_IBAT_STEP_UV * -1;
	} else {
		*ibat = (temp >> BQ2562X_ADC_IBAT_SHIFT) * BQ2562X_ADC_IBAT_STEP_UV;
	}

	return 0;
}

static int bq2562x_get_ibus_adc(const struct device *dev, int32_t *ibus)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t ibus_adc[2] = {0};
	uint16_t temp;
	int ret;

	ret = i2c_burst_read_dt(&config->i2c, BQ2562X_ADC_IBUS_LSB, ibus_adc, ARRAY_SIZE(ibus_adc));
	if (ret) {
		return ret;
	}

	temp = sys_get_le16(ibus_adc);
	if (temp & BIT(15)) {
		temp = ~temp + 1;
		*ibus = (temp >> BQ2562X_ADC_IBUS_SHIFT) * BQ2562X_ADC_CURR_STEP_UA * -1;
	} else {
		*ibus = (temp >> BQ2562X_ADC_IBUS_SHIFT) * BQ2562X_ADC_CURR_STEP_UA;
	}

	return 0;
}

static int bq2562x_get_online_status(const struct device *dev, enum charger_online *online)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_stat_1;
	int online_status;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_STAT_1, &chrg_stat_1);
	if (ret) {
		return ret;
	}

	online_status = chrg_stat_1 & BQ2562X_VBUS_STAT_MSK;
	if (!online_status || (online_status == BQ2562X_OTG_MODE)) {
		*online = CHARGER_ONLINE_OFFLINE;
	} else {
		*online = CHARGER_ONLINE_FIXED;
	}

	return 0;
}

static int bq2562x_get_health(const struct device *dev, enum charger_health *health)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t fault;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_FAULT_STAT_0, &fault);
	if (ret) {
		return ret;
	}

	*health = CHARGER_HEALTH_UNKNOWN;
	switch (fault & BQ2562X_TEMP_MASK) {
	case BQ2562X_TEMP_TS_NORMAL:
		*health = CHARGER_HEALTH_GOOD;
		break;
	case BQ2562X_TEMP_COLD:
		*health = CHARGER_HEALTH_COLD;
		break;
	case BQ2562X_TEMP_HOT:
		*health = CHARGER_HEALTH_HOT;
		break;
	case BQ2562X_TEMP_COOL:
		__fallthrough;
	case BQ2562X_TEMP_PRECOOL:
		*health = CHARGER_HEALTH_COOL;
		break;
	case BQ2562X_TEMP_WARM:
		__fallthrough;
	case BQ2562X_TEMP_PREWARM:
		*health = CHARGER_HEALTH_WARM;
		break;
	case BQ2562X_TEMP_PIN_BIAS_REF_FAULT:
		*health = CHARGER_HEALTH_DEAD;
		break;
	}

	if (fault & BQ2562X_TSHUT_STAT) {
		*health = CHARGER_HEALTH_OVERHEAT;
	} else if (fault & (BQ2562X_OTG_FAULT_STAT | BQ2562X_SYS_FAULT_STAT |
			    BQ2562X_BAT_FAULT_STAT | BQ2562X_VBUS_FAULT_STAT)) {
		*health = CHARGER_HEALTH_OVERVOLTAGE;
	}

	return 0;
}

static int bq2562x_get_charger_type(const struct device *dev, enum charger_charge_type *type)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_stat_1, chrg_ctl;
	int32_t ibat, itrickle_max;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_STAT_1, &chrg_stat_1);
	if (ret) {
		return ret;
	}

	if (bq2562x_get_charge_enable(dev)) {
		chrg_stat_1 = FIELD_GET(BQ2562X_CHG_STAT_MSK, chrg_stat_1);
		switch (chrg_stat_1) {
		case BQ2562X_NOT_CHRGING:
			*type = CHARGER_CHARGE_TYPE_NONE;
			break;
		case BQ2562X_TAPER_CHRG:
			*type = CHARGER_CHARGE_TYPE_STANDARD;
			break;
		case BQ2562X_TOP_OFF_CHRG:
			__fallthrough;
		case BQ2562X_TRICKLE_CHRG:
			*type = CHARGER_CHARGE_TYPE_TRICKLE;
			ret = bq2562x_get_ibat_adc(dev, &ibat);
			if (ret) {
				break;
			}

			ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_0, &chrg_ctl);
			if (ret) {
				break;
			}

			if (chrg_ctl & BQ2562X_CHG_CTL_ITRICKLE) {
				itrickle_max = BQ2562X_CHG_CTL_ITRICKLE_MAX_UA;
			} else {
				itrickle_max = BQ2562X_CHG_CTL_ITRICKLE_DEF_UA;
			}

			if (ibat > itrickle_max) {
				if (ibat > BQ2562X_PRECHG_MAX_UA) {
					*type = CHARGER_CHARGE_TYPE_FAST;
				} else {
					*type = CHARGER_CHARGE_TYPE_STANDARD;
				}
			}
			break;
		}
	} else {
		*type = CHARGER_CHARGE_TYPE_UNKNOWN;
	}

	return 0;
}

static int bq2562x_get_charger_status(const struct device *dev, enum charger_status *charge_status)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_stat_1;
	uint8_t type, status;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_STAT_1, &chrg_stat_1);
	if (ret) {
		return ret;
	}

	if (bq2562x_get_charge_enable(dev)) {
		status = FIELD_GET(BQ2562X_CHG_STAT_MSK, chrg_stat_1);
	} else {
		status = BQ2562X_NOT_CHRGING;
	}

	type = FIELD_GET(BQ2562X_VBUS_STAT_MSK, chrg_stat_1);

	if (!type || (type == BQ2562X_OTG_MODE)) {
		*charge_status = CHARGER_STATUS_DISCHARGING;
	} else if (!status) {
		*charge_status = CHARGER_STATUS_NOT_CHARGING;
	} else {
		*charge_status = CHARGER_STATUS_CHARGING;
	}

	return 0;
}

static int bq2562x_get_usb_type(const struct device *dev, enum charger_usb_type *type)
{
	const struct bq2562x_config *const config = dev->config;
	uint8_t chrg_stat_1;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_CHRG_STAT_1, &chrg_stat_1);
	if (ret) {
		return ret;
	}

	chrg_stat_1 = FIELD_GET(BQ2562X_VBUS_STAT_MSK, chrg_stat_1);
	switch (chrg_stat_1) {
	case BQ2562X_USB_SDP:
		*type = CHARGER_USB_TYPE_SDP;
		break;
	case BQ2562X_USB_CDP:
		*type = CHARGER_USB_TYPE_CDP;
		break;
	case BQ2562X_USB_DCP:
		*type = CHARGER_USB_TYPE_DCP;
		break;
	case BQ2562X_OTG_MODE:
		*type = CHARGER_USB_TYPE_ACA;
		break;
	case BQ2562X_UNKNOWN_500MA:
		__fallthrough;
	case BQ2562X_NON_STANDARD:
		__fallthrough;
	case BQ2562X_HVDCP: /* TODO */
		__fallthrough;
	default:
		*type = CHARGER_USB_TYPE_UNKNOWN;
	}

	return 0;
}

static int bq2562x_get_prop(const struct device *dev, charger_prop_t prop,
			    union charger_propval *val)
{
	switch (prop) {
	case CHARGER_PROP_ONLINE:
		return bq2562x_get_online_status(dev, &val->online);
	case CHARGER_PROP_CHARGE_TYPE:
		return bq2562x_get_charger_type(dev, &val->charge_type);
	case CHARGER_PROP_HEALTH:
		return bq2562x_get_health(dev, &val->health);
	case CHARGER_PROP_STATUS:
		return bq2562x_get_charger_status(dev, &val->status);
	case CHARGER_PROP_USB_TYPE:
		return bq2562x_get_usb_type(dev, &val->usb_type);
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2562x_get_ichrg_curr(dev, &val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2562x_get_chrg_volt(dev, &val->const_charge_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2562x_get_prechrg_curr(dev, &val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2562x_get_term_curr(dev, &val->charge_term_current_ua);
	case CHARGER_PROP_BATTERY_VOLTAGE_NOW:
		return bq2562x_get_vbat_adc(dev, &val->battery_voltage_now_uv);
	case CHARGER_PROP_BATTERY_CURRENT_NOW:
		return bq2562x_get_ibat_adc(dev, &val->battery_current_now_ua);
	case CHARGER_PROP_INPUT_VOLTAGE_NOW:
		return bq2562x_get_vbus_adc(dev, &val->input_voltage_now_uv);
	case CHARGER_PROP_INPUT_CURRENT_NOW:
		return bq2562x_get_ibus_adc(dev, &val->input_current_now_ua);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2562x_get_input_curr_lim(dev, &val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2562x_get_input_volt_lim(dev, &val->input_voltage_regulation_voltage_uv);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq2562x_set_prop(const struct device *dev, charger_prop_t prop,
			    const union charger_propval *val)
{
	struct bq2562x_data *data = dev->data;

	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
		return bq2562x_set_ichrg_curr(dev, val->const_charge_current_ua);
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		return bq2562x_set_chrg_volt(dev, val->const_charge_voltage_uv);
	case CHARGER_PROP_PRECHARGE_CURRENT_UA:
		return bq2562x_set_prechrg_curr(dev, val->precharge_current_ua);
	case CHARGER_PROP_CHARGE_TERM_CURRENT_UA:
		return bq2562x_set_term_curr(dev, val->charge_term_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA:
		return bq2562x_set_input_curr_lim(dev, val->input_current_regulation_current_ua);
	case CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV:
		return bq2562x_set_input_volt_lim(dev, val->input_voltage_regulation_voltage_uv);
	case CHARGER_PROP_STATUS_NOTIFICATION:
		data->charger_status_notifier = val->status_notification;
		break;
	case CHARGER_PROP_ONLINE_NOTIFICATION:
		data->charger_online_notifier = val->online_notification;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2562x_validate_dt(struct bq2562x_data *data)
{
	if (!IN_RANGE(data->min_sys_voltage_uv, BQ2562X_VSYSMIN_V_MIN_UV,
		      BQ2562X_VSYSMIN_V_MAX_UV)) {
		data->min_sys_voltage_uv = BQ2562X_VSYSMIN_V_DEF_UV;
	}

	if (!IN_RANGE(data->input_voltage_min_uv, BQ2562X_VINDPM_V_MIN_UV,
		      BQ2562X_VINDPM_V_MAX_UV)) {
		data->input_voltage_min_uv = BQ2562X_VINDPM_V_DEF_UV;
	}

	if (!IN_RANGE(data->input_current_max_ua, BQ2562X_IINDPM_I_MIN_UA,
		      BQ2562X_IINDPM_I_MAX_UA)) {
		data->input_current_max_ua = BQ2562X_IINDPM_I_DEF_UA;
	}

	return 0;
}

static int bq2562x_set_heat_mgmt(const struct device *dev)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_2, BQ2562X_CTRL2_SET_CONV_STRN,
				     data->switching_converter_strength << 2);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_2, BQ2562X_CTRL2_SET_CONV_FREQ,
				     data->switching_converter_freq << 4);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_2, BQ2562X_CTRL2_TREG,
				     data->thermal_regulation_threshold << 6);
	if (ret) {
		return ret;
	}

	return bq2562x_set_min_sys_volt(dev, data->min_sys_voltage_uv);
}

static int bq2562x_hw_init(const struct device *dev)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_2, BQ2562X_CTRL2_REG_RST,
				     BQ2562X_CTRL2_REG_RST);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_NTC_CTRL_0, BQ2562X_NTC_MASK,
				     BQ2562X_NTC_MASK);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_CTRL_1, BQ2562X_WATCHDOG_MASK,
				     BQ2562X_WATCHDOG_DIS);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_ichrg_curr(dev, data->constant_charge_current_max_ua);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_chrg_volt(dev, data->constant_charge_voltage_max_uv);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_prechrg_curr(dev, data->precharge_current_ua);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_term_curr(dev, data->charge_term_current_ua);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_input_volt_lim(dev, data->input_voltage_min_uv);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_input_curr_lim(dev, data->input_current_max_ua);
	if (ret) {
		return ret;
	}

	ret = bq2562x_set_heat_mgmt(dev);
	if (ret) {
		return ret;
	}

	/* ADC 12 bit resolution */
	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_ADC_CTRL, BQ2562X_ADC_SAMPLE, 0);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_ADC_CTRL, BQ2562X_ADC_EN,
				     BQ2562X_ADC_EN);
	if (ret) {
		return ret;
	}

	return 0;
}

static int bq2562x_enable_interrupt_pin(const struct device *dev, bool enabled)
{
	const struct bq2562x_config *const config = dev->config;
	gpio_flags_t flags;
	int ret;

	flags = enabled ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
	if (ret < 0) {
		LOG_ERR("Could not %s interrupt GPIO callback: %d", enabled ? "enable" : "disable",
			ret);
	}

	return ret;
}

static void bq2562x_int_routine_work_handler(struct k_work *work)
{
	struct bq2562x_data *data = CONTAINER_OF(work, struct bq2562x_data, int_routine_work);
	union charger_propval val;
	int ret;

	if (data->charger_status_notifier != NULL) {
		ret = bq2562x_get_charger_status(data->dev, &val.status);
		if (!ret) {
			data->charger_status_notifier(val.status);
		}
	}

	if (data->charger_online_notifier != NULL) {
		ret = bq2562x_get_online_status(data->dev, &val.online);
		if (!ret) {
			data->charger_online_notifier(val.online);
		}
	}
	(void)bq2562x_enable_interrupt_pin(data->dev, true);
}

static void bq2562x_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct bq2562x_data *data = CONTAINER_OF(cb, struct bq2562x_data, gpio_cb);
	int ret;

	(void)bq2562x_enable_interrupt_pin(data->dev, false);

	ret = k_work_submit(&data->int_routine_work);
	if (ret < 0) {
		LOG_WRN("Could not submit int work: %d", ret);
	}
}

static int bq2562x_configure_interrupt(const struct device *dev)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	int ret;

	k_work_init(&data->int_routine_work, bq2562x_int_routine_work_handler);
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, bq2562x_gpio_callback, BIT(config->int_gpio.pin));
	ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not add interrupt GPIO callback");
		return ret;
	}

	/* enable status and online interrupt */
	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_MSK_0, BQ2562X_CHG_MSK_0_CLR,
				     BQ2562X_CHG_MSK_0_CLR);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_FAULT_MSK_0, BQ2562X_FAULT_MSK_0_CLR,
				     BQ2562X_FAULT_MSK_0_CLR);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_MSK_1, BQ2562X_CHG_MSK, 0);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, BQ2562X_CHRG_MSK_1, BQ2562X_VBUS_MSK, 0);
	if (ret) {
		return ret;
	}
	(void)bq2562x_enable_interrupt_pin(data->dev, true);

	return 0;
}

static int bq2562x_init(const struct device *dev)
{
	const struct bq2562x_config *const config = dev->config;
	struct bq2562x_data *data = dev->data;
	uint8_t val;
	int ret;

	data->dev = dev;
	ret = i2c_reg_read_byte_dt(&config->i2c, BQ2562X_PART_INFO, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(BQ2562X_PART_NO_MASK, val);
	if (val == BQ25622) {
		return -ENOTSUP;
	}

	/* charge enable */
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

	/* DT sanity */
	ret = bq2562x_validate_dt(data);
	if (ret) {
		return ret;
	}

	ret = bq2562x_hw_init(dev);
	if (ret) {
		return ret;
	}

	if (config->int_gpio.port != NULL) {
		ret = bq2562x_configure_interrupt(dev);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static DEVICE_API(charger, bq2562x_driver_api) = {
	.get_property = bq2562x_get_prop,
	.set_property = bq2562x_set_prop,
	.charge_enable = bq2562x_set_charge_enable,
};

#define BQ2562X_INIT(inst)                                                                         \
                                                                                                   \
	static const struct bq2562x_config bq2562x_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.ce_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ce_gpios, {}),                           \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {}),                         \
	};                                                                                         \
                                                                                                   \
	static struct bq2562x_data bq2562x_data_##inst = {                                         \
		.constant_charge_current_max_ua =                                                  \
			DT_INST_PROP(inst, constant_charge_current_max_microamp),                  \
		.constant_charge_voltage_max_uv =                                                  \
			DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),                 \
		.precharge_current_ua = DT_INST_PROP(inst, precharge_current_microamp),            \
		.charge_term_current_ua = DT_INST_PROP(inst, charge_term_current_microamp),        \
		.min_sys_voltage_uv = DT_INST_PROP(inst, ti_min_sys_voltage_microvolt),            \
		.input_voltage_min_uv = DT_INST_PROP(inst, ti_input_voltage_limit_microvolt),      \
		.input_current_max_ua = DT_INST_PROP(inst, ti_input_current_limit_microamp),       \
		.thermal_regulation_threshold =                                                    \
			DT_INST_PROP(inst, ti_thermal_regulation_threshold),                       \
		.switching_converter_freq = DT_INST_PROP(inst, ti_switching_converter_freq),       \
		.switching_converter_strength =                                                    \
			DT_INST_PROP(inst, ti_switching_converter_strength),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, bq2562x_init, NULL, &bq2562x_data_##inst,                      \
			      &bq2562x_config_##inst, POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY,   \
			      &bq2562x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ2562X_INIT)
