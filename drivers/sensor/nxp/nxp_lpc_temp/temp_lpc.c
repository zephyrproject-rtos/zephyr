/*
 * Copyright (c) 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_temperature

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(temp_lpc, CONFIG_SENSOR_LOG_LEVEL);

struct temp_lpc_config {
	const struct device *adc;
	uint8_t sensor_adc_ch;
	struct adc_sequence adc_seq;
};

struct temp_lpc_data {
	uint16_t buffer[FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE];
	struct k_poll_signal adc_signal;
};

static struct temp_lpc_data temp_lpc_data;

static const struct temp_lpc_config temp_lpc_config = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.sensor_adc_ch = DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0),
	.adc_seq = {
		.options = NULL,
		.channels = BIT(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0)),
		.buffer = &temp_lpc_data.buffer,
		.buffer_size = sizeof(temp_lpc_data.buffer),
		.resolution = 16,
		.oversampling = 7,
		.calibrate = false,
	},
};

static int temp_lpc_sample_fetch(const struct device *dev,
					 enum sensor_channel chan)
{
	const struct temp_lpc_config *config = dev->config;
	struct temp_lpc_data *data = dev->data;
	struct k_poll_event adc_event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
		K_POLL_MODE_NOTIFY_ONLY, &data->adc_signal);

	int signaled;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_poll_signal_reset(&data->adc_signal);

	ret = adc_read_async(config->adc, &config->adc_seq, &data->adc_signal);
	if (ret) {
		LOG_ERR("failed to read ADC channels (%d)", ret);
		return ret;
	}

	ret = k_poll(&adc_event, 1, K_SECONDS(1));
	if(ret < 0) {
		return ret;
	}

	k_poll_signal_check(&data->adc_signal, &signaled, &ret);
	if (!signaled) {
		return -EAGAIN;
	}

	return 0;
}

static int temp_lpc_channel_get(const struct device *dev,
					enum sensor_channel chan,
					struct sensor_value *val)
{
	struct temp_lpc_data *data = dev->data;

	uint16_t Vbe1 = 0U;
	uint16_t Vbe8 = 0U;
	uint32_t convResultShift = 0U;
	float slope;
	float offset;
	float temperature;

	if (chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	// If the temperature sensor needs calibration, then read the calibration from the flash.
	uint32_t calibrated_slope  = (*((volatile uint32_t *)(FSL_FEATURE_FLASH_NMPA_TEMP_SLOPE_ADDRS)));
	uint32_t calibrated_offset = (*((volatile uint32_t *)(FSL_FEATURE_FLASH_NMPA_TEMP_OFFSET_ADDRS)));

	if (((calibrated_slope & 0x1UL) != 0UL) && ((calibrated_offset & 0x1UL) != 0UL)) {
		// Re-justify slope value and offset value based on the calibration value.
		slope  = ((float)(uint32_t)(calibrated_slope >> 1UL) / 1024.0f);
		offset = ((float)(uint32_t)(calibrated_offset >> 1UL) / 1024.0f);
	}
	else {
		// Otherwise, use the default slope and offset value.
		slope = FSL_FEATURE_LPADC_TEMP_PARAMETER_A;
		offset = FSL_FEATURE_LPADC_TEMP_PARAMETER_B;
	}

#if defined(FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE) && (FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE == 4U)
	// The LOOP Count should be 4, but the first two results are useless.
	Vbe1 = data->buffer[2] >> convResultShift;
	Vbe8 = data->buffer[3] >> convResultShift;
#else
	// Read the 2 temperature sensor result.
	Vbe1 = data->buffer[0] >> convResultShift;
	Vbe8 = data->buffer[1] >> convResultShift;
#endif

	// Final temperature = A*[alpha*(Vbe8-Vbe1)/(Vbe8 + alpha*(Vbe8-Vbe1))] - B.
	temperature = slope * (FSL_FEATURE_LPADC_TEMP_PARAMETER_ALPHA * ((float)Vbe8 - (float)Vbe1) /
			((float)Vbe8 + FSL_FEATURE_LPADC_TEMP_PARAMETER_ALPHA * ((float)Vbe8 - (float)Vbe1))) - offset;

	return sensor_value_from_float(val, temperature);
}

static DEVICE_API(sensor, temp_lpc_driver_api) = {
	.sample_fetch = temp_lpc_sample_fetch,
	.channel_get = temp_lpc_channel_get,
};

static int temp_lpc_init(const struct device *dev)
{
	const struct temp_lpc_config *config = dev->config;
	struct temp_lpc_data *data = dev->data;
	int ret;

	const struct adc_channel_cfg ch_cfg = {
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 131),
		.channel_id = config->sensor_adc_ch,
		.differential = 1,
		.input_positive = 26,
		.input_negative = 26,
	};

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device is not ready");
		return -EINVAL;
	}

	ret = adc_channel_setup(config->adc, &ch_cfg);
	if (ret) {
		LOG_ERR("failed to configure ADC channel (%d)", ret);
		return ret;
	}

	k_poll_signal_init(&data->adc_signal);

	return 0;
}

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
		 "only one instance is supported");

SENSOR_DEVICE_DT_INST_DEFINE(0, temp_lpc_init, NULL, &temp_lpc_data,
			&temp_lpc_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			&temp_lpc_driver_api);
