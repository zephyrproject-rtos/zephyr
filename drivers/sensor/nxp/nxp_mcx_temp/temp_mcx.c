/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcx_temperature

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(temp_mcx, CONFIG_SENSOR_LOG_LEVEL);

/* Four ADC samples: first two are dropped, then VBE1 and VBE8 */
#define TEMP_MCX_ADC_SAMPLES 1

struct temp_mcx_config {
	const struct device *adc;
	struct adc_sequence adc_seq;
	struct adc_channel_cfg ch_cfg;
};

struct temp_mcx_data {
	float temperature;
	uint16_t buffer[TEMP_MCX_ADC_SAMPLES];
};

static int temp_mcx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct temp_mcx_config *config = dev->config;
	struct temp_mcx_data *data = dev->data;
	int err;
	uint16_t vbe1 = 0U;
	uint16_t vbe8 = 0U;
	uint32_t conv_result_shift = 0U;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

#if defined(FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE) &&                                            \
	(FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE == 4U)
	/* The firsttwo results is useless. */
	adc_read(config->adc, &config->adc_seq);
	adc_read(config->adc, &config->adc_seq);
#endif /* FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE */

	err = adc_read(config->adc, &config->adc_seq);
	if (err) {
		LOG_ERR("Failed to read ADC channels (err %d)", err);
		return err;
	}
	vbe1 = data->buffer[0] >> conv_result_shift;

	err = adc_read(config->adc, &config->adc_seq);
	if (err) {
		LOG_ERR("Failed to read ADC channels (err %d)", err);
		return err;
	}
	vbe8 = data->buffer[0] >> conv_result_shift;

	/* Calculate temperature using exact MCX formula */
	data->temperature =
		FSL_FEATURE_LPADC_TEMP_PARAMETER_A *
			(FSL_FEATURE_LPADC_TEMP_PARAMETER_ALPHA * ((float)vbe8 - (float)vbe1) /
			 ((float)vbe8 +
			  FSL_FEATURE_LPADC_TEMP_PARAMETER_ALPHA * ((float)vbe8 - (float)vbe1))) -
		FSL_FEATURE_LPADC_TEMP_PARAMETER_B;

	LOG_DBG("VBE1=%d VBE8=%d Temp=%.3f", vbe1, vbe8, (double)data->temperature);
	return 0;
}

static int temp_mcx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct temp_mcx_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = (int32_t)data->temperature;
	val->val2 = (int32_t)((data->temperature - (float)val->val1) * 1000000);

	return 0;
}

static const struct sensor_driver_api temp_mcx_driver_api = {
	.sample_fetch = temp_mcx_sample_fetch,
	.channel_get = temp_mcx_channel_get,
};

static int temp_mcx_init(const struct device *dev)
{
	const struct temp_mcx_config *config = dev->config;
	int err;

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	err = adc_channel_setup(config->adc, &config->ch_cfg);
	if (err) {
		LOG_ERR("Failed to setup ADC channel (err %d)", err);
		return err;
	}

	return 0;
}

#define TEMP_MCX_INIT(inst)                                                                        \
	static struct temp_mcx_data temp_mcx_data_##inst = {                                       \
		.buffer = {0},                                                                     \
		.temperature = -273.15f,                                                           \
	};                                                                                         \
                                                                                                   \
	/* 获取 ADC 设备和通道 ID */                                                        \
	static const uint8_t channel_id_##inst = DT_REG_ADDR(DT_INST_PARENT(inst));                \
	static const uint8_t gain_##inst = DT_ENUM_IDX(DT_INST_PARENT(inst), zephyr_gain);         \
	static const uint8_t reference_##inst =                                                    \
		DT_ENUM_IDX(DT_INST_PARENT(inst), zephyr_reference);                               \
	static const uint16_t acq_time_##inst = ADC_ACQ_TIME(                                      \
		ADC_ACQ_TIME_TICKS, DT_PROP(DT_INST_PARENT(inst), zephyr_acquisition_time));       \
	static const uint8_t input_pos_##inst =                                                    \
		DT_PROP(DT_INST_PARENT(inst), zephyr_input_positive);                              \
	static const uint8_t resolution_##inst = DT_PROP(DT_INST_PARENT(inst), zephyr_resolution); \
	static const uint8_t oversampling_##inst =                                                 \
		DT_PROP(DT_INST_PARENT(inst), zephyr_oversampling);                                \
                                                                                                   \
	static const struct temp_mcx_config temp_mcx_config_##inst = {                             \
		.adc = DEVICE_DT_GET(DT_PARENT(DT_INST_PARENT(inst))),                             \
		.adc_seq =                                                                         \
			{                                                                          \
				.channels = BIT(channel_id_##inst),                                \
				.buffer = &temp_mcx_data_##inst.buffer,                            \
				.buffer_size = sizeof(temp_mcx_data_##inst.buffer),                \
				.resolution = resolution_##inst,                                   \
				.oversampling = oversampling_##inst,                               \
			},                                                                         \
		.ch_cfg = {.gain = gain_##inst,                                                    \
			   .reference = reference_##inst,                                          \
			   .acquisition_time = acq_time_##inst,                                    \
			   .channel_id = channel_id_##inst,                                        \
			   .differential = false,                                                  \
			   .input_positive = input_pos_##inst},                                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, temp_mcx_init, NULL, &temp_mcx_data_##inst,             \
				     &temp_mcx_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &temp_mcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TEMP_MCX_INIT)
