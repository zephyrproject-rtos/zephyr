/*
 * Copyright (c) 2023 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include <hardware/adc.h>

LOG_MODULE_REGISTER(rpi_pico_temp, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_temp

struct rpi_pico_temp_config {
	const struct device *adc;
	const struct adc_channel_cfg ch_cfg;
	const int32_t vbe;
	const int32_t vbe_slope;
};

struct rpi_pico_temp_data {
	struct adc_sequence adc_seq;
	int16_t sample;
};

static int rpi_pico_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct rpi_pico_temp_config *cfg = dev->config;
	struct rpi_pico_temp_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	rc = adc_channel_setup(cfg->adc, &cfg->ch_cfg);
	if (rc) {
		LOG_DBG("Setup ADC channel %u failed with %d", cfg->ch_cfg.channel_id, rc);
		return rc;
	}

	rc = adc_read(cfg->adc, &data->adc_seq);

	return rc;
}

static int rpi_pico_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				     struct sensor_value *val)
{
	const struct rpi_pico_temp_config *cfg = dev->config;
	struct rpi_pico_temp_data *data = dev->data;
	int32_t mv = 0;
	int32_t work;
	int rc;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	mv = data->sample;
	rc = adc_raw_to_millivolts(adc_ref_internal(cfg->adc), cfg->ch_cfg.gain,
				   data->adc_seq.resolution, &mv);
	if (rc) {
		LOG_DBG("adc_raw_to_millivolts() failed %d", rc);
		return rc;
	}

	/*
	 * Calculate CPU temperature from voltage by the equation:
	 * T = 27 - (ADC_Voltage - 0.706)/0.001721
	 */
	work = ((27 * (-cfg->vbe_slope)) - (mv * 1000 - cfg->vbe));
	val->val1 = work / (-cfg->vbe_slope);
	work -= val->val1 * (-cfg->vbe_slope);
	val->val2 = work * 1000000 / (-cfg->vbe_slope);

	return 0;
}

static const struct sensor_driver_api rpi_pico_temp_driver_api = {
	.sample_fetch = rpi_pico_temp_sample_fetch,
	.channel_get = rpi_pico_temp_channel_get,
};

static int rpi_pico_temp_init(const struct device *dev)
{
	const struct rpi_pico_temp_config *cfg = dev->config;

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("Device %s is not ready", cfg->adc->name);
		return -ENODEV;
	}

	adc_set_temp_sensor_enabled(true);

	return 0;
}

#define RPI_PICO_TEMP_DEFINE(inst)                                                                 \
	static const struct rpi_pico_temp_config rpi_pico_temp_config_##inst = {                   \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.vbe = DT_INST_PROP(inst, vbe),                                                    \
		.vbe_slope = DT_INST_PROP(inst, vbe_slope),                                        \
		.ch_cfg =                                                                          \
			{                                                                          \
				.gain = ADC_GAIN_1,                                                \
				.reference = ADC_REF_INTERNAL,                                     \
				.acquisition_time = ADC_ACQ_TIME_DEFAULT,                          \
				.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                     \
				.differential = 0,                                                 \
			},                                                                         \
	};                                                                                         \
	static struct rpi_pico_temp_data rpi_pico_temp_dev_data_##inst = {                         \
		.adc_seq =                                                                         \
			{                                                                          \
				.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)),                  \
				.buffer = &rpi_pico_temp_dev_data_##inst.sample,                   \
				.buffer_size = sizeof(rpi_pico_temp_dev_data_##inst.sample),       \
				.resolution = 12U,                                                 \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, rpi_pico_temp_init, NULL,                               \
				     &rpi_pico_temp_dev_data_##inst, &rpi_pico_temp_config_##inst, \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &rpi_pico_temp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_PICO_TEMP_DEFINE)
