/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT seeed_grove_light

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <math.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(grove_light, CONFIG_SENSOR_LOG_LEVEL);

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


struct gls_data {
	const struct device *adc;
	struct adc_channel_cfg ch_cfg;
	uint16_t raw;
};

struct gls_config {
	const struct device *adc;
	uint8_t adc_channel;
};

static struct adc_sequence_options options = {
	.interval_us = 12,
	.extra_samplings = 0,
};

static struct adc_sequence adc_table = {
	.options = &options,
};

static int gls_sample_fetch(const struct device *dev,
			    enum sensor_channel chan)
{
	const struct gls_config *cfg = dev->config;

	return adc_read(cfg->adc, &adc_table);
}

static int gls_channel_get(const struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gls_data *drv_data = dev->data;
	uint16_t analog_val = drv_data->raw;
	double ldr_val, dval;

	/*
	 * The formula for converting the analog value to lux is taken from
	 * the UPM project:
	 * https://github.com/intel-iot-devkit/upm/blob/master/src/grove/grove.cxx#L161
	 */
	ldr_val = (BIT(GROVE_RESOLUTION) - 1.0 - analog_val) * 10.0 / analog_val;
	dval = 10000.0 / pow(ldr_val * 15.0, 4.0/3.0);

	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;

	return 0;
}

static const struct sensor_driver_api gls_api = {
	.sample_fetch = &gls_sample_fetch,
	.channel_get = &gls_channel_get,
};

static int gls_init(const struct device *dev)
{
	struct gls_data *drv_data = dev->data;
	const struct gls_config *cfg = dev->config;

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

static struct gls_data gls_data;
static const struct gls_config gls_cfg = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.adc_channel = DT_INST_IO_CHANNELS_INPUT(0),
};

DEVICE_DT_INST_DEFINE(0, &gls_init, NULL,
		&gls_data, &gls_cfg, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&gls_api);
