/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <adc.h>
#include <device.h>
#include <math.h>
#include <sensor.h>
#include <zephyr.h>

#define SYS_LOG_DOMAIN "GROVE_LIGHT_SENSOR"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_GROVE_LEVEL
#include <misc/sys_log.h>

struct gls_data {
	struct device *adc;
	struct adc_seq_entry sample;
	struct adc_seq_table adc_table;
	uint8_t adc_buffer[4];
};

static int gls_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct gls_data *drv_data = dev->driver_data;

	return adc_read(drv_data->adc, &drv_data->adc_table);
}

static int gls_channel_get(struct device *dev,
			   enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct gls_data *drv_data = dev->driver_data;
	uint16_t analog_val;
	double ldr_val;

	/* rescale sample from 12bit (Zephyr) to 10bit (Grove) */
	analog_val = ((uint16_t)drv_data->adc_buffer[1] << 8) |
		     drv_data->adc_buffer[0];
	analog_val = analog_val >> 2;

	/*
	 * The formula for converting the analog value to lux is taken from
	 * the UPM project:
	 *     https://github.com/intel-iot-devkit/upm/blob/master/src/grove/grove.cxx#L161
	 */
	ldr_val = (1023.0 - analog_val) * 10.0 / analog_val;
	val->type = SENSOR_VALUE_TYPE_DOUBLE;
	val->dval = 10000.0 / pow(ldr_val * 15.0, 4.0/3.0);

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
		SYS_LOG_ERR("Failed to get ADC device.");
		return -EINVAL;
	}

	drv_data->sample.sampling_delay = 12;
	drv_data->sample.channel_id = CONFIG_GROVE_LIGHT_SENSOR_ADC_CHANNEL;
	drv_data->sample.buffer = drv_data->adc_buffer;
	drv_data->sample.buffer_length = 4;

	drv_data->adc_table.entries = &drv_data->sample;
	drv_data->adc_table.num_entries = 1;

	adc_enable(drv_data->adc);

	dev->driver_api = &gls_api;

	return 0;
}

static struct gls_data gls_data;

DEVICE_INIT(gls_dev, CONFIG_GROVE_LIGHT_SENSOR_NAME, &gls_init, &gls_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
