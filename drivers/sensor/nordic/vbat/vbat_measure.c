/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vbat

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_vbat, CONFIG_SENSOR_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "multiple instances not supported");

struct vbat_config {
	struct adc_dt_spec adc;
	int32_t min_threshold_mv;
	bool invert_voltage;
};

struct vbat_data {
	struct adc_sequence sequence;
	int16_t raw;
};

static int vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct vbat_config *cfg = dev->config;
	struct vbat_data *data = dev->data;
	int err = 0;

	if ((chan != SENSOR_CHAN_VOLTAGE) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	err = adc_read(cfg->adc.dev, &data->sequence);
	if (err != 0) {
		LOG_ERR("adc_read failed: %d", err);
		return err;
	}

	return err;
}

static int vbat_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	const struct vbat_config *cfg = dev->config;
	struct vbat_data *data = dev->data;
	int32_t val_mv;
	int err;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	val_mv = data->raw;

	err = adc_raw_to_millivolts_dt(&cfg->adc, &val_mv);
	if (err != 0) {
		LOG_ERR("adc_raw_to_millivolts_dt failed: %d", err);
		return err;
	}
	if (cfg->invert_voltage) {
		val_mv = -val_mv;
	}

	LOG_DBG("raw %" PRIu16 ", %" PRIi32 " uV", data->raw, val_mv);
	return sensor_value_from_milli(val, val_mv);
}

static DEVICE_API(sensor, vbat_driver_api) = {
	.sample_fetch = vbat_sample_fetch,
	.channel_get = vbat_channel_get,
};

static int vbat_init(const struct device *dev)
{
	const struct vbat_config *cfg = dev->config;
	struct vbat_data *data = dev->data;
	int err;

	if (!adc_is_ready_dt(&cfg->adc)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc);
	if (err != 0) {
		LOG_ERR("adc_channel_setup_dt (init) failed: %d", err);
		return err;
	}

	err = adc_sequence_init_dt(&cfg->adc, &data->sequence);
	if (err != 0) {
		LOG_ERR("adc_sequence_init_dt failed: %d", err);
		return err;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);
	data->sequence.calibrate = true;

	return 0;
}

#define NRF_VBAT_INIT(inst)                                                                        \
	static struct vbat_data vbat_data_##inst;                                                  \
	static const struct vbat_config vbat_cfg_##inst = {                                        \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                                 \
		.invert_voltage = DT_INST_PROP_OR(inst, invert_voltage, false),                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, vbat_init, NULL, &vbat_data_##inst, &vbat_cfg_##inst,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &vbat_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NRF_VBAT_INIT)
