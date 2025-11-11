/*
 * Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdint.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(d6f, CONFIG_SENSOR_LOG_LEVEL);

struct d6f_config {
	const struct adc_dt_spec *adc;
	struct adc_sequence sequence;
	const float *polynomial_coefficients;
	uint8_t polynomial_degree;
};

struct d6f_data {
	uint32_t adc_sample;
};

static int d6f_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct d6f_config *config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_FLOW_RATE:
		return adc_read_dt(config->adc, &config->sequence);
	default:
		return -ENOTSUP;
	}
}

static int d6f_flow_rate(const struct d6f_config *config, struct d6f_data *data,
			 struct sensor_value *val)
{
	float flow_rate = config->polynomial_coefficients[0];
	int32_t uv = data->adc_sample;
	float v;
	int rc;

	rc = adc_raw_to_microvolts_dt(config->adc, &uv);
	if (rc != 0) {
		return rc;
	}
	v = uv / 1000000.f;

	for (uint8_t i = 1; i <= config->polynomial_degree; ++i) {
		flow_rate += config->polynomial_coefficients[i] * powf(v, i);
	}

	rc = sensor_value_from_float(val, flow_rate);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

static int d6f_channel_get(const struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val)
{
	const struct d6f_config *config = dev->config;
	struct d6f_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_FLOW_RATE:
		return d6f_flow_rate(config, data, val);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int d6f_init(const struct device *dev)
{
	const struct d6f_config *config = dev->config;
	int rc;

	LOG_DBG("Initializing %s", dev->name);

	if (!adc_is_ready_dt(config->adc)) {
		LOG_ERR("%s not ready", dev->name);
		return -ENODEV;
	}

	rc = adc_channel_setup_dt(config->adc);
	if (rc != 0) {
		LOG_ERR("%s setup failed: %d", config->adc->dev->name, rc);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, d6f_driver_api) = {
	.sample_fetch = d6f_sample_fetch,
	.channel_get = d6f_channel_get,
};

#define D6F_INIT(n, c, p)                                                                          \
	static struct d6f_data d6f_data_##c##_##n;                                                 \
	static const struct adc_dt_spec d6f_adc_##c##_##n = ADC_DT_SPEC_INST_GET(n);               \
	static const struct d6f_config d6f_config_##c##_##n = {                                    \
		.adc = &d6f_adc_##c##_##n,                                                         \
		.sequence =                                                                        \
			{                                                                          \
				.options = NULL,                                                   \
				.channels = BIT(d6f_adc_##c##_##n.channel_id),                     \
				.buffer = &d6f_data_##c##_##n.adc_sample,                          \
				.buffer_size = sizeof(d6f_data_##c##_##n.adc_sample),              \
				.resolution = d6f_adc_##c##_##n.resolution,                        \
				.oversampling = d6f_adc_##c##_##n.oversampling,                    \
				.calibrate = false,                                                \
			},                                                                         \
		.polynomial_coefficients = (p),                                                    \
		.polynomial_degree = (ARRAY_SIZE(p) - 1),                                          \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, d6f_init, NULL, &d6f_data_##c##_##n,                       \
				     &d6f_config_##c##_##n, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &d6f_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT omron_d6f_p0001
static __maybe_unused const float d6f_p0001_polynomial_coefficients[] = {-0.024864, 0.049944};
DT_INST_FOREACH_STATUS_OKAY_VARGS(D6F_INIT, DT_DRV_COMPAT, d6f_p0001_polynomial_coefficients)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT omron_d6f_p0010
static __maybe_unused const float d6f_p0010_polynomial_coefficients[] = {
	-0.269996, 1.060657, -1.601495, 1.374705, 1.374705, -0.564312, 0.094003};
DT_INST_FOREACH_STATUS_OKAY_VARGS(D6F_INIT, DT_DRV_COMPAT, d6f_p0010_polynomial_coefficients)
