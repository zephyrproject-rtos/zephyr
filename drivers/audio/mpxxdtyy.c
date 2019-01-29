/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpxxdtyy.h"

#define LOG_LEVEL CONFIG_MPXXDTYY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpxxdtyy);

u16_t sw_filter_lib_init(struct device *dev, struct dmic_cfg *cfg)
{
	struct mpxxdtyy_data *const data = DEV_DATA(dev);
	TPDMFilter_InitStruct *pdm_filter = &data->pdm_filter;
	u16_t factor;
	u32_t audio_freq = cfg->streams->pcm_rate;

	/* calculate oversampling factor based on pdm clock */
	for (factor = 64; factor <= 128; factor += 64) {
		u32_t pdm_bit_clk = (audio_freq * factor *
				     cfg->channel.req_num_chan);

		if (pdm_bit_clk >= cfg->io.min_pdm_clk_freq &&
		    pdm_bit_clk <= cfg->io.max_pdm_clk_freq) {
			break;
		}
	}

	if (factor != 64 && factor != 128) {
		return 0;
	}

	/* init the filter lib */
	pdm_filter->LP_HZ = audio_freq / 2;
	pdm_filter->HP_HZ = 10;
	pdm_filter->Fs = audio_freq;
	pdm_filter->Out_MicChannels = 1;
	pdm_filter->In_MicChannels = 1;
	pdm_filter->Decimation = factor;
	pdm_filter->MaxVolume = 64;

	Open_PDM_Filter_Init(pdm_filter);

	return factor;
}

int sw_filter_lib_run(TPDMFilter_InitStruct *pdm_filter,
		      void *pdm_block, void *pcm_block,
		      size_t pdm_size, size_t pcm_size)
{
	int i;

	if (pdm_block == NULL || pcm_block == NULL || pdm_filter == NULL) {
		return -EINVAL;
	}

	for (i = 0; i < pdm_size/2; i++) {
		((u16_t *)pdm_block)[i] = HTONS(((u16_t *)pdm_block)[i]);
	}

	switch (pdm_filter->Decimation) {
	case 64:
		Open_PDM_Filter_64((u8_t *) pdm_block, pcm_block,
				    pcm_size, pdm_filter);
		break;
	case 128:
		Open_PDM_Filter_128((u8_t *) pdm_block, pcm_block,
				    pcm_size, pdm_filter);
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

	data->comm_master = device_get_binding(DT_ST_MPXXDTYY_0_BUS_NAME);

	if (data->comm_master == NULL) {
		LOG_ERR("master %s not found", DT_ST_MPXXDTYY_0_BUS_NAME);
		return -EINVAL;
	}

	data->state = DMIC_STATE_INITIALIZED;
	return 0;
}

static struct mpxxdtyy_data mpxxdtyy_data;

DEVICE_AND_API_INIT(mpxxdtyy, DT_ST_MPXXDTYY_0_LABEL, mpxxdtyy_initialize,
		&mpxxdtyy_data, NULL, POST_KERNEL,
		CONFIG_AUDIO_DMIC_INIT_PRIORITY, &mpxxdtyy_driver_api);
