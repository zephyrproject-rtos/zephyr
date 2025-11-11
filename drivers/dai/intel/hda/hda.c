/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
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

	if (config->channels) {
		hda->params.channels = config->channels;
	}

	if (config->rate) {
		hda->params.rate = config->rate;
	}

	return 0;
}

static int dai_hda_config_get(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_config *params = (struct dai_config *)dev->config;
	struct dai_intel_hda *dp = (struct dai_intel_hda *)dev->data;
	struct dai_intel_hda_pdata *hda = dai_get_drvdata(dp);

	if (!cfg) {
		return -EINVAL;
	}

	params->rate = hda->params.rate;
	params->channels = hda->params.channels;

	params->word_size = DAI_INTEL_HDA_DEFAULT_WORD_SIZE;

	*cfg = *params;

	return 0;
}

static int dai_hda_config_set(const struct device *dev, const struct dai_config *cfg,
				  const void *bespoke_cfg)
{
	struct dai_intel_hda *dp = (struct dai_intel_hda *)dev->data;

	if (cfg->type == DAI_INTEL_HDA) {
		return dai_hda_set_config_tplg(dp, bespoke_cfg);
	}

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

static int hda_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		dai_hda_remove(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		dai_hda_probe(dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		/* All device pm is handled during resume and suspend */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int hda_init(const struct device *dev)
{
	LOG_DBG("%s", __func__);
	return pm_device_driver_init(dev, hda_pm_action);
}

static DEVICE_API(dai, dai_intel_hda_api_funcs) = {
	.probe			= pm_device_runtime_get,
	.remove			= pm_device_runtime_put,
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
	PM_DEVICE_DT_INST_DEFINE(n, hda_pm_action);		\
								\
	DEVICE_DT_INST_DEFINE(n,				\
			hda_init, PM_DEVICE_DT_INST_GET(n),	\
			&dai_intel_hda_data_##n,		\
			&dai_intel_hda_config_##n,		\
			POST_KERNEL,				\
			CONFIG_DAI_INIT_PRIORITY,		\
			&dai_intel_hda_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_HDA_DEVICE_INIT)
