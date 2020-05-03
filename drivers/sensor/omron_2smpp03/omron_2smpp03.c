/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <devicetree/adc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(omron_2smpp03, CONFIG_SENSOR_LOG_LEVEL);

/** ADC gain. */
#define ADC_GAIN ADC_GAIN_1
/** ADC reference. */
#define ADC_REF ADC_REF_INTERNAL
/** ADC acquisition time. */
#define ADC_ACQT ADC_ACQ_TIME_DEFAULT
/** ADC resolution. */
#define ADC_RES CONFIG_OMRON_2SMPP03_ADC_RESOLUTION

/** 2SMPP03 offset: 2.5mV (uV) */
#define SENSOR_OFFSET_UV 2500
/** 2SMPP03 pressure span: -50kPa to 50kPa (Pa). */
#define SENSOR_SPAN_PA 100000
/** 2SMPP03 voltage span: -43mV to 42mV (uV). */
#define SENSOR_SPAN_UV 85000

/** Omron 2SMPP03 data. */
struct omron_2smpp03_data {
	/** ADC device. */
	struct device *adc;
	/** ADC sequence. */
	struct adc_sequence seq;
	/** ADC raw data (buffer). */
	u16_t raw;
};

/** Omron 2SMPP03 configuration. */
struct omron_2smpp03_config {
	/** ADC label. */
	const char *adc_label;
	/** ADC channel. */
	u8_t adc_channel;
	/** Amplifier gain. */
	s32_t amplifier_gain;
	/** Amplifier offset (mV). */
	s32_t amplifier_offset;
};

static inline struct omron_2smpp03_data *to_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct omron_2smpp03_config *to_config(struct device *dev)
{
	return dev->config->config_info;
}

static int omron_2smpp03_sample_fetch(struct device *dev,
				      enum sensor_channel chan)
{
	struct omron_2smpp03_data *data = to_data(dev);

	return adc_read(data->adc, &data->seq);
}

static int omron_2smpp03_channel_get(struct device *dev,
				     enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct omron_2smpp03_data *data = to_data(dev);
	const struct omron_2smpp03_config *cfg = to_config(dev);

	s32_t ref_mv, val_uv, val_pa;

	if (chan != SENSOR_CHAN_GAUGE_PRESS) {
		return -ENOTSUP;
	}

	ref_mv = (s32_t)adc_ref_internal(data->adc);

	val_uv = (s32_t)data->raw;
	adc_raw_to_millivolts(ref_mv, ADC_GAIN, ADC_RES, &val_uv);
	val_uv *= 1000;

	/* compensate amplifier gain and offset */
	val_uv = (val_uv - cfg->amplifier_offset * 1000) / cfg->amplifier_gain;

	val_pa = ((s64_t)(val_uv - SENSOR_OFFSET_UV) * SENSOR_SPAN_PA) /
		 SENSOR_SPAN_UV;

	val->val1 = val_pa / 1000;
	val->val2 = val_pa - val->val1 * 1000;

	return 0;
}

static const struct sensor_driver_api omron_2smpp03_api = {
	.sample_fetch = &omron_2smpp03_sample_fetch,
	.channel_get = &omron_2smpp03_channel_get,
};

static int omron_2smpp03_init(struct device *dev)
{
	struct omron_2smpp03_data *data = to_data(dev);
	const struct omron_2smpp03_config *cfg = to_config(dev);

	int r;
	struct adc_channel_cfg adc_cfg = { 0 };

	data->adc = device_get_binding(cfg->adc_label);
	if (!data->adc) {
		LOG_ERR("Could not obtain ADC device");
		return -ENODEV;
	}

	adc_cfg.gain = ADC_GAIN;
	adc_cfg.reference = ADC_REF;
	adc_cfg.acquisition_time = ADC_ACQT;
	adc_cfg.channel_id = cfg->adc_channel;

	r = adc_channel_setup(data->adc, &adc_cfg);
	if (r < 0) {
		LOG_ERR("Could not configure ADC cannel (%d)", r);
		return r;
	}

	data->seq.buffer = &data->raw;
	data->seq.buffer_size = sizeof(data->raw);
	data->seq.channels = BIT(cfg->adc_channel);
	data->seq.resolution = ADC_RES;

	return 0;
}

#define OMRON_2SMPP03_INIT(index)                                              \
	static struct omron_2smpp03_data omron_2smpp03_data_##index;           \
									       \
	static const struct omron_2smpp03_config omron_2smpp03_cfg_##index = { \
		.adc_label = DT_INST_IO_CHANNELS_LABEL(index),                 \
		.adc_channel = DT_INST_IO_CHANNELS_INPUT(index),               \
		.amplifier_gain = DT_INST_PROP(index, amplifier_gain),         \
		.amplifier_offset = DT_INST_PROP(index, amplifier_offset)      \
	};                                                                     \
									       \
	DEVICE_AND_API_INIT(omron_2smpp03_##index, DT_INST_LABEL(index),       \
			    &omron_2smpp03_init, &omron_2smpp03_data_##index,  \
			    &omron_2smpp03_cfg_##index, POST_KERNEL,           \
			    CONFIG_SENSOR_INIT_PRIORITY, &omron_2smpp03_api)

#define DT_DRV_COMPAT omron_2smpp03
DT_INST_FOREACH(OMRON_2SMPP03_INIT)
