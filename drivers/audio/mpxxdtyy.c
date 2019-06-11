/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpxxdtyy.h"

#define LOG_LEVEL CONFIG_AUDIO_DMIC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpxxdtyy);

#define CHANNEL_MASK	0x55

static u8_t ch_demux[128] = {
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

static u8_t left_channel(u8_t a, u8_t b)
{
	return ch_demux[a & CHANNEL_MASK] | (ch_demux[b & CHANNEL_MASK] << 4);
}

static u8_t right_channel(u8_t a, u8_t b)
{
	a >>= 1;
	b >>= 1;
	return ch_demux[a & CHANNEL_MASK] | (ch_demux[b & CHANNEL_MASK] << 4);
}

u16_t sw_filter_lib_init(struct device *dev, struct dmic_cfg *cfg)
{
	struct mpxxdtyy_data *const data = DEV_DATA(dev);
	TPDMFilter_InitStruct *pdm_filter = &data->pdm_filter[0];
	u16_t factor;
	u32_t audio_freq = cfg->streams->pcm_rate;
	int i;

	/* calculate oversampling factor based on pdm clock */
	for (factor = 64U; factor <= 128U; factor += 64U) {
		u32_t pdm_bit_clk = (audio_freq * factor *
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
	u8_t a, b;

	if (pdm_block == NULL || pcm_block == NULL || pdm_filter == NULL) {
		return -EINVAL;
	}

	for (i = 0; i < pdm_size/2; i++) {
		switch (pdm_filter[0].In_MicChannels) {
		case 1: /* MONO */
			((u16_t *)pdm_block)[i] = HTONS(((u16_t *)pdm_block)[i]);
			break;

		case 2: /* STEREO */
			if (pdm_filter[0].In_MicChannels > 1) {
				a = ((u8_t *)pdm_block)[2*i];
				b = ((u8_t *)pdm_block)[2*i + 1];

				((u8_t *)pdm_block)[2*i] = left_channel(a, b);
				((u8_t *)pdm_block)[2*i + 1] = right_channel(a, b);
			}
			break;

		default:
			return -EINVAL;
		}
	}

	switch (pdm_filter[0].Decimation) {
	case 64:
		for (i = 0; i < pdm_filter[0].In_MicChannels; i++) {
			Open_PDM_Filter_64(&((u8_t *) pdm_block)[i],
					   &((u16_t *) pcm_block)[i],
					   pdm_filter->MaxVolume,
					   &pdm_filter[i]);
		}
		break;
	case 128:
		for (i = 0; i < pdm_filter[0].In_MicChannels; i++) {
			Open_PDM_Filter_128(&((u8_t *) pdm_block)[i],
					    &((u16_t *) pcm_block)[i],
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
#ifdef DT_ST_MPXXDTYY_BUS_I2S
	.configure		= mpxxdtyy_i2s_configure,
	.trigger		= mpxxdtyy_i2s_trigger,
	.read			= mpxxdtyy_i2s_read,
#endif /* DT_ST_MPXXDTYY_BUS_I2S */
};

static int mpxxdtyy_initialize(struct device *dev)
{
	struct mpxxdtyy_data *const data = DEV_DATA(dev);

	data->comm_master = device_get_binding(DT_INST_0_ST_MPXXDTYY_BUS_NAME);

	if (data->comm_master == NULL) {
		LOG_ERR("master %s not found", DT_INST_0_ST_MPXXDTYY_BUS_NAME);
		return -EINVAL;
	}

	data->state = DMIC_STATE_INITIALIZED;
	return 0;
}

static struct mpxxdtyy_data mpxxdtyy_data;

DEVICE_AND_API_INIT(mpxxdtyy, DT_INST_0_ST_MPXXDTYY_LABEL, mpxxdtyy_initialize,
		&mpxxdtyy_data, NULL, POST_KERNEL,
		CONFIG_AUDIO_DMIC_INIT_PRIORITY, &mpxxdtyy_driver_api);
