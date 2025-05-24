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

#include <fsl_lpadc.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
		 "only one instance is supported");

struct temp_lpc_config {
	const struct device *adc;
	uint8_t sensor_adc_ch;
	struct adc_sequence adc_seq;
	const struct adc_channel_cfg adc_ch_cfg[];
};

struct temp_lpc_data {
	uint16_t buffer[FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE];
};

static struct temp_lpc_data inst_data;

static const struct temp_lpc_config inst_config = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.sensor_adc_ch = DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0),
	.adc_seq = {
		.options = NULL,
		.channels = BIT(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0)),
		.buffer = &inst_data.buffer,
		.buffer_size = sizeof(inst_data.buffer),
		.resolution = 16,
		.oversampling = 7,
		.calibrate = false,
	},
	.adc_ch_cfg = {
		DT_FOREACH_CHILD_SEP(DT_INST_IO_CHANNELS_CTLR(0), ADC_CHANNEL_CFG_DT, (,))
	},
};

static int temp_lpc_sample_fetch(const struct device *dev,
					enum sensor_channel chan)
{
	const struct temp_lpc_config *config = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return adc_read(config->adc, &config->adc_seq);
}

static int temp_lpc_channel_get(const struct device *dev,
					enum sensor_channel chan, struct sensor_value *val)
{
	struct temp_lpc_data *data = dev->data;

	uint16_t Vbe1;
	uint16_t Vbe8;
	float slope;
	float offset;
	float temperature;
	uint32_t cal_slope;
	uint32_t cal_offset;

	const float alpha = FSL_FEATURE_LPADC_TEMP_PARAMETER_ALPHA;
	const uintptr_t slope_addr = FSL_FEATURE_FLASH_NMPA_TEMP_SLOPE_ADDRS;
	const uintptr_t offset_addr = FSL_FEATURE_FLASH_NMPA_TEMP_OFFSET_ADDRS;

	if (chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* Read the calibration from flash. */
	cal_slope = (*((volatile uint32_t *)(slope_addr)));
	cal_offset = (*((volatile uint32_t *)(offset_addr)));

	if (((cal_slope & 0x1UL) != 0UL) && ((cal_offset & 0x1UL) != 0UL)) {
		/* Re-justify based on the calibration values. */
		slope = ((float)(uint32_t)(cal_slope >> 1UL) / 1024.0f);
		offset = ((float)(uint32_t)(cal_offset >> 1UL) / 1024.0f);
	} else {
		/* Otherwise, use the default slope and offset value. */
		slope = FSL_FEATURE_LPADC_TEMP_PARAMETER_A;
		offset = FSL_FEATURE_LPADC_TEMP_PARAMETER_B;
	}

	/* Read the 2 temperature sensor result. */
#if (FSL_FEATURE_LPADC_TEMP_SENS_BUFFER_SIZE == 4U)
	Vbe1 = data->buffer[2];
	Vbe8 = data->buffer[3];
#else
	Vbe1 = data->buffer[0];
	Vbe8 = data->buffer[1];
#endif

	/* T = A*[alpha*(Vbe8-Vbe1)/(Vbe8 + alpha*(Vbe8-Vbe1))] - B. */
	temperature = slope * (alpha * ((float)Vbe8 - (float)Vbe1) /
			((float)Vbe8 + alpha * ((float)Vbe8 - (float)Vbe1))) - offset;

	return sensor_value_from_float(val, temperature);
}

static DEVICE_API(sensor, temp_lpc_driver_api) = {
	.sample_fetch = temp_lpc_sample_fetch,
	.channel_get = temp_lpc_channel_get,
};

static int temp_lpc_init(const struct device *dev)
{
	const struct temp_lpc_config *config = dev->config;
	const struct adc_channel_cfg *channels = config->adc_ch_cfg;
	int ret;

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device is not ready");
		return -EINVAL;
	}

	ret = adc_channel_setup(config->adc, &channels[config->sensor_adc_ch]);
	if (ret) {
		LOG_ERR("failed to configure ADC channel (%d)", ret);
		return ret;
	}

	return 0;
}

SENSOR_DEVICE_DT_INST_DEFINE(0, temp_lpc_init, NULL, &inst_data,
			&inst_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			&temp_lpc_driver_api);
