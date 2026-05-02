/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <acp_sdw_dai.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#define DAI_BASE_SW (PU_REGISTER_BASE + ACP_AUDIO_RX_RINGBUFADDR)
#endif

LOG_MODULE_REGISTER(amd_dai_acpsdw, CONFIG_DAI_LOG_LEVEL);

#define DT_DRV_COMPAT amd_acp_sdw_dai

static inline int acp_sdwdai_get_config(const struct device *dev, struct dai_config *cfg,
					enum dai_dir dir)
{
	/* dump content of the DAI configuration */
	struct dai_config *params = (struct dai_config *)dev->config;
	struct acp_dai_pdata *data = dev->data;
	struct acp_dai_config *acp_dai_config = data->priv_data;

	params->rate = acp_dai_config->acp_params->rate;
	params->tdm_slot_group = acp_dai_config->acp_params->channels;
	params->channels = acp_dai_config->acp_params->channels;
	*cfg = *params;

	return 0;
}

static inline int acp_sdwdai_set_config(const struct device *dev, const struct dai_config *cfg,
					const void *spec_config, size_t spec_config_size)
{
	struct acp_dai_pdata *data = dev->data;
	struct acp_dai_config *acp_dai_config = data->priv_data;

	if (cfg->type != DAI_AMD_SDW) {
		return -EINVAL;
	}
	const struct sof_ipc_dai_acp_sdw_params *config = spec_config;

	if (config->rate && config->channels) {
		acp_dai_config->acp_params->rate = config->rate;
		acp_dai_config->acp_params->channels = config->channels;
	}
	acp_dai_config->type = cfg->type;
	acp_dai_config->dai_index = cfg->dai_index;

	return 0;
}

static int acp_sdwdai_trigger(const struct device *dev, enum dai_dir dir, enum dai_trigger_cmd cmd)
{
	/* nothing to do for SDW dai */
	return 0;
}

static int acp_sdwdai_probe(const struct device *dev)
{
	return 0;
}

static int acp_sdwdai_remove(const struct device *dev)
{
	return 0;
}

static const struct dai_properties *acp_sdwdai_get_properties(const struct device *dev,
							      enum dai_dir direction, int stream_id)
{
	struct acp_dai_pdata *data = dev->data;
	struct acp_dai_config *acp_dai_config = data->priv_data;
	struct dai_properties *prop = &acp_dai_config->prop;
	struct dai_config *params = (struct dai_config *)dev->config;

#ifdef CONFIG_SOC_ACP_7_0
	switch (direction) {
	case DAI_DIR_RX:
		prop->fifo_address = (DAI_BASE_SW + ACP_AUDIO_RX_FIFOADDR);
		prop->fifo_depth = 8;
		prop->dma_hs_id = params->dai_index;
		return prop;
	case DAI_DIR_TX:
		prop->fifo_address = (DAI_BASE_SW + ACP_AUDIO_TX_FIFOADDR);
		prop->fifo_depth = 8;
		prop->dma_hs_id = params->dai_index;
		return prop;
	default:
		return NULL;
	}
#endif
}

static DEVICE_API(dai, acp_sdwdai_driver_ops) = {
	.probe = acp_sdwdai_probe,
	.remove = acp_sdwdai_remove,
	.config_set = acp_sdwdai_set_config,
	.config_get = acp_sdwdai_get_config,
	.get_properties = acp_sdwdai_get_properties,
	.trigger = acp_sdwdai_trigger,
};

static int amd_dai_acp_sdw_init(const struct device *dev)
{
	/* nothing doing here, we dont do any init of sdw */
	struct acp_dai_pdata *data = dev->data;
	struct acp_dai_config *acp_dai_config = data->priv_data;

	static struct sof_ipc_dai_acp_sdw_params acp_sdw_params = {
		.rate = ACP_SDW_DEFAULT_RATE,
		.channels = ACP_SDW_DEFAULT_CHANNELS,
	};
	acp_dai_config->acp_params = &acp_sdw_params;

	return 0;
}

#define AMD_DAI_ACP_SDW_INIT(inst)                                                                 \
	static struct acp_dai_config acp_dai_sdw_config_##inst = {                                 \
		.type = DAI_AMD_SDW,                                                               \
		.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),                                  \
	};                                                                                         \
	static struct acp_dai_pdata acp_dai_sdw_pdata_##inst = {                                   \
		.priv_data = &acp_dai_sdw_config_##inst,                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &amd_dai_acp_sdw_init, NULL, &acp_dai_sdw_pdata_##inst,        \
			      &acp_dai_sdw_config_##inst, POST_KERNEL, CONFIG_DAI_INIT_PRIORITY,   \
			      &acp_sdwdai_driver_ops)
DT_INST_FOREACH_STATUS_OKAY(AMD_DAI_ACP_SDW_INIT);
