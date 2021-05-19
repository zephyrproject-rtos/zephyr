/*
 * Copyright (c) 2021 Eug Krashtan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(stm32_temp, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_temp

struct stm32_temp_data {
	const struct device *adc;
	uint8_t channel;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int32_t mv; /* Sensor value in millivolts */
};

struct stm32_temp_config {
	int avgslope;
	int v25_mv;
	int tsv_mv;
	bool is_ntc;
};

static int stm32_temp_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	const struct stm32_temp_config *cfg = dev->config;
	struct stm32_temp_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = adc_read(data->adc, sp);
	sp->calibrate = false;
	if (rc == 0) {
		data->mv = data->sample_buffer * cfg->tsv_mv /	0x0FFF;
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int stm32_temp_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct stm32_temp_data *data = dev->data;
	const struct stm32_temp_config *cfg = dev->config;
	float temp;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	if (cfg->is_ntc) {
		temp = (float)(cfg->v25_mv - data->mv);
	} else {
		temp = (float)(data->mv - cfg->v25_mv);
	}
	temp = (temp/cfg->avgslope)*10;
	temp += 25;
	sensor_value_from_double(val, temp);

	return 0;
}

static const struct sensor_driver_api stm32_temp_driver_api = {
	.sample_fetch = stm32_temp_sample_fetch,
	.channel_get = stm32_temp_channel_get,
};

static int stm32_temp_init(const struct device *dev)
{
	struct stm32_temp_data *data = dev->data;
	struct adc_channel_cfg *accp = &data->adc_cfg;
	struct adc_sequence *asp = &data->adc_seq;
	int rc;

	k_mutex_init(&data->mutex);

	if (!device_is_ready(data->adc)) {
		LOG_ERR("Device %s is not ready", data->adc->name);
		return -ENODEV;
	}

	*accp = (struct adc_channel_cfg){
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME_MAX,
		.channel_id = data->channel,
		.differential = 0
	};
	rc = adc_channel_setup(data->adc, accp);
	LOG_DBG("Setup AIN%u got %d", data->channel, rc);

	*asp = (struct adc_sequence){
		.channels = BIT(data->channel),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12,
		.calibrate = true,
	};

	return 0;
}

#define STM32_TEMP_INST(idx)    \
	static struct stm32_temp_data inst_##idx##_data = {  \
		.adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(  \
						DT_INST(idx, st_stm32_temp))), \
		.channel = DT_IO_CHANNELS_INPUT( \
						DT_INST(idx, st_stm32_temp)) \
	};  \
	static const struct stm32_temp_config inst_##idx##_config = { \
		.avgslope = DT_INST_PROP(idx, avgslope),  \
		.v25_mv = DT_INST_PROP(idx, v25),  \
		.tsv_mv = DT_INST_PROP(idx, ts_voltage_mv),  \
		.is_ntc = DT_INST_PROP(idx, ntc)  \
	};  \
	DEVICE_DT_INST_DEFINE(idx,  \
		    stm32_temp_init,    \
		    NULL,               \
		    &inst_##idx##_data, \
			&inst_##idx##_config,  \
		    POST_KERNEL,           \
		    CONFIG_SENSOR_INIT_PRIORITY, \
		    &stm32_temp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_TEMP_INST)
