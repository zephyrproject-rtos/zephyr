/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm2100_vbat

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/npm2100.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/drivers/sensor/npm2100_vbat.h>

LOG_MODULE_REGISTER(vbat_npm2100, CONFIG_SENSOR_LOG_LEVEL);

#define BOOST_START    0x20U
#define BOOST_OPER     0x24U
#define BOOST_DPSCOUNT 0x25U
#define BOOST_DPSLIMIT 0x26U
#define BOOST_DPSDUR   0x27U
#define BOOST_CTRLSET  0x2AU
#define BOOST_CTRLCLR  0x2BU
#define BOOST_VBATSEL  0x2EU
#define BOOST_VBATMINL 0x2FU
#define BOOST_VBATMINH 0x30U
#define BOOST_VOUTMIN  0x31U
#define BOOST_VOUTWRN  0x32U
#define BOOST_VOUTDPS  0x33U

#define ADC_TASKS_ADC      0x90U
#define ADC_CONFIG         0X91U
#define ADC_DELAY          0x92U
#define ADC_OFFSETCFG      0x93U
#define ADC_CTRLSET        0x94U
#define ADC_CTRLCLR        0x95U
#define ADC_RESULTS        0x96U
#define ADC_READVBAT       0x96U
#define ADC_READTEMP       0x97U
#define ADC_READDROOP      0x98U
#define ADC_READVOUT       0x99U
#define ADC_AVERAGE        0x9BU
#define ADC_OFFSETMEASURED 0x9FU

#define ADC_CONFIG_MODE_MASK     0x07U
#define ADC_CONFIG_MODE_INS_VBAT 0x00U
#define ADC_CONFIG_MODE_DEL_VBAT 0x01U
#define ADC_CONFIG_MODE_TEMP     0x02U
#define ADC_CONFIG_MODE_DROOP    0x03U
#define ADC_CONFIG_MODE_VOUT     0x04U
#define ADC_CONFIG_MODE_OFFSET   0x05U
#define ADC_CONFIG_AVG_MASK      0x38U

#define ADC_SAMPLE_TIME_US 100

#define VBAT_SCALING_OFFSET 0
#define VBAT_SCALING_MUL    3200000
#define VBAT_SCALING_DIV    256
#define VOUT_SCALING_OFFSET 1800000
#define VOUT_SCALING_MUL    1500000
#define VOUT_SCALING_DIV    256
#define TEMP_SCALING_OFFSET 389500000
#define TEMP_SCALING_MUL    2120000
#define TEMP_SCALING_DIV    -1
#define DPS_SCALING_OFFSET  0
#define DPS_SCALING_MUL     1000000
#define DPS_SCALING_DIV     1

static const struct linear_range vbat_range = LINEAR_RANGE_INIT(650000, 50000, 0U, 50U);
static const struct linear_range vout_range = LINEAR_RANGE_INIT(1700000, 50000, 0U, 31U);
static const struct linear_range vdps_range = LINEAR_RANGE_INIT(1900000, 50000, 0U, 31U);
static const struct linear_range dpslim_range = LINEAR_RANGE_INIT(0, 1, 0U, 255U);
static const struct linear_range dpstimer_range = LINEAR_RANGE_INIT(0, 1, 0U, 3U);
static const struct linear_range oversample_range = LINEAR_RANGE_INIT(0, 1, 0U, 4U);
static const struct linear_range adcdelay_range = LINEAR_RANGE_INIT(5000, 4000, 0U, 255U);

struct npm2100_vbat_config {
	struct i2c_dt_spec i2c;
	struct sensor_value voutmin;
	struct sensor_value vbatmin;
};

struct adc_config {
	const enum sensor_channel chan;
	const uint8_t result_reg;
	uint8_t config;
	uint8_t result;
	bool enabled;
};

struct npm2100_vbat_data {
	struct adc_config adc[4U];
	uint8_t vbat_delay;
	uint8_t dpsdur;
};

