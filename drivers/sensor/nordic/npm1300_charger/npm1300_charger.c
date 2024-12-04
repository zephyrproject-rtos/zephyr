/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_charger

#include <math.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>

struct npm1300_charger_config {
	const struct device *mfd;
	int32_t term_microvolt;
	int32_t term_warm_microvolt;
	int32_t current_microamp;
	int32_t dischg_limit_microamp;
	int32_t vbus_limit_microamp;
	int32_t temp_thresholds[4U];
	int32_t dietemp_thresholds[2U];
	uint32_t thermistor_ohms;
	uint16_t thermistor_beta;
	uint8_t thermistor_idx;
	uint8_t trickle_sel;
	uint8_t iterm_sel;
	bool charging_enable;
	bool vbatlow_charge_enable;
	bool disable_recharge;
};

struct npm1300_charger_data {
	uint16_t voltage;
	uint16_t current;
	uint16_t temp;
	uint16_t dietemp;
	uint8_t status;
	uint8_t error;
	uint8_t ibat_stat;
	uint8_t vbus_stat;
};

/* nPM1300 base addresses */
#define CHGR_BASE 0x03U
#define ADC_BASE  0x05U
#define VBUS_BASE 0x02U

/* nPM1300 charger register offsets */
#define CHGR_OFFSET_ERR_CLR     0x00U
#define CHGR_OFFSET_EN_SET      0x04U
#define CHGR_OFFSET_EN_CLR      0x05U
#define CHGR_OFFSET_DIS_SET     0x06U
#define CHGR_OFFSET_ISET        0x08U
#define CHGR_OFFSET_ISET_DISCHG 0x0AU
#define CHGR_OFFSET_VTERM       0x0CU
#define CHGR_OFFSET_VTERM_R     0x0DU
#define CHGR_OFFSET_TRICKLE_SEL 0x0EU
#define CHGR_OFFSET_ITERM_SEL   0x0FU
#define CHGR_OFFSET_NTC_TEMPS   0x10U
#define CHGR_OFFSET_DIE_TEMPS   0x18U
#define CHGR_OFFSET_CHG_STAT    0x34U
#define CHGR_OFFSET_ERR_REASON  0x36U
#define CHGR_OFFSET_VBATLOW_EN  0x50U

/* nPM1300 ADC register offsets */
#define ADC_OFFSET_TASK_VBAT 0x00U
#define ADC_OFFSET_TASK_TEMP 0x01U
#define ADC_OFFSET_TASK_DIE  0x02U
#define ADC_OFFSET_CONFIG    0x09U
#define ADC_OFFSET_NTCR_SEL  0x0AU
#define ADC_OFFSET_TASK_AUTO 0x0CU
#define ADC_OFFSET_RESULTS   0x10U
#define ADC_OFFSET_IBAT_EN   0x24U

/* nPM1300 VBUS register offsets */
#define VBUS_OFFSET_ILIMUPDATE  0x00U
#define VBUS_OFFSET_ILIM        0x01U
#define VBUS_OFFSET_ILIMSTARTUP 0x02U
#define VBUS_OFFSET_DETECT      0x05U
#define VBUS_OFFSET_STATUS      0x07U

/* Ibat status */
#define IBAT_STAT_DISCHARGE      0x04U
#define IBAT_STAT_CHARGE_TRICKLE 0x0CU
#define IBAT_STAT_CHARGE_COOL    0x0DU
#define IBAT_STAT_CHARGE_NORMAL  0x0FU

struct adc_results_t {
	uint8_t ibat_stat;
	uint8_t msb_vbat;
	uint8_t msb_ntc;
	uint8_t msb_die;
	uint8_t msb_vsys;
	uint8_t lsb_a;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t msb_ibat;
	uint8_t msb_vbus;
	uint8_t lsb_b;
} __packed;

/* ADC result masks */
#define ADC_MSB_SHIFT      2U
#define ADC_LSB_MASK       0x03U
#define ADC_LSB_VBAT_SHIFT 0U
#define ADC_LSB_NTC_SHIFT  2U
#define ADC_LSB_DIE_SHIFT  4U
#define ADC_LSB_IBAT_SHIFT 4U

