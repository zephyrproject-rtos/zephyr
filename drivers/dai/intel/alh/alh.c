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

#define DT_DRV_COMPAT intel_alh_dai
#define LOG_DOMAIN dai_intel_alh

LOG_MODULE_REGISTER(LOG_DOMAIN);

#include "alh.h"

/* Digital Audio interface formatting */
static int dai_alh_set_config_tplg(struct dai_intel_alh *dp, const void *spec_config)
{
	struct dai_intel_alh_pdata *alh = dai_get_drvdata(dp);
	const struct dai_intel_ipc3_alh_params *config = spec_config;

	if (config->channels && config->rate) {
		alh->params.channels = config->channels;
		alh->params.rate = config->rate;
		LOG_INF("%s channels %d rate %d", __func__, config->channels, config->rate);
	}

	alh->params.stream_id = config->stream_id;

	return 0;
}

static int dai_alh_set_config_blob(struct dai_intel_alh *dp, const struct dai_config *cfg,
				   const void *spec_config)
{
	struct dai_intel_alh_pdata *alh = dai_get_drvdata(dp);
	const struct dai_intel_ipc4_alh_configuration_blob *blob = spec_config;
	const struct ipc4_alh_multi_gtw_cfg *alh_cfg = &blob->alh_cfg;

	alh->params.rate = cfg->rate;

	for (int i = 0; i < alh_cfg->count; i++) {
		/* the LSB 8bits are for stream id */
		int alh_id = alh_cfg->mapping[i].alh_id & 0xff;

		if (IPC4_ALH_DAI_INDEX(alh_id) == dp->index) {
			alh->params.stream_id = alh_id;
			alh->params.channels = POPCOUNT(alh_cfg->mapping[i].channel_mask);
			break;
		}
	}

	return 0;
}

static int dai_alh_trigger(const struct device *dev, enum dai_dir dir,
			   enum dai_trigger_cmd cmd)
{
	LOG_DBG("cmd %d", cmd);

	return 0;
}

static void alh_claim_ownership(void)
{
#if CONFIG_DAI_ALH_HAS_OWNERSHIP
	uint32_t ALHASCTL = DT_INST_PROP_BY_IDX(0, reg, 0);
	uint32_t ALHCSCTL = DT_INST_PROP_BY_IDX(0, reg, 1);

	sys_write32(sys_read32(ALHASCTL) | ALHASCTL_OSEL(0x3), ALHASCTL);
	sys_write32(sys_read32(ALHCSCTL) | ALHASCTL_OSEL(0x3), ALHCSCTL);
#endif
}

static void alh_release_ownership(void)
{
#if CONFIG_DAI_ALH_HAS_OWNERSHIP
	uint32_t ALHASCTL = DT_INST_PROP_BY_IDX(0, reg, 0);
	uint32_t ALHCSCTL = DT_INST_PROP_BY_IDX(0, reg, 1);

	sys_write32(sys_read32(ALHASCTL) | ALHASCTL_OSEL(0), ALHASCTL);
	sys_write32(sys_read32(ALHCSCTL) | ALHASCTL_OSEL(0), ALHCSCTL);
#endif
}


static int dai_alh_config_get(const struct device *dev, struct dai_config *cfg,
			      enum dai_dir dir)
{
	struct dai_config *params = (struct dai_config *)dev->config;
	struct dai_intel_alh *dp = (struct dai_intel_alh *)dev->data;
	struct dai_intel_alh_pdata *alh = dai_get_drvdata(dp);

	if (!cfg) {
		return -EINVAL;
	}

	params->rate = alh->params.rate;
	params->channels = alh->params.channels;
	params->word_size = ALH_WORD_SIZE_DEFAULT;

	*cfg = *params;

	return 0;
}

static int dai_alh_config_set(const struct device *dev, const struct dai_config *cfg,
				  const void *bespoke_cfg)
{
	struct dai_intel_alh *dp = (struct dai_intel_alh *)dev->data;

	LOG_DBG("%s", __func__);

	if (cfg->type == DAI_INTEL_ALH) {
		return dai_alh_set_config_tplg(dp, bespoke_cfg);
	} else {
		return dai_alh_set_config_blob(dp, cfg, bespoke_cfg);
	}
}

static const struct dai_properties *dai_alh_get_properties(const struct device *dev,
							   enum dai_dir dir, int stream_id)
{
	struct dai_intel_alh *dp = (struct dai_intel_alh *)dev->data;
	struct dai_intel_alh_pdata *alh = dai_get_drvdata(dp);
	struct dai_properties *prop = &alh->props;
	uint32_t offset = dir == DAI_DIR_PLAYBACK ?
		ALH_TXDA_OFFSET : ALH_RXDA_OFFSET;

	prop->fifo_address = dai_base(dp) + offset + ALH_STREAM_OFFSET * stream_id;
	prop->fifo_depth = ALH_GPDMA_BURST_LENGTH;
	prop->dma_hs_id = alh_handshake_map[stream_id];
	prop->stream_id = alh->params.stream_id;

	LOG_DBG("dai_index %u", dp->index);
	LOG_DBG("fifo %u", prop->fifo_address);
	LOG_DBG("handshake %u", prop->dma_hs_id);

	return prop;
}

static int dai_alh_probe(const struct device *dev)
{
	struct dai_intel_alh *dp = (struct dai_intel_alh *)dev->data;
	k_spinlock_key_t key;

	LOG_DBG("%s", __func__);

	key = k_spin_lock(&dp->lock);

	if (dp->sref == 0) {
		alh_claim_ownership();
	}

	dp->sref++;

	k_spin_unlock(&dp->lock, key);

	return 0;
}

static int dai_alh_remove(const struct device *dev)
{
	struct dai_intel_alh *dp = (struct dai_intel_alh *)dev->data;
	k_spinlock_key_t key;

	LOG_DBG("%s", __func__);

	key = k_spin_lock(&dp->lock);

	if (--dp->sref == 0) {
		alh_release_ownership();
	}

	k_spin_unlock(&dp->lock, key);

	return 0;
}

static const struct dai_driver_api dai_intel_alh_api_funcs = {
	.probe			= dai_alh_probe,
	.remove			= dai_alh_remove,
	.config_set		= dai_alh_config_set,
	.config_get		= dai_alh_config_get,
	.trigger		= dai_alh_trigger,
	.get_properties		= dai_alh_get_properties,
};

#define DAI_INTEL_ALH_DEVICE_INIT(n)						\
	static struct dai_config dai_intel_alh_config_##n = {			\
		.type = DAI_INTEL_ALH,						\
		.dai_index = (n / DAI_NUM_ALH_BI_DIR_LINKS_GROUP) << 8 |	\
			(n % DAI_NUM_ALH_BI_DIR_LINKS_GROUP),			\
	};									\
	static struct dai_intel_alh dai_intel_alh_data_##n = {			\
		.index = (n / DAI_NUM_ALH_BI_DIR_LINKS_GROUP) << 8 |		\
			(n % DAI_NUM_ALH_BI_DIR_LINKS_GROUP),			\
		.plat_data = {							\
			.base =	DT_INST_PROP_BY_IDX(n, reg, 0),			\
			.fifo_depth[DAI_DIR_PLAYBACK] =	ALH_GPDMA_BURST_LENGTH,		\
			.fifo_depth[DAI_DIR_CAPTURE] = ALH_GPDMA_BURST_LENGTH,		\
		},								\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			NULL, NULL,					\
			&dai_intel_alh_data_##n,			\
			&dai_intel_alh_config_##n,			\
			POST_KERNEL, 32,				\
			&dai_intel_alh_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_ALH_DEVICE_INIT)