struct npm2100_attr_t {
	enum sensor_channel chan;
	enum sensor_attribute attr;
	const struct linear_range *range;
	uint8_t reg;
	uint8_t reg_mask;
	uint8_t ctrlsel_mask;
};

static const struct npm2100_attr_t npm2100_attr[] = {
	{SENSOR_CHAN_GAUGE_VOLTAGE, SENSOR_ATTR_UPPER_THRESH, &vbat_range, BOOST_VBATMINH, 0xFF, 0},
	{SENSOR_CHAN_GAUGE_VOLTAGE, SENSOR_ATTR_LOWER_THRESH, &vbat_range, BOOST_VBATMINL, 0xFF, 0},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_UPPER_THRESH, &vdps_range, BOOST_VOUTDPS, 0xFF, BIT(2)},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH, &vout_range, BOOST_VOUTMIN, 0xFF, BIT(0)},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_ALERT, &vout_range, BOOST_VOUTWRN, 0xFF, BIT(1)},
	{(enum sensor_channel)SENSOR_CHAN_NPM2100_DPS_COUNT, SENSOR_ATTR_UPPER_THRESH,
	 &dpslim_range, BOOST_DPSLIMIT, 0xFF, 0},
	{(enum sensor_channel)SENSOR_CHAN_NPM2100_DPS_TIMER, SENSOR_ATTR_UPPER_THRESH,
	 &dpstimer_range, BOOST_OPER, 0x60, 0},
};

static struct adc_config *adc_cfg_get(const struct device *dev, enum sensor_channel chan)
{
	struct npm2100_vbat_data *const data = dev->data;

	for (int idx = 0; idx < ARRAY_SIZE(data->adc); idx++) {
		if (data->adc[idx].chan == chan) {
			return &data->adc[idx];
		}
	}

	return NULL;
}

int npm2100_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *valp)
{
	uint8_t *result = NULL;

	if ((uint32_t)chan == SENSOR_CHAN_NPM2100_DPS_DURATION) {
		struct npm2100_vbat_data *const data = dev->data;

		result = &data->dpsdur;
	} else {
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);

		if (!adc_cfg) {
			return -ENOTSUP;
		}

		result = &adc_cfg->result;
	}

	if (!result) {
		return -ENOTSUP;
	}

	int32_t scaling_mul;
	int32_t scaling_div;
	int32_t scaling_off;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		scaling_mul = VBAT_SCALING_MUL;
		scaling_div = VBAT_SCALING_DIV;
		scaling_off = VBAT_SCALING_OFFSET;
		break;
	case SENSOR_CHAN_VOLTAGE:
		/* Fall through */
	case SENSOR_CHAN_NPM2100_VOLT_DROOP:
		scaling_mul = VOUT_SCALING_MUL;
		scaling_div = VOUT_SCALING_DIV;
		scaling_off = VOUT_SCALING_OFFSET;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		scaling_mul = TEMP_SCALING_MUL;
		scaling_div = TEMP_SCALING_DIV;
		scaling_off = TEMP_SCALING_OFFSET;
		break;
	case SENSOR_CHAN_NPM2100_DPS_DURATION:
		scaling_mul = DPS_SCALING_MUL;
		scaling_div = DPS_SCALING_DIV;
		scaling_off = DPS_SCALING_OFFSET;
		break;
	default:
		return -ENOTSUP;
	}

	int32_t tmp = scaling_off + ((int32_t)*result * scaling_mul) / scaling_div;

	valp->val1 = tmp / 1000000;
	valp->val2 = tmp % 1000000;

	return 0;
}