/* NTC temp masks */
#define NTCTEMP_MSB_SHIFT 2U
#define NTCTEMP_LSB_MASK  0x03U

/* dietemp masks */
#define DIETEMP_MSB_SHIFT 2U
#define DIETEMP_LSB_MASK  0x03U

/* VBUS detect masks */
#define DETECT_HI_MASK    0x0AU
#define DETECT_HI_CURRENT 1500000
#define DETECT_LO_CURRENT 500000

/* VBUS status masks */
#define STATUS_PRESENT_MASK      0x01U
#define STATUS_CUR_LIMIT_MASK    0x02U
#define STATUS_OVERVLT_PROT_MASK 0x04U
#define STATUS_UNDERVLT_MASK     0x08U
#define STATUS_SUSPENDED_MASK    0x10U
#define STATUS_BUSOUT_MASK       0x20U

/* Dietemp calculation constants */
#define DIETEMP_OFFSET_MDEGC 394670
#define DIETEMP_FACTOR_MUL   3963000
#define DIETEMP_FACTOR_DIV   5000

/* Linear range for charger terminal voltage */
static const struct linear_range charger_volt_ranges[] = {
	LINEAR_RANGE_INIT(3500000, 50000, 0U, 3U), LINEAR_RANGE_INIT(4000000, 50000, 4U, 13U)};

/* Linear range for charger current */
static const struct linear_range charger_current_range = LINEAR_RANGE_INIT(32000, 2000, 16U, 400U);

/* Linear range for Discharge limit */
static const struct linear_range discharge_limit_range = LINEAR_RANGE_INIT(268090, 3230, 83U, 415U);

/* Linear range for vbusin current limit */
static const struct linear_range vbus_current_ranges[] = {
	LINEAR_RANGE_INIT(100000, 0, 1U, 1U), LINEAR_RANGE_INIT(500000, 100000, 5U, 15U)};

static void calc_temp(const struct npm1300_charger_config *const config, uint16_t code,
		      struct sensor_value *valp)
{
	/* Ref: Datasheet Figure 42: Battery temperature (Kelvin) */
	float log_result = log((1024.f / (float)code) - 1);
	float inv_temp_k = (1.f / 298.15f) - (log_result / (float)config->thermistor_beta);

	float temp = (1.f / inv_temp_k) - 273.15f;

	valp->val1 = (int32_t)temp;
	valp->val2 = (int32_t)(fmodf(temp, 1.f) * 1000000.f);
}

static void calc_dietemp(const struct npm1300_charger_config *const config, uint16_t code,
			 struct sensor_value *valp)
{
	/* Ref: Datasheet Figure 36: Die temperature (Celcius) */
	int32_t temp =
		DIETEMP_OFFSET_MDEGC - (((int32_t)code * DIETEMP_FACTOR_MUL) / DIETEMP_FACTOR_DIV);

	valp->val1 = temp / 1000;
	valp->val2 = (temp % 1000) * 1000;
}

static uint32_t calc_ntc_res(const struct npm1300_charger_config *const config, int32_t temp_mdegc)
{
	float inv_t0 = 1.f / 298.15f;
	float temp = (float)temp_mdegc / 1000.f;

	float inv_temp_k = 1.f / (temp + 273.15f);

	return config->thermistor_ohms *
	       exp((float)config->thermistor_beta * (inv_temp_k - inv_t0));
}

static uint16_t adc_get_res(uint8_t msb, uint8_t lsb, uint16_t lsb_shift)
{
	return ((uint16_t)msb << ADC_MSB_SHIFT) | ((lsb >> lsb_shift) & ADC_LSB_MASK);
}

static void calc_current(const struct npm1300_charger_config *const config,
			 struct npm1300_charger_data *const data, struct sensor_value *valp)
{
	int32_t full_scale_ma;
	int32_t current;

	switch (data->ibat_stat) {
	case IBAT_STAT_DISCHARGE:
		full_scale_ma = config->dischg_limit_microamp / 1000;
		break;
	case IBAT_STAT_CHARGE_TRICKLE:
	/* Fallthrough */
	case IBAT_STAT_CHARGE_COOL:
	/* Fallthrough */
	case IBAT_STAT_CHARGE_NORMAL:
		full_scale_ma = -config->current_microamp / 800;
		break;
	default:
		full_scale_ma = 0;
		break;
	}

