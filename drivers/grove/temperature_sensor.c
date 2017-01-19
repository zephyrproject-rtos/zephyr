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

struct gts_data {
	struct device *adc;
	struct adc_seq_entry sample;
	struct adc_seq_table adc_table;
	uint8_t adc_buffer[4];
};

static int gts_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct gts_data *drv_data = dev->driver_data;

	return adc_read(drv_data->adc, &drv_data->adc_table);
}

static int gts_channel_get(struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gts_data *drv_data = dev->driver_data;
	uint16_t analog_val;
	double dval;

	/* rescale sample from 12bit (Zephyr) to 10bit (Grove) */
	analog_val = ((uint16_t)drv_data->adc_buffer[1] << 8) |
		     drv_data->adc_buffer[0];
	analog_val = analog_val >> 2;

	/*
	 * The formula for converting the analog value to degrees Celisus
	 * is taken from the sensor reference page:
	 *     http://www.seeedstudio.com/wiki/Grove_-_Temperature_Sensor
	 */
	dval = 1 / (log(1023.0 / analog_val - 1.0) / B_CONST +
		    1 / 298.15) - 273.15;

	val->val1 = (int32_t)dval;
	val->val2 = ((int32_t)(dval * 1000000)) % 1000000;

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

	drv_data->sample.sampling_delay = 12;
	drv_data->sample.channel_id =
		CONFIG_GROVE_TEMPERATURE_SENSOR_ADC_CHANNEL;
	drv_data->sample.buffer = drv_data->adc_buffer;
	drv_data->sample.buffer_length = 4;

	drv_data->adc_table.entries = &drv_data->sample;
	drv_data->adc_table.num_entries = 1;

	adc_enable(drv_data->adc);

	dev->driver_api = &gts_api;

	return 0;
}

static struct gts_data gts_data;

DEVICE_INIT(gts_dev, CONFIG_GROVE_TEMPERATURE_SENSOR_NAME, &gts_init, &gts_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