int npm2100_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct npm2100_vbat_config *const config = dev->config;
	struct npm2100_vbat_data *data = dev->data;
	int ret;

	for (int idx = 0; idx < ARRAY_SIZE(data->adc); idx++) {
		if (!data->adc[idx].enabled) {
			continue;
		}

		/* Oversampling by 2^field */
		const uint8_t oversampling =
			BIT(FIELD_GET(ADC_CONFIG_AVG_MASK, data->adc[idx].config));
		int32_t delay_usec;

		if (chan == SENSOR_CHAN_GAUGE_VOLTAGE) {
			ret = linear_range_get_value(&adcdelay_range, data->vbat_delay,
						     &delay_usec);
			if (ret < 0) {
				return ret;
			}
		} else {
			delay_usec = 0;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, ADC_CONFIG, data->adc[idx].config);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, ADC_TASKS_ADC, 1U);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_USEC(ADC_SAMPLE_TIME_US * oversampling + delay_usec));

		if (oversampling > 1) {
			ret = i2c_reg_read_byte_dt(&config->i2c, ADC_AVERAGE,
						   &data->adc[idx].result);
			if (ret < 0) {
				return ret;
			}
		} else {
			ret = i2c_reg_read_byte_dt(&config->i2c, data->adc[idx].result_reg,
						   &data->adc[idx].result);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* Fetch previous DPS duration result before triggering new one.The time it takes to get
	 * the DPS duration depends on many factors and cannot be predicted here.
	 */
	ret = i2c_reg_read_byte_dt(&config->i2c, BOOST_DPSDUR, &data->dpsdur);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->i2c, BOOST_START, 2U);
}

static int npm2100_vbat_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	const struct npm2100_vbat_config *const config = dev->config;

	if (attr == SENSOR_ATTR_FEATURE_MASK) {
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);

		if (!adc_cfg) {
			return -EINVAL;
		}

		*val = (struct sensor_value){.val1 = adc_cfg->enabled, .val2 = 0};

		return 0;
	}

	if (attr == SENSOR_ATTR_OVERSAMPLING) {
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);

		if (!adc_cfg) {
			return -EINVAL;
		}

		*val = (struct sensor_value){
			.val1 = FIELD_GET(ADC_CONFIG_AVG_MASK, adc_cfg->config), .val2 = 0};

		return 0;
	}

	/* Delay of vbat ADC measurement */
	if ((chan == SENSOR_CHAN_GAUGE_VOLTAGE) &&
	    (attr == (enum sensor_attribute)SENSOR_ATTR_NPM2100_ADC_DELAY)) {
		struct npm2100_vbat_data *const data = dev->data;
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);
		int32_t val_usec;

		if (!adc_cfg) {
			return -ENOENT;
		}

		if (FIELD_GET(ADC_CONFIG_MODE_MASK, adc_cfg->config) == ADC_CONFIG_MODE_INS_VBAT) {
			/* Instant measurement */
			return sensor_value_from_micro(val, 0);
		}

		int ret = linear_range_get_value(&adcdelay_range, data->vbat_delay, &val_usec);

		if (ret < 0) {
			return ret;
		}

		return sensor_value_from_micro(val, val_usec);
	}

	for (int idx = 0; idx < ARRAY_SIZE(npm2100_attr); idx++) {
		if ((npm2100_attr[idx].chan == chan) && (npm2100_attr[idx].attr == attr)) {

			int32_t val_mv;
			uint8_t reg_data;

			int ret = i2c_reg_read_byte_dt(&config->i2c, npm2100_attr[idx].reg,
						       &reg_data);

			if (ret < 0) {
				return ret;
			}

			reg_data = FIELD_GET(npm2100_attr[idx].reg_mask, reg_data);

			ret = linear_range_get_value(npm2100_attr[idx].range, reg_data, &val_mv);
			if (ret < 0) {
				return ret;
			}

			val->val1 = val_mv / 1000000;
			val->val2 = val_mv % 1000000;

			return 0;
		}
	}

	return -ENOTSUP;
}

