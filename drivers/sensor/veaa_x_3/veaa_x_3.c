/*
 * Copyright (c) 2024, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.festo.com/media/pim/620/D15000100140620.PDF
 *
 */

#define DT_DRV_COMPAT festo_veaa_x_3

#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/veaa_x_3.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(veaa_x_3_sensor, CONFIG_SENSOR_LOG_LEVEL);

struct veaa_x_3_data {
	uint16_t adc_buf;
};

struct veaa_x_3_cfg {
	const struct adc_dt_spec adc;
	const struct device *dac;
	const uint8_t dac_channel;
	const uint8_t dac_resolution;
	const uint16_t kpa_max;
	const uint8_t kpa_min;
};

static uint16_t veaa_x_3_kpa_range(const struct veaa_x_3_cfg *cfg)
{
	return cfg->kpa_max - cfg->kpa_min;
}

static int veaa_x_3_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct veaa_x_3_cfg *cfg = dev->config;
	uint32_t tmp;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_veaa_x_3)attr) {
	case SENSOR_ATTR_VEAA_X_3_SETPOINT:
		if (val->val1 > cfg->kpa_max || val->val1 < cfg->kpa_min) {
			LOG_ERR("%d kPa outside range", val->val1);
			return -EINVAL;
		}

		/* Convert from kPa to DAC value */
		tmp = val->val1 - cfg->kpa_min;
		if (u32_mul_overflow(tmp, BIT(cfg->dac_resolution) - 1, &tmp)) {
			LOG_ERR("kPa to DAC overflow");
			return -ERANGE;
		}
		tmp /= veaa_x_3_kpa_range(cfg);

		return dac_write_value(cfg->dac, cfg->dac_channel, tmp);
	default:
		return -ENOTSUP;
	}
}

static int veaa_x_3_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct veaa_x_3_cfg *cfg = dev->config;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_veaa_x_3)attr) {
	case SENSOR_ATTR_VEAA_X_3_RANGE:
		val->val1 = cfg->kpa_min;
		val->val2 = cfg->kpa_max;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int veaa_x_3_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int rc;
	const struct veaa_x_3_cfg *cfg = dev->config;
	struct veaa_x_3_data *data = dev->data;
	struct adc_sequence sequence = {
		.buffer = &data->adc_buf,
		.buffer_size = sizeof(data->adc_buf),
	};

	if (chan != SENSOR_CHAN_PRESS && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	rc = adc_sequence_init_dt(&cfg->adc, &sequence);
	if (rc != 0) {
		return rc;
	}
	sequence.options = NULL;
	sequence.buffer = &data->adc_buf;
	sequence.buffer_size = sizeof(data->adc_buf);
	sequence.calibrate = false;

	rc = adc_read_dt(&cfg->adc, &sequence);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

static int veaa_x_3_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct veaa_x_3_cfg *cfg = dev->config;
	struct veaa_x_3_data *data = dev->data;
	const uint32_t max_adc_val = BIT(cfg->adc.resolution) - 1;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

	/* Convert from ADC value to kPa */
	if (u32_mul_overflow(data->adc_buf, veaa_x_3_kpa_range(cfg), &val->val1)) {
		LOG_ERR("ADC to kPa overflow");
		return -ERANGE;
	}
	val->val2 = (val->val1 % max_adc_val) * 1000000 / max_adc_val;
	val->val1 = (val->val1 / max_adc_val) + cfg->kpa_min;

	return 0;
}

static DEVICE_API(sensor, veaa_x_3_api_funcs) = {
	.attr_set = veaa_x_3_attr_set,
	.attr_get = veaa_x_3_attr_get,
	.sample_fetch = veaa_x_3_sample_fetch,
	.channel_get = veaa_x_3_channel_get,
};

static int veaa_x_3_init(const struct device *dev)
{
	int rc;
	const struct veaa_x_3_cfg *cfg = dev->config;
	const struct dac_channel_cfg dac_cfg = {
		.channel_id = cfg->dac_channel,
		.resolution = cfg->dac_resolution,
		.buffered = false,
	};

	LOG_DBG("Initializing %s with range %u-%u kPa", dev->name, cfg->kpa_min, cfg->kpa_max);

	if (!adc_is_ready_dt(&cfg->adc)) {
		LOG_ERR("ADC not ready");
		return -ENODEV;
	}

	rc = adc_channel_setup_dt(&cfg->adc);
	if (rc != 0) {
		LOG_ERR("%s setup failed: %d", cfg->adc.dev->name, rc);
		return -ENODEV;
	}

	if (!device_is_ready(cfg->dac)) {
		LOG_ERR("DAC not ready");
		return -ENODEV;
	}

	rc = dac_channel_setup(cfg->dac, &dac_cfg);
	if (rc != 0) {
		LOG_ERR("%s setup failed: %d", cfg->dac->name, rc);
		return -ENODEV;
	}

	return 0;
}

#define VEAA_X_3_RANGE_KPA_INIT(n)                                                                 \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, pressure_range_type, d11), ({.max = 1000, min = 5}), \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, pressure_range_type, d9),               \
				 ({.max = 600, min = 3}), ({.max = 200, .min = 1}))))

#define VEAA_X_3_TYPE_INIT(n)                                                                      \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, pressure_range_type, d11),                           \
		    (.kpa_max = 1000, .kpa_min = 5),                                               \
		    (COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, pressure_range_type, d9),               \
				 (.kpa_max = 600, kpa_min = 3), (.kpa_max = 200, .kpa_min = 1))))

#define VEAA_X_3_INIT(n)                                                                           \
                                                                                                   \
	static struct veaa_x_3_data veaa_x_3_data_##n;                                             \
                                                                                                   \
	static const struct veaa_x_3_cfg veaa_x_3_cfg_##n = {                                      \
		.adc = ADC_DT_SPEC_INST_GET(n),                                                    \
		.dac = DEVICE_DT_GET(DT_INST_PHANDLE(n, dac)),                                     \
		.dac_channel = DT_INST_PROP(n, dac_channel_id),                                    \
		.dac_resolution = DT_INST_PROP(n, dac_resolution),                                 \
		VEAA_X_3_TYPE_INIT(n)};                                                            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, veaa_x_3_init, NULL, &veaa_x_3_data_##n,                   \
				     &veaa_x_3_cfg_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,  \
				     &veaa_x_3_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(VEAA_X_3_INIT)
