/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpxxdtyy.h"
#include <drivers/i2s.h>

#define LOG_LEVEL CONFIG_AUDIO_DMIC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(mpxxdtyy);

#ifdef DT_ST_MPXXDTYY_BUS_I2S

#define NUM_RX_BLOCKS			4
#define PDM_BLOCK_MAX_SIZE_BYTES	512

K_MEM_SLAB_DEFINE(rx_pdm_i2s_mslab, PDM_BLOCK_MAX_SIZE_BYTES, NUM_RX_BLOCKS, 1);

int mpxxdtyy_i2s_read(struct device *dev, u8_t stream, void **buffer,
		      size_t *size, s32_t timeout)
{
	int ret;
	struct mpxxdtyy_data *const data = DEV_DATA(dev);
	void *pdm_block, *pcm_block;
	size_t pdm_size;
	TPDMFilter_InitStruct *pdm_filter = &data->pdm_filter[0];

	ret = i2s_read(data->comm_master, &pdm_block, &pdm_size);
	if (ret != 0) {
		LOG_ERR("read failed (%d)", ret);
		return ret;
	}

	ret = k_mem_slab_alloc(data->pcm_mem_slab,
			       &pcm_block, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	sw_filter_lib_run(pdm_filter, pdm_block, pcm_block, pdm_size,
			  data->pcm_mem_size);
	k_mem_slab_free(&rx_pdm_i2s_mslab, &pdm_block);

	*buffer = pcm_block;
	*size = data->pcm_mem_size;

	return 0;
}

int mpxxdtyy_i2s_trigger(struct device *dev, enum dmic_trigger cmd)
{
	int ret;
	struct mpxxdtyy_data *const data = DEV_DATA(dev);
	enum i2s_trigger_cmd i2s_cmd;
	enum dmic_state tmp_state;

	switch (cmd) {
	case DMIC_TRIGGER_START:
		if (data->state == DMIC_STATE_CONFIGURED) {
			tmp_state = DMIC_STATE_ACTIVE;
			i2s_cmd = I2S_TRIGGER_START;
		} else {
			return 0;
		}
		break;
	case DMIC_TRIGGER_STOP:
		if (data->state == DMIC_STATE_ACTIVE) {
			tmp_state = DMIC_STATE_CONFIGURED;
			i2s_cmd = I2S_TRIGGER_STOP;
		} else {
			return 0;
		}
		break;
	default:
		return -EINVAL;
	}

	ret = i2s_trigger(data->comm_master, I2S_DIR_RX, i2s_cmd);
	if (ret != 0) {
		LOG_ERR("trigger failed with %d error", ret);
		return ret;
	}

	data->state = tmp_state;
	return 0;
}

int mpxxdtyy_i2s_configure(struct device *dev, struct dmic_cfg *cfg)
{
	int ret;
	struct mpxxdtyy_data *const data = DEV_DATA(dev);
	u8_t chan_size = cfg->streams->pcm_width;
	u32_t audio_freq = cfg->streams->pcm_rate;
	u16_t factor;

	/* PCM buffer size */
	data->pcm_mem_slab = cfg->streams->mem_slab;
	data->pcm_mem_size = cfg->streams->block_size;

	/* check requested min pdm frequency */
	if (cfg->io.min_pdm_clk_freq < MPXXDTYY_MIN_PDM_FREQ ||
	    cfg->io.min_pdm_clk_freq > cfg->io.max_pdm_clk_freq) {
		return -EINVAL;
	}

	/* check requested max pdm frequency */
	if (cfg->io.max_pdm_clk_freq > MPXXDTYY_MAX_PDM_FREQ ||
	    cfg->io.max_pdm_clk_freq < cfg->io.min_pdm_clk_freq) {
		return -EINVAL;
	}

	factor = sw_filter_lib_init(dev, cfg);
	if (factor == 0U) {
		return -EINVAL;
	}

	/* configure I2S channels */
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = chan_size;
	i2s_cfg.channels = cfg->channel.req_num_chan;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED |
			 I2S_FMT_BIT_CLK_INV;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.frame_clk_freq = audio_freq * factor / chan_size;
	i2s_cfg.block_size = data->pcm_mem_size * (factor / chan_size);
	i2s_cfg.mem_slab = &rx_pdm_i2s_mslab;
	i2s_cfg.timeout = 2000;

	ret = i2s_configure(data->comm_master, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("I2S device configuration error");
		return ret;
	}

	data->state = DMIC_STATE_CONFIGURED;
	return 0;
}
#endif /* DT_ST_MPXXDTYY_BUS_I2S */
