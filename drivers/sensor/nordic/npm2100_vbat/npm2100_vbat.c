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

struct npm2100_vbat_config {
	struct i2c_dt_spec i2c;
	struct sensor_value voutmin;
	struct sensor_value vbatmin;
};

struct adc_results_t {
	uint8_t vbat;
	uint8_t temp;
	uint8_t droop;
	uint8_t vout;
	uint8_t voutrecov;
	uint8_t average;
	uint8_t boost;
	uint8_t status;
} __packed;

struct npm2100_vbat_data {
	struct adc_results_t results;
	uint8_t dpscount;
};

#define BOOST_DPSCOUNT 0x25U
#define BOOST_DPSLIMIT 0x26U
#define BOOST_CTRLSET  0x2AU
#define BOOST_CTRLCLR  0x2BU
#define BOOST_VBATSEL  0x2EU
#define BOOST_VBATMINL 0x2FU
#define BOOST_VBATMINH 0x30U
#define BOOST_VOUTMIN  0x31U
#define BOOST_VOUTWRN  0x32U
#define BOOST_VOUTDPS  0x33U

#define ADC_TASKS_START 0x90U
#define ADC_CONFIG      0X91U
#define ADC_DELAY       0x92U
#define ADC_OFFSETCFG   0x93U
#define ADC_CTRLSET     0x94U
#define ADC_CTRLCLR     0x95U
#define ADC_RESULTS     0x96U

#define ADC_CONFIG_MODE_INS_VBAT 0x00U
#define ADC_CONFIG_MODE_DEL_VBAT 0x01U
#define ADC_CONFIG_MODE_TEMP     0x02U
#define ADC_CONFIG_MODE_DROOP    0x03U
#define ADC_CONFIG_MODE_VOUT     0x04U
#define ADC_CONFIG_MODE_OFFSET   0x05U

#define ADC_SAMPLE_TIME_US 100

#define VBAT_SCALING_MUL    3200000
#define VBAT_SCALING_DIV    256
#define VOUT_SCALING_OFFSET 1800000
#define VOUT_SCALING_MUL    1500000
#define VOUT_SCALING_DIV    256
#define TEMP_SCALING_OFFSET 389500
#define TEMP_SCALING_MUL    212000
#define TEMP_SCALING_DIV    100

static const struct linear_range vbat_range = LINEAR_RANGE_INIT(700000, 50000, 0U, 46U);
static const struct linear_range vout_range = LINEAR_RANGE_INIT(1800000, 50000, 0U, 31U);
static const struct linear_range vdps_range = LINEAR_RANGE_INIT(1700000, 50000, 0U, 31U);
static const struct linear_range dpslim_range = LINEAR_RANGE_INIT(3, 1, 3U, 255U);

struct npm2100_attr_t {
	enum sensor_channel chan;
	enum sensor_attribute attr;
	const struct linear_range *range;
	uint8_t reg;
	uint8_t ctrlsel_mask;
};

static const struct npm2100_attr_t npm2100_attr[] = {
	{SENSOR_CHAN_GAUGE_VOLTAGE, SENSOR_ATTR_UPPER_THRESH, &vbat_range, BOOST_VBATMINH, 0},
	{SENSOR_CHAN_GAUGE_VOLTAGE, SENSOR_ATTR_LOWER_THRESH, &vbat_range, BOOST_VBATMINL, 0},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_UPPER_THRESH, &vdps_range, BOOST_VOUTDPS, BIT(2)},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH, &vout_range, BOOST_VOUTMIN, BIT(0)},
	{SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_ALERT, &vout_range, BOOST_VOUTWRN, BIT(1)},
	{(enum sensor_channel)SENSOR_CHAN_NPM2100_DPS_COUNT, SENSOR_ATTR_UPPER_THRESH,
	 &dpslim_range, BOOST_DPSLIMIT, 0},
};