static int npm2100_vbat_attr_set(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct npm2100_vbat_config *const config = dev->config;
	int ret;

	/* ADC sampling feature masks to enable individual measurements */
	if (attr == SENSOR_ATTR_FEATURE_MASK) {
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);

		if (!adc_cfg) {
			return -EINVAL;
		}

		adc_cfg->enabled = val->val1 == 0 ? false : true;

		return 0;
	}

	/* Delay of vbat ADC measurement */
	if ((chan == SENSOR_CHAN_GAUGE_VOLTAGE) &&
	    (attr == (enum sensor_attribute)SENSOR_ATTR_NPM2100_ADC_DELAY)) {
		struct npm2100_vbat_data *const data = dev->data;
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);
		int32_t val_usec = sensor_value_to_micro(val);
		uint16_t delay;

		if (!adc_cfg) {
			return -ENOENT;
		}

		if (val_usec == 0) {
			delay = 0;
		} else {
			ret = linear_range_get_index(&adcdelay_range, val_usec, &delay);
			if (ret < 0) {
				return ret;
			}

			ret = i2c_reg_write_byte_dt(&config->i2c, ADC_DELAY, delay);
			if (ret < 0) {
				return ret;
			}
		}

		/* Delayed vbat measurement uses different mode */
		data->vbat_delay = delay;
		adc_cfg->config &= ~ADC_CONFIG_MODE_MASK;
		if (delay == 0) {
			adc_cfg->config |=
				FIELD_PREP(ADC_CONFIG_MODE_MASK, ADC_CONFIG_MODE_INS_VBAT);
		} else {
			adc_cfg->config |=
				FIELD_PREP(ADC_CONFIG_MODE_MASK, ADC_CONFIG_MODE_DEL_VBAT);
		}

		return 0;
	}

	/* ADC per-channel oversampling */
	if (attr == SENSOR_ATTR_OVERSAMPLING) {
		struct adc_config *adc_cfg = adc_cfg_get(dev, chan);
		uint16_t oversample;

		if (!adc_cfg) {
			return -ENOENT;
		}

		/* Oversample factor is 2^value */
		ret = linear_range_get_index(&oversample_range, val->val1, &oversample);
		if (ret < 0) {
			return ret;
		}

		adc_cfg->config &= ~ADC_CONFIG_AVG_MASK;
		adc_cfg->config |= FIELD_PREP(ADC_CONFIG_AVG_MASK, oversample);

		return 0;
	}

	/* Threshold attributes */
	for (int idx = 0; idx < ARRAY_SIZE(npm2100_attr); idx++) {
		if ((npm2100_attr[idx].chan == chan) && (npm2100_attr[idx].attr == attr)) {

			uint16_t range_idx;
			uint8_t reg_data;

			ret = linear_range_get_index(npm2100_attr[idx].range,
						     sensor_value_to_micro(val), &range_idx);

			if (ret < 0) {
				return ret;
			}

			reg_data = FIELD_PREP(npm2100_attr[idx].reg_mask, range_idx);

			if (npm2100_attr[idx].ctrlsel_mask != 0) {
				/* Disable comparator */
				ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLCLR,
							    npm2100_attr[idx].ctrlsel_mask);
				if (ret < 0) {
					return ret;
				}
			}

			/* Set threshold */
			if (npm2100_attr[idx].reg_mask == 0xFFU) {
				ret = i2c_reg_write_byte_dt(&config->i2c, npm2100_attr[idx].reg,
							    reg_data);
			} else {
				ret = i2c_reg_update_byte_dt(&config->i2c, npm2100_attr[idx].reg,
							     npm2100_attr[idx].reg_mask, reg_data);
			}
			if (ret < 0) {
				return ret;
			}

			if (npm2100_attr[idx].ctrlsel_mask != 0) {
				/* Enable comparator */
				ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLSET,
							    npm2100_attr[idx].ctrlsel_mask);
			}

			return ret;
		}
	}

	return -ENOTSUP;
}

