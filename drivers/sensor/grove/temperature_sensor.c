/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT seeed_grove_temperature

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <math.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(grove_temp, CONFIG_SENSOR_LOG_LEVEL);

/* The effect of gain and reference voltage must cancel. */
#ifdef CONFIG_ADC_NRFX_SAADC
#define GROVE_GAIN ADC_GAIN_1_4
#define GROVE_REF ADC_REF_VDD_1_4
#define GROVE_RESOLUTION 12
#else
#define GROVE_GAIN ADC_GAIN_1
#define GROVE_REF ADC_REF_VDD_1
#define GROVE_RESOLUTION 12
#endif

struct gts_data {
	struct adc_channel_cfg ch_cfg;
	uint16_t raw;
};

struct gts_config {
	const struct device *adc;
	int16_t b_const;
	uint8_t adc_channel;
};

static struct adc_sequence_options options = {
	.extra_samplings = 0,
	.interval_us = 15,
};

static struct adc_sequence adc_table = {
	.options = &options,
};

static int gts_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	const struct gts_config *cfg = dev->config;

	return adc_read(cfg->adc, &adc_table);
}

static int gts_channel_get(const struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gts_data *drv_data = dev->data;
	const struct gts_config *cfg = dev->config;
	double dval;

	/*
	 * The formula for converting the analog value to degrees Celsius
	 * is taken from the sensor reference page:
	 *     http://www.seeedstudio.com/wiki/Grove_-_Temperature_Sensor
	 */
	dval = (1 / (log((BIT(GROVE_RESOLUTION) - 1.0)
			/ drv_data->raw
			- 1.0)
		     / cfg->b_const
		     + (1 / 298.15)))
		- 273.15;
	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;

	return 0;
}

static const struct sensor_driver_api gts_api = {
	.sample_fetch = &gts_sample_fetch,
	.channel_get = &gts_channel_get,
};

static int gts_init(const struct device *dev)
{
	struct gts_data *drv_data = dev->data;
	const struct gts_config *cfg = dev->config;

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("ADC device is not ready.");
		return -EINVAL;
	}

	/*Change following parameters according to board if necessary*/
	drv_data->ch_cfg = (struct adc_channel_cfg){
		.gain = GROVE_GAIN,
		.reference = GROVE_REF,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		.channel_id = cfg->adc_channel,
#ifdef CONFIG_ADC_NRFX_SAADC
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + cfg->adc_channel,
#endif
	};
	adc_table.buffer = &drv_data->raw;
	adc_table.buffer_size = sizeof(drv_data->raw);
	adc_table.resolution = GROVE_RESOLUTION;
	adc_table.channels = BIT(cfg->adc_channel);

	adc_channel_setup(cfg->adc, &drv_data->ch_cfg);

	return 0;
}

static struct gts_data gts_data;
static const struct gts_config gts_cfg = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.b_const = (IS_ENABLED(DT_INST_PROP(0, v1p0))
		    ? 3975
		    : 4250),
	.adc_channel = DT_INST_IO_CHANNELS_INPUT(0),
};

DEVICE_DT_INST_DEFINE(0, &gts_init, NULL,
		&gts_data, &gts_cfg, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&gts_api);
