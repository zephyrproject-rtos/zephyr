/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_hpdac

#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_nxp_hpdac, CONFIG_DAC_LOG_LEVEL);

struct nxp_hpdac_config {
	HPDAC_Type *base;
	uint8_t resolution;
};

struct nxp_hpdac_data {
	bool configured;
};

static int nxp_hpdac_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct nxp_hpdac_config *config = dev->config;
	struct nxp_hpdac_data *data = dev->data;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("internal channels not supported");
		return -ENOTSUP;
	}

	config->base->RCR = HPDAC_RCR_SWRST_MASK;
	config->base->RCR = 0U;
	config->base->RCR = HPDAC_RCR_FIFORST_MASK;
	config->base->RCR = 0U;

	config->base->GCR = HPDAC_GCR_DACEN_MASK;

	data->configured = true;

	return 0;
}

static int nxp_hpdac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct nxp_hpdac_config *config = dev->config;
	struct nxp_hpdac_data *data = dev->data;

	if (!data->configured) {
		LOG_ERR("channel not initialized");
		return -EINVAL;
	}

	if (channel != 0) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= BIT(config->resolution)) {
		LOG_ERR("unsupported value %d", value);
		return -EINVAL;
	}

	config->base->DATA = HPDAC_DATA_DATA(value);

	return 0;
}

static DEVICE_API(dac, nxp_hpdac_driver_api) = {
	.channel_setup = nxp_hpdac_channel_setup,
	.write_value = nxp_hpdac_write_value,
};

#define NXP_HPDAC_INIT(n)						\
	static struct nxp_hpdac_data nxp_hpdac_data_##n;		\
									\
	static const struct nxp_hpdac_config nxp_hpdac_config_##n = {	\
		.base = (HPDAC_Type *)DT_INST_REG_ADDR(n),		\
		.resolution = DT_INST_PROP(n, resolution),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &nxp_hpdac_data_##n,	\
			      &nxp_hpdac_config_##n,			\
			      POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,	\
			      &nxp_hpdac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_HPDAC_INIT)