int npm2100_vbat_init(const struct device *dev)
{
	const struct npm2100_vbat_config *const config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s i2c not ready", dev->name);
		return -ENODEV;
	}

	/* Set initial voltage thresholds */
	if ((config->voutmin.val1 != 0) || (config->voutmin.val2 != 0)) {
		ret = npm2100_vbat_attr_set(dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH,
					    &config->voutmin);
		if (ret < 0) {
			return ret;
		}
	}

	if ((config->vbatmin.val1 != 0) || (config->vbatmin.val2 != 0)) {
		ret = npm2100_vbat_attr_set(dev, SENSOR_CHAN_GAUGE_VOLTAGE,
					    SENSOR_ATTR_UPPER_THRESH, &config->vbatmin);
		if (ret < 0) {
			return ret;
		}

		ret = npm2100_vbat_attr_set(dev, SENSOR_CHAN_GAUGE_VOLTAGE,
					    SENSOR_ATTR_LOWER_THRESH, &config->vbatmin);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set MEE thresholds to SW control */
	ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_VBATSEL, 3U);
	if (ret < 0) {
		return ret;
	}

	/* Allow VOUTMIN comparator to select VBATMIN threshold */
	return i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLSET, 0x10U);
}

static DEVICE_API(sensor, npm2100_vbat_battery_driver_api) = {
	.sample_fetch = npm2100_vbat_sample_fetch,
	.channel_get = npm2100_vbat_channel_get,
	.attr_set = npm2100_vbat_attr_set,
	.attr_get = npm2100_vbat_attr_get,
};

/* DPS-related measurements off by default. Enable via attribute feature masks. */
#define NPM2100_VBAT_DATA_INIT()                                                                   \
	{                                                                                          \
		.adc =                                                                             \
			{                                                                          \
				{.chan = SENSOR_CHAN_GAUGE_VOLTAGE,                                \
				 .config = ADC_CONFIG_MODE_INS_VBAT,                               \
				 .result_reg = ADC_READVBAT,                                       \
				 .enabled = true},                                                 \
				{.chan = SENSOR_CHAN_VOLTAGE,                                      \
				 .config = ADC_CONFIG_MODE_VOUT,                                   \
				 .result_reg = ADC_READVOUT,                                       \
				 .enabled = true},                                                 \
				{.chan = SENSOR_CHAN_DIE_TEMP,                                     \
				 .config = ADC_CONFIG_MODE_TEMP,                                   \
				 .result_reg = ADC_READTEMP,                                       \
				 .enabled = true},                                                 \
				{.chan = (enum sensor_channel)SENSOR_CHAN_NPM2100_VOLT_DROOP,      \
				 .config = ADC_CONFIG_MODE_DROOP,                                  \
				 .result_reg = ADC_READDROOP,                                      \
				 .enabled = false},                                                \
			},                                                                         \
		.vbat_delay = 0, .dpsdur = 0,                                                      \
	}

#define NPM2100_VBAT_INIT(n)                                                                       \
	static struct npm2100_vbat_data npm2100_vbat_data_##n = NPM2100_VBAT_DATA_INIT();          \
                                                                                                   \
	static const struct npm2100_vbat_config npm2100_vbat_config_##n = {                        \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
		.voutmin = {.val1 = DT_INST_PROP_OR(n, vout_min_microvolt, 0) / 1000000,           \
			    .val2 = DT_INST_PROP_OR(n, vout_min_microvolt, 0) % 1000000},          \
		.vbatmin = {.val1 = DT_INST_PROP_OR(n, vbat_min_microvolt, 0) / 1000000,           \
			    .val2 = DT_INST_PROP_OR(n, vbat_min_microvolt, 0) % 1000000},          \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		n, &npm2100_vbat_init, NULL, &npm2100_vbat_data_##n, &npm2100_vbat_config_##n,     \
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &npm2100_vbat_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPM2100_VBAT_INIT)
