/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adc.h>
#include <device.h>
#include <math.h>
#include <sensor.h>
#include <zephyr.h>

#define SYS_LOG_DOMAIN "GROVE_TEMPERATURE_SENSOR"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_GROVE_LEVEL
#include <logging/sys_log.h>

/* thermistor Nominal B-Constant */
#if defined(CONFIG_GROVE_TEMPERATURE_SENSOR_V1_0)
	#define B_CONST			3975
#elif defined(CONFIG_GROVE_TEMPERATURE_SENSOR_V1_X)
	#define B_CONST			4250
#endif

#define ADC_RESOLUTION 12

struct gts_data {
	struct device *adc;
	struct adc_channel_cfg ch10_cfg;
	u8_t adc_buffer[4];
};

static struct adc_sequence_options options = {
	.extra_samplings = 0,
	.interval_us = 15,
};

static struct adc_sequence adc_table = {
	.options = &options,
};

static int gts_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct gts_data *drv_data = dev->driver_data;

	return adc_read(drv_data->adc, &adc_table);
}

static int gts_channel_get(struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gts_data *drv_data = dev->driver_data;
	u16_t analog_val;
	double dval;

	/* rescale sample from 12bit (Zephyr) to 10bit (Grove) */
	analog_val = ((u16_t)drv_data->adc_buffer[1] << 8) |
		     drv_data->adc_buffer[0];
	analog_val = analog_val >> 2;

	/*
	 * The formula for converting the analog value to degrees Celisus
	 * is taken from the sensor reference page:
	 *     http://www.seeedstudio.com/wiki/Grove_-_Temperature_Sensor
	 */
	dval = 1 / (log(1023.0 / analog_val - 1.0) / B_CONST +
		    1 / 298.15) - 273.15;

	val->val1 = (s32_t)dval;
	val->val2 = ((s32_t)(dval * 1000000)) % 1000000;

	return 0;
}

static const struct sensor_driver_api gts_api = {
	.sample_fetch = &gts_sample_fetch,
	.channel_get = &gts_channel_get,
};

static int gts_init(struct device *dev)
{
	struct gts_data *drv_data = dev->driver_data;

	drv_data->adc = device_get_binding(
		CONFIG_GROVE_TEMPERATURE_SENSOR_ADC_DEV_NAME);
	if (drv_data->adc == NULL) {
		SYS_LOG_ERR("Failed to get ADC device.");
		return -EINVAL;
	}

	/*Change following parameters according to board if necessary*/
	drv_data->ch10_cfg.channel_id =	CONFIG_GROVE_TEMPERATURE_SENSOR_ADC_CHANNEL;
	drv_data->ch10_cfg.differential = false;
	drv_data->ch10_cfg.gain = ADC_GAIN_1,
	drv_data->ch10_cfg.reference = ADC_REF_INTERNAL;
	drv_data->ch10_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
	adc_table.buffer = drv_data->adc_buffer;
	adc_table.resolution = ADC_RESOLUTION;
	adc_table.buffer_size = 4;
	adc_table.channels = BIT(CONFIG_GROVE_TEMPERATURE_SENSOR_ADC_CHANNEL);

	adc_channel_setup(drv_data->adc, &drv_data->ch10_cfg);

	dev->driver_api = &gts_api;

	return 0;
}

static struct gts_data gts_data;

DEVICE_INIT(gts_dev, CONFIG_GROVE_TEMPERATURE_SENSOR_NAME, &gts_init, &gts_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
