/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpdac

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

#include <fsl_dac.h>

LOG_MODULE_REGISTER(dac_mcux_lpdac, CONFIG_DAC_LOG_LEVEL);

struct mcux_lpdac_config {
	LPDAC_Type *base;
	dac_reference_voltage_source_t ref_voltage;
	bool low_power;
};

struct mcux_lpdac_data {
	bool configured;
};

static int mcux_lpdac_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct mcux_lpdac_config *config = dev->config;
	struct mcux_lpdac_data *data = dev->data;
	dac_config_t dac_config;

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

	DAC_GetDefaultConfig(&dac_config);
	dac_config.referenceVoltageSource = config->ref_voltage;
#if defined(FSL_FEATURE_LPDAC_HAS_GCR_BUF_SPD_CTRL) && FSL_FEATURE_LPDAC_HAS_GCR_BUF_SPD_CTRL
	dac_config.enableLowerLowPowerMode = config->low_power;
#else
	dac_config.enableLowPowerMode = config->low_power;
#endif
	DAC_Init(config->base, &dac_config);
	DAC_Enable(config->base, false);
	data->configured = true;

	return 0;
}

static int mcux_lpdac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct mcux_lpdac_config *config = dev->config;
	struct mcux_lpdac_data *data = dev->data;

	if (!data->configured) {
		LOG_ERR("channel not initialized");
		return -EINVAL;
	}

	if (channel != 0) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= 4096) {
		LOG_ERR("unsupported value %d", value);
		return -EINVAL;
	}

	DAC_Enable(config->base, true);
	DAC_SetData(config->base, value);

	return 0;
}

static int mcux_lpdac_init(const struct device *dev)
{
	return 0;
}

static const struct dac_driver_api mcux_lpdac_driver_api = {
	.channel_setup = mcux_lpdac_channel_setup,
	.write_value = mcux_lpdac_write_value,
};

#define MCUX_LPDAC_INIT(n)                                                                         \
	static struct mcux_lpdac_data mcux_lpdac_data_##n;                                         \
                                                                                                   \
	static const struct mcux_lpdac_config mcux_lpdac_config_##n = {                            \
		.base = (LPDAC_Type *)DT_INST_REG_ADDR(n),                                         \
		.ref_voltage = DT_INST_PROP(n, voltage_reference),                                 \
		.low_power = DT_INST_PROP(n, low_power_mode),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_lpdac_init, NULL, &mcux_lpdac_data_##n,                      \
			      &mcux_lpdac_config_##n, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,       \
			      &mcux_lpdac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_LPDAC_INIT)
