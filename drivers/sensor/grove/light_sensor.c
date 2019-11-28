/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/adc.h>
#include <device.h>
#include <math.h>
#include <drivers/sensor.h>
#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(grove_light, CONFIG_SENSOR_LOG_LEVEL);

struct gls_data {
	struct device *adc;
	struct adc_channel_cfg ch10_cfg;
	u8_t adc_buffer[4];
};

static struct adc_sequence_options options = {
	.interval_us = 12,
	.extra_samplings = 0,
};

static struct adc_sequence adc_table = {
	.options = &options,
};

#define ADC_RESOLUTION 12

static int gls_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct gls_data *drv_data = dev->driver_data;

	return adc_read(drv_data->adc, &adc_table);
}

static int gls_channel_get(struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gls_data *drv_data = dev->driver_data;
	u16_t analog_val;
	double ldr_val, dval;

	/* rescale sample from 12bit (Zephyr) to 10bit (Grove) */
	analog_val = ((u16_t)drv_data->adc_buffer[1] << 8) |
		     drv_data->adc_buffer[0];
	analog_val = analog_val >> 2;

	/*
	 * The formula for converting the analog value to lux is taken from
	 * the UPM project:
	 * https://github.com/intel-iot-devkit/upm/blob/master/src/grove/grove.cxx#L161
	 */
	ldr_val = (1023.0 - analog_val) * 10.0 / analog_val;
	dval = 10000.0 / pow(ldr_val * 15.0, 4.0/3.0);

	val->val1 = (s32_t)dval;
	val->val2 = ((s32_t)(dval * 1000000)) % 1000000;

	return 0;
}

static const struct sensor_driver_api gls_api = {
	.sample_fetch = &gls_sample_fetch,
	.channel_get = &gls_channel_get,
};

static int gls_init(struct device *dev)
{
	struct gls_data *drv_data = dev->driver_data;

	drv_data->adc =
		device_get_binding(CONFIG_GROVE_LIGHT_SENSOR_ADC_DEV_NAME);
	if (drv_data->adc == NULL) {
		LOG_ERR("Failed to get ADC device.");
		return -EINVAL;
	}

	/*Change following parameters according to board if necessary*/
	drv_data->ch10_cfg.channel_id =	CONFIG_GROVE_LIGHT_SENSOR_ADC_CHANNEL;
	drv_data->ch10_cfg.differential = false;
	drv_data->ch10_cfg.gain = ADC_GAIN_1,
	drv_data->ch10_cfg.reference = ADC_REF_INTERNAL;
	drv_data->ch10_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
	adc_table.buffer = drv_data->adc_buffer;
	adc_table.channels = BIT(CONFIG_GROVE_LIGHT_SENSOR_ADC_CHANNEL);
	adc_table.resolution = ADC_RESOLUTION;
	adc_table.buffer_size = 4;

	adc_channel_setup(drv_data->adc, &drv_data->ch10_cfg);

	return 0;
}

static struct gls_data gls_data;

DEVICE_AND_API_INIT(gls_dev, CONFIG_GROVE_LIGHT_SENSOR_NAME, &gls_init,
		&gls_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&gls_api);