int npm2100_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *valp)
{
	struct npm2100_vbat_data *const data = dev->data;
	int32_t tmp;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		tmp = ((int32_t)data->results.vbat * VBAT_SCALING_MUL) / VBAT_SCALING_DIV;
		valp->val1 = tmp / 1000000;
		valp->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_VOLTAGE:
		tmp = VOUT_SCALING_OFFSET +
		      (((int32_t)data->results.vout * VOUT_SCALING_MUL) / VOUT_SCALING_DIV);
		valp->val1 = tmp / 1000000;
		valp->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_NPM2100_VOLT_DROOP:
		tmp = VOUT_SCALING_OFFSET +
		      (((int32_t)data->results.vout * VOUT_SCALING_MUL) / VOUT_SCALING_DIV);
		valp->val1 = tmp / 1000000;
		valp->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		tmp = TEMP_SCALING_OFFSET -
		      (((int32_t)data->results.temp * TEMP_SCALING_MUL) / TEMP_SCALING_DIV);
		valp->val1 = tmp / 1000;
		valp->val2 = (tmp % 1000) * 1000;
		break;
	case SENSOR_CHAN_NPM2100_VBAT_STATUS:
		valp->val1 = data->results.status;
		valp->val2 = 0;
		break;
	case SENSOR_CHAN_NPM2100_DPS_COUNT:
		valp->val1 = data->dpscount;
		valp->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

int npm2100_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct npm2100_vbat_config *const config = dev->config;
	struct npm2100_vbat_data *data = dev->data;
	int ret;

	/* All ADC channels need to be triggered individually */
	uint8_t mode_list[] = {ADC_CONFIG_MODE_INS_VBAT, ADC_CONFIG_MODE_TEMP,
			       ADC_CONFIG_MODE_VOUT};

	for (int mode = 0; mode < ARRAY_SIZE(mode_list); mode++) {
		ret = i2c_reg_write_byte_dt(&config->i2c, ADC_CONFIG, mode_list[mode]);
		if (ret != 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, ADC_TASKS_START, 1U);
		if (ret != 0) {
			return ret;
		}

		k_sleep(K_USEC(ADC_SAMPLE_TIME_US));
	}

	/* Read adc results */
	ret = i2c_write_read_dt(&config->i2c, &(uint8_t){ADC_RESULTS}, 1U, &data->results,
				sizeof(struct adc_results_t));
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_read_byte_dt(&config->i2c, BOOST_DPSCOUNT, &data->dpscount);
}

static int npm2100_vbat_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	const struct npm2100_vbat_config *const config = dev->config;

	for (int idx = 0; idx < ARRAY_SIZE(npm2100_attr); idx++) {
		if ((npm2100_attr[idx].chan == chan) && (npm2100_attr[idx].attr == attr)) {

			int32_t val_mv;
			uint8_t data;

			int ret = i2c_reg_read_byte_dt(&config->i2c, npm2100_attr[idx].reg, &data);

			if (ret < 0) {
				return ret;
			}

			ret = linear_range_get_value(npm2100_attr[idx].range, data, &val_mv);
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

	for (int idx = 0; idx < ARRAY_SIZE(npm2100_attr); idx++) {
		if ((npm2100_attr[idx].chan == chan) && (npm2100_attr[idx].attr == attr)) {

			uint16_t data;

			int ret = linear_range_get_index(npm2100_attr[idx].range,
							 sensor_value_to_micro(val), &data);

			if (ret == -EINVAL) {
				return ret;
			}

			if (npm2100_attr[idx].ctrlsel_mask == 0) {
				/* No control bit, so just set threshold */
				return i2c_reg_write_byte_dt(&config->i2c, npm2100_attr[idx].reg,
							     data);
			}

			/* Disable comparator */
			ret = i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLCLR,
						    npm2100_attr[idx].ctrlsel_mask);
			if (ret < 0) {
				return ret;
			}

			/* Set threshold */
			ret = i2c_reg_write_byte_dt(&config->i2c, npm2100_attr[idx].reg, data);
			if (ret < 0) {
				return ret;
			}

			/* Enable comparator */
			return i2c_reg_write_byte_dt(&config->i2c, BOOST_CTRLSET,
						     npm2100_attr[idx].ctrlsel_mask);
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

static const struct sensor_driver_api npm2100_vbat_battery_driver_api = {
	.sample_fetch = npm2100_vbat_sample_fetch,
	.channel_get = npm2100_vbat_channel_get,
	.attr_set = npm2100_vbat_attr_set,
	.attr_get = npm2100_vbat_attr_get,
};

#define NPM2100_VBAT_INIT(n)                                                                       \
	static struct npm2100_vbat_data npm2100_vbat_data_##n;                                     \
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