	current = (data->current * full_scale_ma) / 1024;

	valp->val1 = current / 1000;
	valp->val2 = (current % 1000) * 1000;
}

int npm1300_charger_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *valp)
{
	const struct npm1300_charger_config *const config = dev->config;
	struct npm1300_charger_data *const data = dev->data;
	int32_t tmp;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		tmp = data->voltage * 5000 / 1024;
		valp->val1 = tmp / 1000;
		valp->val2 = (tmp % 1000) * 1000;
		break;
	case SENSOR_CHAN_GAUGE_TEMP:
		if (config->thermistor_idx == 0) {
			return -ENOTSUP;
		}
		calc_temp(config, data->temp, valp);
		break;
	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		calc_current(config, data, valp);
		break;
	case SENSOR_CHAN_NPM1300_CHARGER_STATUS:
		valp->val1 = data->status;
		valp->val2 = 0;
		break;
	case SENSOR_CHAN_NPM1300_CHARGER_ERROR:
		valp->val1 = data->error;
		valp->val2 = 0;
		break;
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		valp->val1 = config->current_microamp / 1000000;
		valp->val2 = config->current_microamp % 1000000;
		break;
	case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
		valp->val1 = config->dischg_limit_microamp / 1000000;
		valp->val2 = config->dischg_limit_microamp % 1000000;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		calc_dietemp(config, data->dietemp, valp);
		break;
	case SENSOR_CHAN_NPM1300_CHARGER_VBUS_STATUS:
		valp->val1 = data->vbus_stat;
		valp->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int npm1300_charger_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct npm1300_charger_config *const config = dev->config;
	struct npm1300_charger_data *data = dev->data;
	struct adc_results_t results;
	int ret;

	/* Read charge status and error reason */
	ret = mfd_npm1300_reg_read(config->mfd, CHGR_BASE, CHGR_OFFSET_CHG_STAT, &data->status);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_npm1300_reg_read(config->mfd, CHGR_BASE, CHGR_OFFSET_ERR_REASON, &data->error);
	if (ret != 0) {
		return ret;
	}

	/* Read adc results */
	ret = mfd_npm1300_reg_read_burst(config->mfd, ADC_BASE, ADC_OFFSET_RESULTS, &results,
					 sizeof(results));
	if (ret != 0) {
		return ret;
	}

	data->voltage = adc_get_res(results.msb_vbat, results.lsb_a, ADC_LSB_VBAT_SHIFT);
	data->temp = adc_get_res(results.msb_ntc, results.lsb_a, ADC_LSB_NTC_SHIFT);
	data->dietemp = adc_get_res(results.msb_die, results.lsb_a, ADC_LSB_DIE_SHIFT);
	data->current = adc_get_res(results.msb_ibat, results.lsb_b, ADC_LSB_IBAT_SHIFT);
	data->ibat_stat = results.ibat_stat;

	/* Trigger ntc and die temperature measurements */
	ret = mfd_npm1300_reg_write2(config->mfd, ADC_BASE, ADC_OFFSET_TASK_TEMP, 1U, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger current and voltage measurement */
	ret = mfd_npm1300_reg_write(config->mfd, ADC_BASE, ADC_OFFSET_TASK_VBAT, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Read vbus status */
	ret = mfd_npm1300_reg_read(config->mfd, VBUS_BASE, VBUS_OFFSET_STATUS, &data->vbus_stat);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static int set_ntc_thresholds(const struct npm1300_charger_config *const config)
{
	for (uint8_t idx = 0U; idx < 4U; idx++) {
		if (config->temp_thresholds[idx] < INT32_MAX) {
			uint32_t res = calc_ntc_res(config, config->temp_thresholds[idx]);

			/* Ref: Datasheet Figure 14: Equation for battery temperature */
			uint16_t code = (1024 * res) / (res + config->thermistor_ohms);

			int ret = mfd_npm1300_reg_write2(
				config->mfd, CHGR_BASE, CHGR_OFFSET_NTC_TEMPS + (idx * 2U),
				code >> NTCTEMP_MSB_SHIFT, code & NTCTEMP_LSB_MASK);

			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int set_dietemp_thresholds(const struct npm1300_charger_config *const config)
{
	for (uint8_t idx = 0U; idx < 2U; idx++) {
		if (config->dietemp_thresholds[idx] < INT32_MAX) {
			/* Ref: Datasheet section 6.2.6: Charger thermal regulation */
			int32_t numerator =
				(DIETEMP_OFFSET_MDEGC - config->dietemp_thresholds[idx]) *
				DIETEMP_FACTOR_DIV;
			uint16_t code = DIV_ROUND_CLOSEST(numerator, DIETEMP_FACTOR_MUL);

			int ret = mfd_npm1300_reg_write2(
				config->mfd, CHGR_BASE, CHGR_OFFSET_DIE_TEMPS + (idx * 2U),
				code >> DIETEMP_MSB_SHIFT, code & DIETEMP_LSB_MASK);

			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int npm1300_charger_attr_get(const struct device *dev, enum sensor_channel chan,
				    enum sensor_attribute attr, struct sensor_value *val)
{
	const struct npm1300_charger_config *const config = dev->config;
	uint8_t data;
	int ret;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		if (attr != SENSOR_ATTR_CONFIGURATION) {
			return -ENOTSUP;
		}

		ret = mfd_npm1300_reg_read(config->mfd, CHGR_BASE, CHGR_OFFSET_EN_SET, &data);
		if (ret == 0) {
			val->val1 = data;
			val->val2 = 0U;
		}
		return ret;

	case SENSOR_CHAN_CURRENT:
		if (attr != SENSOR_ATTR_UPPER_THRESH) {
			return -ENOTSUP;
		}

		ret = mfd_npm1300_reg_read(config->mfd, VBUS_BASE, VBUS_OFFSET_DETECT, &data);
		if (ret < 0) {
			return ret;
		}

		if (data == 0U) {
			/* No charger connected */
			val->val1 = 0;
			val->val2 = 0;
		} else if ((data & DETECT_HI_MASK) != 0U) {
			/* CC1 or CC2 indicate 1.5A or 3A capability */
			val->val1 = DETECT_HI_CURRENT / 1000000;
			val->val2 = DETECT_HI_CURRENT % 1000000;
		} else {
			val->val1 = DETECT_LO_CURRENT / 1000000;
			val->val2 = DETECT_LO_CURRENT % 1000000;
		}

		return 0;

	case SENSOR_CHAN_NPM1300_CHARGER_VBUS_STATUS:
		ret = mfd_npm1300_reg_read(config->mfd, VBUS_BASE, VBUS_OFFSET_STATUS, &data);
		if (ret < 0) {
			return ret;
		}

		switch ((enum sensor_attribute_npm1300_charger)attr) {
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_PRESENT:
			val->val1 = (data & STATUS_PRESENT_MASK) != 0;
			break;
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_CUR_LIMIT:
			val->val1 = (data & STATUS_CUR_LIMIT_MASK) != 0;
			break;
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_OVERVLT_PROT:
			val->val1 = (data & STATUS_OVERVLT_PROT_MASK) != 0;
			break;
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_UNDERVLT:
			val->val1 = (data & STATUS_UNDERVLT_MASK) != 0;
			break;
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_SUSPENDED:
			val->val1 = (data & STATUS_SUSPENDED_MASK) != 0;
			break;
		case SENSOR_ATTR_NPM1300_CHARGER_VBUS_BUSOUT:
			val->val1 = (data & STATUS_BUSOUT_MASK) != 0;
			break;
		default:
			return -ENOTSUP;
		}
		val->val2 = 0;
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int npm1300_charger_attr_set(const struct device *dev, enum sensor_channel chan,
				    enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct npm1300_charger_config *const config = dev->config;
	int ret;

	if (attr != SENSOR_ATTR_CONFIGURATION) {
		return -ENOTSUP;
	}

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT:
		if (val->val1 == 0) {
			/* Disable charging */
			return mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_EN_CLR,
						     1U);
		}

		/* Clear any errors and enable charging */
		ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_ERR_CLR, 1U);
		if (ret != 0) {
			return ret;
		}
		return mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_EN_SET, 1U);

	case SENSOR_CHAN_CURRENT:
		/* Set vbus current limit */
		int32_t current = (val->val1 * 1000000) + val->val2;
		uint16_t idx;

		ret = linear_range_group_get_win_index(vbus_current_ranges,
						       ARRAY_SIZE(vbus_current_ranges), current,
						       current, &idx);

		if (ret == -EINVAL) {
			return ret;
		}

		ret = mfd_npm1300_reg_write(config->mfd, VBUS_BASE, VBUS_OFFSET_ILIM, idx);
		if (ret != 0) {
			return ret;
		}

		/* Switch to new current limit, this will be reset automatically on USB removal */
		return mfd_npm1300_reg_write(config->mfd, VBUS_BASE, VBUS_OFFSET_ILIMUPDATE, 1U);

	default:
		return -ENOTSUP;
	}
}

int npm1300_charger_init(const struct device *dev)
{
	const struct npm1300_charger_config *const config = dev->config;
	uint16_t idx;
	uint8_t byte = 0U;
	int ret;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	/* Configure temperature thresholds */
	ret = mfd_npm1300_reg_write(config->mfd, ADC_BASE, ADC_OFFSET_NTCR_SEL,
				    config->thermistor_idx);
	if (ret != 0) {
		return ret;
	}

	ret = set_ntc_thresholds(config);
	if (ret != 0) {
		return ret;
	}

	ret = set_dietemp_thresholds(config);
	if (ret != 0) {
		return ret;
	}

	/* Configure termination voltages */
	ret = linear_range_group_get_win_index(charger_volt_ranges, ARRAY_SIZE(charger_volt_ranges),
					       config->term_microvolt, config->term_microvolt,
					       &idx);
	if (ret == -EINVAL) {
		return ret;
	}
	ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_VTERM, idx);
	if (ret != 0) {
		return ret;
	}

	ret = linear_range_group_get_win_index(charger_volt_ranges, ARRAY_SIZE(charger_volt_ranges),
					       config->term_warm_microvolt,
					       config->term_warm_microvolt, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_VTERM_R, idx);
	if (ret != 0) {
		return ret;
	}

	/* Set current, allow rounding down to closest value */
	ret = linear_range_get_win_index(&charger_current_range,
					 config->current_microamp - charger_current_range.step,
					 config->current_microamp, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	ret = mfd_npm1300_reg_write2(config->mfd, CHGR_BASE, CHGR_OFFSET_ISET, idx / 2U, idx & 1U);
	if (ret != 0) {
		return ret;
	}

	/* Set discharge limit, allow rounding down to closest value */
	ret = linear_range_get_win_index(&discharge_limit_range,
					 config->dischg_limit_microamp - discharge_limit_range.step,
					 config->dischg_limit_microamp, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	ret = mfd_npm1300_reg_write2(config->mfd, CHGR_BASE, CHGR_OFFSET_ISET_DISCHG, idx / 2U,
				     idx & 1U);
	if (ret != 0) {
		return ret;
	}

	/* Configure vbus current limit */
	ret = linear_range_group_get_win_index(vbus_current_ranges, ARRAY_SIZE(vbus_current_ranges),
					       config->vbus_limit_microamp,
					       config->vbus_limit_microamp, &idx);
	if (ret == -EINVAL) {
		return ret;
	}
	ret = mfd_npm1300_reg_write(config->mfd, VBUS_BASE, VBUS_OFFSET_ILIMSTARTUP, idx);
	if (ret != 0) {
		return ret;
	}

	/* Configure trickle voltage threshold */
	ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_TRICKLE_SEL,
				    config->trickle_sel);
	if (ret != 0) {
		return ret;
	}

	/* Configure termination current */
	ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_ITERM_SEL,
				    config->iterm_sel);
	if (ret != 0) {
		return ret;
	}

	/* Enable current measurement */
	ret = mfd_npm1300_reg_write(config->mfd, ADC_BASE, ADC_OFFSET_IBAT_EN, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger current and voltage measurement */
	ret = mfd_npm1300_reg_write(config->mfd, ADC_BASE, ADC_OFFSET_TASK_VBAT, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Trigger ntc and die temperature measurements */
	ret = mfd_npm1300_reg_write2(config->mfd, ADC_BASE, ADC_OFFSET_TASK_TEMP, 1U, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Enable automatic temperature measurements during charging */
	ret = mfd_npm1300_reg_write(config->mfd, ADC_BASE, ADC_OFFSET_TASK_AUTO, 1U);
	if (ret != 0) {
		return ret;
	}

	/* Enable charging at low battery if configured */
	if (config->vbatlow_charge_enable) {
		ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_VBATLOW_EN, 1U);
		if (ret != 0) {
			return ret;
		}
	}

	/* Disable automatic recharging if configured */
	if (config->disable_recharge) {
		WRITE_BIT(byte, 0U, true);
	}

	/* Disable NTC if configured */
	if (config->thermistor_idx == 0U) {
		WRITE_BIT(byte, 1U, true);
	}

	ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_DIS_SET, byte);
	if (ret != 0) {
		return ret;
	}

	/* Enable charging if configured */
	if (config->charging_enable) {
		ret = mfd_npm1300_reg_write(config->mfd, CHGR_BASE, CHGR_OFFSET_EN_SET, 1U);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(sensor, npm1300_charger_battery_driver_api) = {
	.sample_fetch = npm1300_charger_sample_fetch,
	.channel_get = npm1300_charger_channel_get,
	.attr_set = npm1300_charger_attr_set,
	.attr_get = npm1300_charger_attr_get,
};

#define NPM1300_CHARGER_INIT(n)                                                                    \
	static struct npm1300_charger_data npm1300_charger_data_##n;                               \
                                                                                                   \
	static const struct npm1300_charger_config npm1300_charger_config_##n = {                  \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.term_microvolt = DT_INST_PROP(n, term_microvolt),                                 \
		.term_warm_microvolt =                                                             \
			DT_INST_PROP_OR(n, term_warm_microvolt, DT_INST_PROP(n, term_microvolt)),  \
		.current_microamp = DT_INST_PROP(n, current_microamp),                             \
		.dischg_limit_microamp = DT_INST_PROP(n, dischg_limit_microamp),                   \
		.vbus_limit_microamp = DT_INST_PROP(n, vbus_limit_microamp),                       \
		.thermistor_ohms = DT_INST_PROP(n, thermistor_ohms),                               \
		.thermistor_idx = DT_INST_ENUM_IDX(n, thermistor_ohms),                            \
		.thermistor_beta = DT_INST_PROP(n, thermistor_beta),                               \
		.charging_enable = DT_INST_PROP(n, charging_enable),                               \
		.trickle_sel = DT_INST_ENUM_IDX(n, trickle_microvolt),                             \
		.iterm_sel = DT_INST_ENUM_IDX(n, term_current_percent),                            \
		.vbatlow_charge_enable = DT_INST_PROP(n, vbatlow_charge_enable),                   \
		.disable_recharge = DT_INST_PROP(n, disable_recharge),                             \
		.dietemp_thresholds = {DT_INST_PROP_OR(n, dietemp_stop_millidegrees, INT32_MAX),   \
				       DT_INST_PROP_OR(n, dietemp_resume_millidegrees,             \
						       INT32_MAX)},                                \
		.temp_thresholds = {DT_INST_PROP_OR(n, thermistor_cold_millidegrees, INT32_MAX),   \
				    DT_INST_PROP_OR(n, thermistor_cool_millidegrees, INT32_MAX),   \
				    DT_INST_PROP_OR(n, thermistor_warm_millidegrees, INT32_MAX),   \
				    DT_INST_PROP_OR(n, thermistor_hot_millidegrees, INT32_MAX)}};  \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &npm1300_charger_init, NULL, &npm1300_charger_data_##n,    \
				     &npm1300_charger_config_##n, POST_KERNEL,                     \
				     CONFIG_SENSOR_INIT_PRIORITY,                                  \
				     &npm1300_charger_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPM1300_CHARGER_INIT)
