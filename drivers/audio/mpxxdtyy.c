/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_mpxxdtyy

#include <zephyr/devicetree.h>

#include "mpxxdtyy.h"

#define LOG_LEVEL CONFIG_AUDIO_DMIC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mpxxdtyy);

#define CHANNEL_MASK	0x55

static uint8_t ch_demux[128] = {
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f
};

static uint8_t left_channel(uint8_t a, uint8_t b)
{
	return ch_demux[a & CHANNEL_MASK] | (ch_demux[b & CHANNEL_MASK] << 4);
}

static uint8_t right_channel(uint8_t a, uint8_t b)
{
	a >>= 1;
	b >>= 1;
	return ch_demux[a & CHANNEL_MASK] | (ch_demux[b & CHANNEL_MASK] << 4);
}

uint16_t sw_filter_lib_init(const struct device *dev, struct dmic_cfg *cfg)
{
	struct mpxxdtyy_data *const data = dev->data;
	TPDMFilter_InitStruct *pdm_filter = &data->pdm_filter[0];
	uint16_t factor;
	uint32_t audio_freq = cfg->streams->pcm_rate;
	int i;

	/* calculate oversampling factor based on pdm clock */
	for (factor = 64U; factor <= 128U; factor += 64U) {
		uint32_t pdm_bit_clk = (audio_freq * factor *
				     cfg->channel.req_num_chan);

		if (pdm_bit_clk >= cfg->io.min_pdm_clk_freq &&
		    pdm_bit_clk <= cfg->io.max_pdm_clk_freq) {
			break;
		}
	}

	if (factor != 64U && factor != 128U) {
		return 0;
	}

	for (i = 0; i < cfg->channel.req_num_chan; i++) {
		/* init the filter lib */
		pdm_filter[i].LP_HZ = audio_freq / 2U;
		pdm_filter[i].HP_HZ = 10;
		pdm_filter[i].Fs = audio_freq;
		pdm_filter[i].Out_MicChannels = cfg->channel.req_num_chan;
		pdm_filter[i].In_MicChannels = cfg->channel.req_num_chan;
		pdm_filter[i].Decimation = factor;
		pdm_filter[i].MaxVolume = 64;

		Open_PDM_Filter_Init(&data->pdm_filter[i]);
	}

	return factor;
}

int sw_filter_lib_run(TPDMFilter_InitStruct *pdm_filter,
		      void *pdm_block, void *pcm_block,
		      size_t pdm_size, size_t pcm_size)
{
	int i;
	uint8_t a, b;

	if (pdm_block == NULL || pcm_block == NULL || pdm_filter == NULL) {
		return -EINVAL;
	}

	for (i = 0; i < pdm_size/2; i++) {
		switch (pdm_filter[0].In_MicChannels) {
		case 1: /* MONO */
			((uint16_t *)pdm_block)[i] = HTONS(((uint16_t *)pdm_block)[i]);
			break;

		case 2: /* STEREO */
			if (pdm_filter[0].In_MicChannels > 1) {
				a = ((uint8_t *)pdm_block)[2*i];
				b = ((uint8_t *)pdm_block)[2*i + 1];

				((uint8_t *)pdm_block)[2*i] = left_channel(a, b);
				((uint8_t *)pdm_block)[2*i + 1] = right_channel(a, b);
			}
			break;

		default:
			return -EINVAL;
		}
	}

	switch (pdm_filter[0].Decimation) {
	case 64:
		for (i = 0; i < pdm_filter[0].In_MicChannels; i++) {
			Open_PDM_Filter_64(&((uint8_t *) pdm_block)[i],
					   &((uint16_t *) pcm_block)[i],
					   pdm_filter->MaxVolume,
					   &pdm_filter[i]);
		}
		break;
	case 128:
		for (i = 0; i < pdm_filter[0].In_MicChannels; i++) {
			Open_PDM_Filter_128(&((uint8_t *) pdm_block)[i],
					    &((uint16_t *) pcm_block)[i],
					    pdm_filter->MaxVolume,
					    &pdm_filter[i]);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct _dmic_ops mpxxdtyy_driver_api = {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2s)
	.configure		= mpxxdtyy_i2s_configure,
	.trigger		= mpxxdtyy_i2s_trigger,
	.read			= mpxxdtyy_i2s_read,
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2s) */
};

static int mpxxdtyy_initialize(const struct device *dev)
{
	const struct mpxxdtyy_config *config = dev->config;
	struct mpxxdtyy_data *const data = dev->data;

	if (!device_is_ready(config->comm_master)) {
		return -ENODEV;
	}

	data->state = DMIC_STATE_INITIALIZED;
	return 0;
}

static const struct mpxxdtyy_config mpxxdtyy_config = {
	.comm_master = DEVICE_DT_GET(DT_INST_BUS(0)),
};

static struct mpxxdtyy_data mpxxdtyy_data;

DEVICE_DT_INST_DEFINE(0, mpxxdtyy_initialize, NULL, &mpxxdtyy_data,
		      &mpxxdtyy_config, POST_KERNEL,
		      CONFIG_AUDIO_DMIC_INIT_PRIORITY, &mpxxdtyy_driver_api);
