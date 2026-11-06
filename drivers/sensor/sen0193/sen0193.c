/*
 * Copyright (c) 2026 J. Nishanth Raj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT dfrobot_sen0193

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(SEN0913,CONFIG_SENSOR_LOG_LEVEL);

struct sen0913_config {
	const struct adc_dt_spec adc_channel;
	uint16_t air_mv;
	uint16_t water_mv;
};

struct sen0913_data {
	uint16_t raw_data;
};

static int sen0913_init(const struct device *dev)
{
	const struct sen0913_config *config = dev->config;
	int err;

	if (!adc_is_ready_dt(dev->adc_channel)) {
		LOG_ERR("ADC controller not ready");
		return -ENODEV;
	}
	err = adc_channel_setup_dt(&config->adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup ADC channel (%d)", err);
		return err;
	}

	LOG_INF("SEN0193 initialized successfully on ADC channel");
	return 0;

}

/*
 * Routine: sen0193_sample_fetch
 * Purpose: Commands the Zephyr ADC subsystem to lock the multiplexer,
 *          take a physical reading from the sensor pin, and cache the raw
 *          millivolt value in the driver's private RAM.
 */
static int sen0193_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	const struct sen0913_config *config = dev->config;
	struct sen0193_data *data = dev->data;

	int err;
	uint16_t adc_buf;

	struct adc_sequence seq = {
		.buffer = adc_buf,
		.buffer_size = sizeof(adc_buf)
	};

	adc_sequence_init_dt(&config->adc_channel, &seq);

	err = adc_read_dt(&config->adc_channel, &seq);

	if (err < 0) {
		LOG_ERR("ADC read failed (%d)", err);
		return err;
	}

	int32_t raw_val = adc_buffer;

	err = adc_raw_to_millivolts_dt(&config->adc_channel, &raw_val);
	if (err < 0) {
		LOG_WRN("ADC to mV conversion failed, using raw value");
	}

	data->raw_data = (uint16_t)raw_val;

	return 0;
}

/*
 * Routine: sen0193_channel_get
 * Purpose: Retrieves the cached millivolt reading and converts it to a 0-100%
 *          moisture value.
 * Note:    This uses strict integer arithmetic to avoid FPU overhead. Analog
 *          fluctuations are clamped at the Devicetree calibration boundaries.
 */
static int sen0193_channel_get(const struct device *dev, enum sensor_channel channel,
			       struct sensor_value *val)
{
	const struct sen0193_config *config = dev->config;
	struct sen0193_data *data = dev->data;

	if (chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	int32_t raw_mv = data->raw_mv;
	int32_t air_mv = config->air_mv;
	int32_t water_mv = config->water_mv;
	int32_t moisture_percent;

	if (raw_mv >= air_mv) {
		moisture_percent = 0;
	} else if (raw_mv <= water_mv) {
		moisture_percent = 100;
	} else {
		moisture_percent = ((air_mv - raw_mv) * 100) / (air_mv - water_mv);
	}

	val->val1 = moisture_percent;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, sen0193_api) = {
	.sample_fetch = sen0193_sample_fetch,
	.channel_get = sen0193_channel_get,
};

#define SENSOR_INIT(inst)							\
	const struct sen0193_data sen0193_data##inst;				\
	const struct sen0913_config sen0913_config##inst = {			\
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),			\
		.air_mv = DT_INST_PROP(inst, air_volatge_mv),			\
		.water_mv = DT_INST_PROP(inst, water_voltage_mv)		\
	};									\
	DEVICE_DT_INST_DEFINE(inst, sen0193_init, NULL,				\
			      &sen0193_data_##inst, &sen0193_cfg_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
			      &api);
DT_INST_FOREACH_STATUS_OKAY(SENSOR_INIT);
