/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/dai.h>
#include "dmic_dai.h"
#include <zephyr/irq.h>
#if defined(CONFIG_SOC_ACP_7_X)
#include <acp7x_chip_offsets.h>
#include <acp7x_chip_reg.h>
#endif
#define DT_DRV_COMPAT amd_dmic_dai

LOG_MODULE_REGISTER(acp_dmic_dai, CONFIG_DAI_LOG_LEVEL);
#define SILENCE_ADDING 1
extern struct acp_dmic_silence acp_initsilence;

static inline int acp_dmic_dai_set_config(const struct device *dev, const struct dai_config *cfg,
					  const void *spec_config, size_t spec_config_size)
{
	acp_wov_pdm_no_of_channels_t pdm_channels;
	struct acp_dmic_dai_pdata *dmic = dev->data;
	struct acp_dmic_dai_config *acp_dmic_dai_config = dmic->priv_data;

	if (cfg->type != DAI_AMD_DMIC) {
		return -EINVAL;
	}

	const struct sof_ipc_dai_acpdmic_params *config = spec_config;

	LOG_DBG("set_config: dai_idx=%d rate=%d ch=%d", cfg->dai_index, config->pdm_rate,
		config->pdm_ch);

	acp_dmic_dai_config->type = cfg->type;
	acp_dmic_dai_config->dai_index = cfg->dai_index;
	if (acp_dmic_dai_config->dai_index == 0) {
		if (config->pdm_rate || config->pdm_ch) {
			acp_dmic_dai_config->dmic_params->pdm_rate = config->pdm_rate;
			acp_dmic_dai_config->dmic_params->pdm_ch = config->pdm_ch;
			LOG_DBG("DMIC0: rate=%d ch=%d", config->pdm_rate, config->pdm_ch);
		}
		io_reg_write(PU_REGISTER_BASE + ACP_AUDIO_CLK_SEL, 0x1);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_CORE_CLK_SEL, 0x0);
	} else if (acp_dmic_dai_config->dai_index == 1) {
		if (config->pdm_rate || config->pdm_ch) {
			acp_dmic_dai_config->dmic1_params->pdm_rate = config->pdm_rate;
			acp_dmic_dai_config->dmic1_params->pdm_ch = config->pdm_ch;
			LOG_DBG("DMIC1: rate=%d ch=%d", config->pdm_rate, config->pdm_ch);
		}
	}
	switch (config->pdm_ch) {
	case 2:
		pdm_channels.bits.pdm_no_of_channels = 0;
		break;
	case 4:
		pdm_channels.bits.pdm_no_of_channels = 1;
		break;
	default:
		LOG_WRN("DMIC%d:Unsupported channels", acp_dmic_dai_config->dai_index);
		break;
	}

	switch (config->pdm_rate) {
	case 96000:
		LOG_DBG("clock: 96kHz brm_clk_ctrl=7");
		break;
	case 48000:
		LOG_DBG("clock: 48kHz brm_clk_ctrl=7");
		break;
	case 16000:
		LOG_DBG("clock: 16kHz brm_clk_ctrl=7");
		break;
	default:
		/* Unsupported samplerate */
		LOG_WRN("unsupported pdm_rate=%d", config->pdm_rate);
		break;
	}
	if (acp_dmic_dai_config->dai_index == 0) {
		if (config->pdm_rate != 48000) {
			LOG_ERR("DMIC0 supports 48000 only, rate=%d", config->pdm_rate);
			return -EINVAL;
		}
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_DECIMATION_FACTOR, 0x2);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_CLKDIV, 0xC0);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_NO_OF_CHANNELS, pdm_channels.u32all);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_CLK_CTRL, 0x7);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_MISC_CTRL, 0x0);
#if SILENCE_ADDING
		acp_initsilence.samplerate_khz = acp_dmic_dai_config->dmic_params->pdm_rate / 1000;
		acp_initsilence.num_chs = acp_dmic_dai_config->dmic_params->pdm_ch;
#endif
	} else if (acp_dmic_dai_config->dai_index == 1) {
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_CLKDIV, 0x40);
		if (config->pdm_rate == 96000) {
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DECIMATION_FACTOR, 0x1);
		} else if (config->pdm_rate == 48000) {
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DECIMATION_FACTOR, 0x2);
		} else if (config->pdm_rate == 16000) {
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DECIMATION_FACTOR, 0x4);
		}
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_NO_OF_CHANNELS, pdm_channels.u32all);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_CLK_CTRL, 0x7);
		io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_MISC_CTRL, 0x0);
#if SILENCE_ADDING
		acp_initsilence.samplerate_khz =
			acp_dmic_dai_config->dmic1_params->pdm_rate / 1000;
		acp_initsilence.num_chs = acp_dmic_dai_config->dmic1_params->pdm_ch;
#endif
	}
	return 0;
}

