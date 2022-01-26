/*
 * Copyright (c) 2021 Eug Krashtan
 * Copyright (c) 2022 Wouter Cappelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(stm32_temp, CONFIG_SENSOR_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp)
#define DT_DRV_COMPAT st_stm32_temp
#define HAS_CALIBRATION 0
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp_cal)
#define DT_DRV_COMPAT st_stm32_temp_cal
#define HAS_CALIBRATION 1
#else
#error "No compatible devicetree node found"
#endif

struct stm32_temp_data {
	const struct device *adc;
	uint8_t channel;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

struct stm32_temp_config {
	int tsv_mv;
#if HAS_CALIBRATION
	uint16_t *cal1_addr;
	uint16_t *cal2_addr;
	int cal1_temp;
	int cal2_temp;
	int cal_vrefanalog;
	int cal_offset;
#else
	int avgslope;
	int v25_mv;
	bool is_ntc;
#endif
};

static int stm32_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
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
		data->raw = data->sample_buffer;
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int stm32_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_temp_data *data = dev->data;
	const struct stm32_temp_config *cfg = dev->config;
	float temp;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

#if HAS_CALIBRATION
	temp = ((float)data->raw * cfg->tsv_mv) / cfg->cal_vrefanalog;
	temp -= *cfg->cal1_addr;
	temp *= (cfg->cal2_temp - cfg->cal1_temp);
	temp /= (*cfg->cal2_addr - *cfg->cal1_addr);
	temp += cfg->cal_offset;
#else
	int32_t mv = data->raw * cfg->tsv_mv / 0x0FFF; /* Sensor value in millivolts */

	if (cfg->is_ntc) {
		temp = (float)(cfg->v25_mv - mv);
	} else {
		temp = (float)(mv - cfg->v25_mv);
	}
	temp = (temp / cfg->avgslope) * 10;
	temp += 25;
#endif

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

	*accp = (struct adc_channel_cfg){ .gain = ADC_GAIN_1,
					  .reference = ADC_REF_INTERNAL,
					  .acquisition_time = ADC_ACQ_TIME_MAX,
					  .channel_id = data->channel,
					  .differential = 0 };
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

static const struct stm32_temp_config stm32_temp_dev_config = {
	.tsv_mv = DT_INST_PROP(0, ts_voltage_mv),
#if HAS_CALIBRATION
	.cal1_addr = (uint16_t *)DT_INST_PROP(0, ts_cal1_addr),
	.cal2_addr = (uint16_t *)DT_INST_PROP(0, ts_cal2_addr),
	.cal1_temp = DT_INST_PROP(0, ts_cal1_temp),
	.cal2_temp = DT_INST_PROP(0, ts_cal2_temp),
	.cal_vrefanalog = DT_INST_PROP(0, ts_cal_vrefanalog),
	.cal_offset = DT_INST_PROP(0, ts_cal_offset)
#else
	.avgslope = DT_INST_PROP(0, avgslope),
	.v25_mv = DT_INST_PROP(0, v25),
	.is_ntc = DT_INST_PROP(0, ntc)
#endif
};

static struct stm32_temp_data stm32_temp_dev_data = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.channel = DT_INST_IO_CHANNELS_INPUT(0),
};

DEVICE_DT_INST_DEFINE(0, stm32_temp_init, NULL, &stm32_temp_dev_data, &stm32_temp_dev_config,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &stm32_temp_driver_api);
