/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2023, NXP
 * Copyright (c) 2025 PHYTEC America LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dac12

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

#include <fsl_dac12.h>

LOG_MODULE_REGISTER(dac_mcux_dac12, CONFIG_DAC_LOG_LEVEL);

struct mcux_dac12_config {
	DAC_Type *base;
	dac12_reference_voltage_source_t reference;
	bool buffered;
};

struct mcux_dac12_data {
	bool configured;
};

static int mcux_dac12_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct mcux_dac12_config *config = dev->config;
	struct mcux_dac12_data *data = dev->data;
	dac12_config_t dac12_config;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != 12) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	DAC12_GetDefaultConfig(&dac12_config);
	dac12_config.referenceVoltageSource = config->reference;

	DAC12_Init(config->base, &dac12_config);
	DAC12_Enable(config->base, true);

	data->configured = true;

	return 0;
}

static int mcux_dac12_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct mcux_dac12_config *config = dev->config;
	struct mcux_dac12_data *data = dev->data;

	if (!data->configured) {
		LOG_ERR("channel not initialized");
		return -EINVAL;
	}

	if (channel != 0) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= 4096) {
		LOG_ERR("value %d out of range", value);
		return -EINVAL;
	}

	DAC12_SetData(config->base, value);

	return 0;
}

static DEVICE_API(dac, mcux_dac12_driver_api) = {
	.channel_setup = mcux_dac12_channel_setup,
	.write_value = mcux_dac12_write_value,
};

#define TO_DAC12_VREF_SRC(val) _DO_CONCAT(kDAC12_ReferenceVoltageSourceAlt, val)

#define MCUX_DAC12_INIT(n) \
	static struct mcux_dac12_data mcux_dac12_data_##n;		\
									\
	static const struct mcux_dac12_config mcux_dac12_config_##n = {	\
		.base = (DAC_Type *)DT_INST_REG_ADDR(n),		\
		.reference =						\
		TO_DAC12_VREF_SRC(DT_INST_PROP(n, voltage_reference)),	\
		.buffered = DT_INST_PROP(n, buffered),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,				\
			&mcux_dac12_data_##n,				\
			&mcux_dac12_config_##n,				\
			POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,		\
			&mcux_dac12_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DAC12_INIT)