static const struct dai_properties *dai_dmic_get_properties(const struct device *dev,
							    enum dai_dir direction, int stream_id)
{
	struct acp_dmic_dai_pdata *data = (struct acp_dmic_dai_pdata *)dev->data;
	struct acp_dmic_dai_config *acp_dmic_dai_config = data->priv_data;
	struct dai_properties *prop = &acp_dmic_dai_config->prop;

	struct dai_config *params = (struct dai_config *)dev->config;

	if (acp_dmic_dai_config->dai_index == 0 || acp_dmic_dai_config->dai_index == 1) {
		prop->dma_hs_id = params->dai_index;
		prop->fifo_depth = 8;
	}
	return prop;
}

static int acp_dmic_dai_probe(const struct device *dev)
{
	return 0;
}

static int acp_dmic_dai_remove(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int acp_dmic_dai_trigger(const struct device *dev, enum dai_dir dir,
				enum dai_trigger_cmd cmd)
{
	return 0;
}

static int dai_dmic_get_config(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_config *params = (struct dai_config *)dev->config;

	LOG_DBG("get_config: dai_idx=%d", params->dai_index);
	LOG_DBG("get_config: rate=%d", params->rate);

	struct acp_dmic_dai_pdata *dmic = dev->data;
	struct acp_dmic_dai_config *acp_dmic_dai_config = dmic->priv_data;

	if (params->dai_index == 0) {
		params->rate = acp_dmic_dai_config->dmic_params->pdm_rate;
		params->tdm_slot_group = acp_dmic_dai_config->dmic_params->pdm_ch;
		params->channels = acp_dmic_dai_config->dmic_params->pdm_ch;
		acp_initsilence.samplerate_khz = acp_dmic_dai_config->dmic_params->pdm_rate / 1000;
		LOG_DBG("get_config DMIC0: rate=%d", acp_dmic_dai_config->dmic_params->pdm_rate);
	} else if (params->dai_index == 1) {
		params->rate = acp_dmic_dai_config->dmic1_params->pdm_rate;
		params->tdm_slot_group = acp_dmic_dai_config->dmic1_params->pdm_ch;
		params->channels = acp_dmic_dai_config->dmic1_params->pdm_ch;
		acp_initsilence.samplerate_khz = acp_dmic_dai_config->dmic1_params->pdm_rate / 1000;
		LOG_DBG("get_config DMIC1: rate=%d", acp_dmic_dai_config->dmic1_params->pdm_rate);
		switch (acp_dmic_dai_config->dmic1_params->pdm_rate) {
		case 48000:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable, 0x2);
			break;
		case 16000:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable, 0x4);
			LOG_INF("PDM1 dma enable 16k value=0x%x",
				io_reg_read(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable));
			break;
		case 96000:
			io_reg_write(PU_REGISTER_BASE + ACP_PDM_2_DMA_Enable, 0x1);
			break;
		default:
			return -EINVAL;
		}
	}
	*cfg = *params;
	return 0;
}

static DEVICE_API(dai, dai_acpdmic_ops) = {
	.probe = acp_dmic_dai_probe,
	.remove = acp_dmic_dai_remove,
	.config_set = acp_dmic_dai_set_config,
	.config_get = dai_dmic_get_config,
	.get_properties = dai_dmic_get_properties,
	.trigger = acp_dmic_dai_trigger,
};

static int acp_dmic_dai_init(const struct device *dev)
{
	struct acp_dmic_dai_pdata *data = dev->data;
	struct acp_dmic_dai_config *acp_dmic_dai_config = data->priv_data;

	static struct sof_ipc_dai_acpdmic_params default_dmic0_params = {
		.pdm_rate = 48000,
		.pdm_ch = 2,
	};
	static struct sof_ipc_dai_acpdmic_params default_dmic1_params = {
		.pdm_rate = 48000,
		.pdm_ch = 2,
	};
	LOG_INF("init: default pdm0_rate=%d", default_dmic0_params.pdm_rate);
	LOG_INF("init: default pdm1_rate=%d", default_dmic1_params.pdm_rate);
	/* Assign default DMIC params */
	acp_dmic_dai_config->dmic_params = &default_dmic0_params;
	acp_dmic_dai_config->dmic1_params = &default_dmic1_params;
	return 0;
}

#define AMD_DAI_DMIC_INIT(inst)                                                                    \
	static struct acp_dmic_dai_config acp_dai_dmic_cfg_##inst = {                              \
		.type = DAI_AMD_DMIC,                                                              \
		.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),                                  \
	};                                                                                         \
	static struct acp_dmic_dai_pdata acp_dmic_dai_pdata_##inst = {                             \
		.priv_data = &acp_dai_dmic_cfg_##inst,                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &acp_dmic_dai_init, NULL, &acp_dmic_dai_pdata_##inst,          \
			      &acp_dai_dmic_cfg_##inst, POST_KERNEL, CONFIG_DAI_INIT_PRIORITY,     \
			      &dai_acpdmic_ops);

DT_INST_FOREACH_STATUS_OKAY(AMD_DAI_DMIC_INIT);
