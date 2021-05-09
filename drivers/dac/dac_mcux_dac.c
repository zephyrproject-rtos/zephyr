/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_dac

#include <zephyr.h>
#include <drivers/dac.h>
#include <logging/log.h>

#include <fsl_dac.h>

LOG_MODULE_REGISTER(dac_mcux_dac, CONFIG_DAC_LOG_LEVEL);

struct mcux_dac_config {
	DAC_Type *base;
	dac_reference_voltage_source_t reference;
	bool low_power;
};

struct mcux_dac_data {
	bool configured;
};

static int mcux_dac_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct mcux_dac_config *config = dev->config;
	struct mcux_dac_data *data = dev->data;
	dac_config_t dac_config;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != 12) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	DAC_GetDefaultConfig(&dac_config);
	dac_config.enableLowPowerMode = config->low_power;
	dac_config.referenceVoltageSource = config->reference;

	DAC_Init(config->base, &dac_config);

	data->configured = true;

	return 0;
}

static int mcux_dac_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct mcux_dac_config *config = dev->config;
	struct mcux_dac_data *data = dev->data;

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

	/* Static operation */
	DAC_EnableBuffer(config->base, false);

	DAC_SetBufferValue(config->base, 0, value);
	DAC_Enable(config->base, true);

	return 0;
}

static int mcux_dac_init(const struct device *dev)
{
	return 0;
}

static const struct dac_driver_api mcux_dac_driver_api = {
	.channel_setup = mcux_dac_channel_setup,
	.write_value = mcux_dac_write_value,
};

#define TO_DAC_VREF_SRC(val) \
	_DO_CONCAT(kDAC_ReferenceVoltageSourceVref, val)

#define MCUX_DAC_INIT(n) \
	static struct mcux_dac_data mcux_dac_data_##n;			\
									\
	static const struct mcux_dac_config mcux_dac_config_##n = {	\
		.base = (DAC_Type *)DT_INST_REG_ADDR(n),		\
		.reference =						\
		TO_DAC_VREF_SRC(DT_INST_PROP(n, voltage_reference)),	\
		.low_power = DT_INST_PROP(n, low_power_mode),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, mcux_dac_init, device_pm_control_nop,	\
			&mcux_dac_data_##n,				\
			&mcux_dac_config_##n,				\
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
			&mcux_dac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DAC_INIT)
