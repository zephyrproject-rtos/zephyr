/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT intel_hda_dai
#define LOG_DOMAIN dai_intel_hda

LOG_MODULE_REGISTER(LOG_DOMAIN);

#include "hda.h"

static int dai_hda_trigger(const struct device *dev, enum dai_dir dir,
			   enum dai_trigger_cmd cmd)
{
	LOG_DBG("cmd %d", cmd);

	return 0;
}

/* Digital Audio interface formatting */
static int dai_hda_set_config_tplg(struct dai_intel_hda *dp, const void *spec_config)
{
	struct dai_intel_hda_pdata *hda = dai_get_drvdata(dp);
	const struct dai_intel_ipc_hda_params *config = spec_config;

	if (config->channels)
		hda->params.channels = config->channels;
	if (config->rate)
		hda->params.rate = config->rate;

	return 0;
}

static const struct dai_config *dai_hda_config_get(const struct device *dev, enum dai_dir dir)
{
	struct dai_config *params = (struct dai_config *)dev->config;
	struct dai_intel_hda *dp = (struct dai_intel_hda *)dev->data;
	struct dai_intel_hda_pdata *hda = dai_get_drvdata(dp);

	params->rate = hda->params.rate;
	params->channels = hda->params.channels;

	params->word_size = DAI_INTEL_HDA_DEFAULT_WORD_SIZE;

	return params;
}

static int dai_hda_config_set(const struct device *dev, const struct dai_config *cfg,
				  const void *bespoke_cfg)
{
	struct dai_intel_hda *dp = (struct dai_intel_hda *)dev->data;

	if (cfg->type == DAI_INTEL_HDA)
		return dai_hda_set_config_tplg(dp, bespoke_cfg);

	return 0;
}

static const struct dai_properties *dai_hda_get_properties(const struct device *dev,
							   enum dai_dir dir, int stream_id)
{
	struct dai_intel_hda *dp = (struct dai_intel_hda *)dev->data;
	struct dai_intel_hda_pdata *hda = dai_get_drvdata(dp);
	struct dai_properties *prop = &hda->props;

	prop->fifo_address = 0;
	prop->dma_hs_id = 0;
	prop->stream_id = 0;

	return prop;
}

static int dai_hda_probe(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	return 0;
}

static int dai_hda_remove(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	return 0;
}

static int hda_init(const struct device *dev)
{
	return 0;
}

static const struct dai_driver_api dai_intel_hda_api_funcs = {
	.probe			= dai_hda_probe,
	.remove			= dai_hda_remove,
	.config_set		= dai_hda_config_set,
	.config_get		= dai_hda_config_get,
	.trigger		= dai_hda_trigger,
	.get_properties		= dai_hda_get_properties,
};

#define DAI_INTEL_HDA_DEVICE_INIT(n)				\
	static struct dai_config dai_intel_hda_config_##n = {	\
		.type = DAI_INTEL_HDA,				\
		.dai_index = DT_INST_REG_ADDR(n),	\
	};							\
	static struct dai_intel_hda dai_intel_hda_data_##n = {	\
		.index = DT_INST_REG_ADDR(n)		\
								\
	};							\
								\
	DEVICE_DT_INST_DEFINE(n,				\
			hda_init, NULL,				\
			&dai_intel_hda_data_##n,		\
			&dai_intel_hda_config_##n,		\
			POST_KERNEL, 32,			\
			&dai_intel_hda_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_HDA_DEVICE_INIT)
