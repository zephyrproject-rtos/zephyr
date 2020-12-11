/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_temperature

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(temp_kinetis, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Driver assumptions:
 * - ADC samples are in uint16_t format
 * - Both ADC channels (sensor and bandgap) are on the same ADC instance
 *
 * See NXP Application Note AN3031 for details on calculations.
 */

/* Two ADC samples required for each reading, sensor value and bandgap value */
#define TEMP_KINETIS_ADC_SAMPLES 2

struct temp_kinetis_config {
	const char *adc_dev_name;
	uint8_t sensor_adc_ch;
	uint8_t bandgap_adc_ch;
	int bandgap_mv;
	int vtemp25_mv;
	int slope_cold_uv;
	int slope_hot_uv;
	struct adc_sequence adc_seq;
};

struct temp_kinetis_data {
	const struct device *adc;
	uint16_t buffer[TEMP_KINETIS_ADC_SAMPLES];
};

static int temp_kinetis_sample_fetch(const struct device *dev,
				     enum sensor_channel chan)
{
	const struct temp_kinetis_config *config = dev->config;
	struct temp_kinetis_data *data = dev->data;
#ifdef CONFIG_TEMP_KINETIS_FILTER
	uint16_t previous[TEMP_KINETIS_ADC_SAMPLES];
	int i;
#endif /* CONFIG_TEMP_KINETIS_FILTER */
	int err;

	/* Always read both sensor and bandgap voltage in one go */
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP &&
	    chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

#ifdef CONFIG_TEMP_KINETIS_FILTER
	memcpy(previous, data->buffer, sizeof(previous));
#endif /* CONFIG_TEMP_KINETIS_FILTER */

	err = adc_read(data->adc, &config->adc_seq);
	if (err) {
		LOG_ERR("failed to read ADC channels (err %d)", err);
		return err;
	}

	LOG_DBG("sensor = %d, bandgap = %d", data->buffer[0], data->buffer[1]);

#ifdef CONFIG_TEMP_KINETIS_FILTER
	if (previous[0] != 0 && previous[1] != 0) {
		for (i = 0; i < ARRAY_SIZE(previous); i++) {
			data->buffer[i] = (data->buffer[i] >> 1) +
				(previous[i] >> 1);
		}

		LOG_DBG("sensor = %d, bandgap = %d (filtered)", data->buffer[0],
			data->buffer[1]);
	}
#endif /* CONFIG_TEMP_KINETIS_FILTER */

	return 0;
}

static int temp_kinetis_channel_get(const struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct temp_kinetis_config *config = dev->config;
	struct temp_kinetis_data *data = dev->data;
	uint16_t adcr_vdd = BIT_MASK(config->adc_seq.resolution);
	uint16_t adcr_temp25;
	int32_t temp_cc;
	int32_t vdd_mv;
	int slope_uv;
	uint16_t adcr_100m;

	if (chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* VDD (or VREF, but AN3031 calls it VDD) in millivolts */
	vdd_mv = (adcr_vdd * config->bandgap_mv) / data->buffer[1];

	if (chan == SENSOR_CHAN_VOLTAGE) {
		val->val1 = vdd_mv / 1000;
		val->val2 = (vdd_mv % 1000) * 1000;
		return 0;
	}

	/* ADC result for temperature = 25 degrees Celsius */
	adcr_temp25 = (adcr_vdd * config->vtemp25_mv) / vdd_mv;

	/* Determine which slope to use */
	if (data->buffer[0] > adcr_temp25) {
		slope_uv = config->slope_cold_uv;
	} else {
		slope_uv = config->slope_hot_uv;
	}

	adcr_100m = (adcr_vdd * slope_uv) / (vdd_mv * 10);

	/* Temperature in centi degrees Celsius */
	temp_cc = 2500 -
		(((data->buffer[0] - adcr_temp25) * 10000) / adcr_100m);

	val->val1 = temp_cc / 100;
	val->val2 = (temp_cc % 100) * 10000;

	return 0;
}

static const struct sensor_driver_api temp_kinetis_driver_api = {
	.sample_fetch = temp_kinetis_sample_fetch,
	.channel_get = temp_kinetis_channel_get,
};

static int temp_kinetis_init(const struct device *dev)
{
	const struct temp_kinetis_config *config = dev->config;
	struct temp_kinetis_data *data = dev->data;
	int err;
	int i;
	const struct adc_channel_cfg ch_cfg[] = {
		{
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = config->sensor_adc_ch,
			.differential = 0,
		},
		{
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
			.channel_id = config->bandgap_adc_ch,
			.differential = 0,
		},
	};

	memset(&data->buffer, 0, ARRAY_SIZE(data->buffer));

	data->adc = device_get_binding(config->adc_dev_name);
	if (!data->adc) {
		LOG_ERR("could not get ADC device");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(ch_cfg); i++) {
		err = adc_channel_setup(data->adc, &ch_cfg[i]);
		if (err) {
			LOG_ERR("failed to configure ADC channel (err %d)",
				err);
			return err;
		}
	}

	return 0;
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported temp instance");

#define TEMP_KINETIS_INIT(inst)						\
	BUILD_ASSERT(DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, sensor) <	\
		     DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, bandgap),	\
		     "This driver assumes sensor ADC channel to come before "\
		     "bandgap ADC channel");				\
									\
	static struct temp_kinetis_data temp_kinetis_data_0;		\
									\
	static const struct temp_kinetis_config temp_kinetis_config_0 = {\
		.adc_dev_name =						\
			DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, 0),	\
		.sensor_adc_ch =					\
			DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, sensor),\
		.bandgap_adc_ch =					\
			DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, bandgap),\
		.bandgap_mv = DT_INST_PROP(0, bandgap_voltage) / 1000,	\
		.vtemp25_mv = DT_INST_PROP(0, vtemp25) / 1000,		\
		.slope_cold_uv = DT_INST_PROP(0, sensor_slope_cold),	\
		.slope_hot_uv = DT_INST_PROP(0, sensor_slope_hot),	\
		.adc_seq = {						\
			.options = NULL,				\
			.channels =					\
		BIT(DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, sensor)) |	\
		BIT(DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, bandgap)),	\
			.buffer = &temp_kinetis_data_0.buffer,		\
			.buffer_size = sizeof(temp_kinetis_data_0.buffer),\
			.resolution = CONFIG_TEMP_KINETIS_RESOLUTION,	\
			.oversampling = CONFIG_TEMP_KINETIS_OVERSAMPLING,\
			.calibrate = false,				\
		},							\
	};								\
									\
	DEVICE_AND_API_INIT(temp_kinetis, DT_INST_LABEL(inst),		\
			    temp_kinetis_init, &temp_kinetis_data_0,	\
			    &temp_kinetis_config_0, POST_KERNEL,	\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &temp_kinetis_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TEMP_KINETIS_INIT)
