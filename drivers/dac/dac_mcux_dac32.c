/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_dac32

#include <zephyr.h>
#include <drivers/dac.h>
#include <logging/log.h>

#include <fsl_dac32.h>

LOG_MODULE_REGISTER(dac_mcux_dac32, CONFIG_DAC_LOG_LEVEL);

struct mcux_dac32_config {
	DAC_Type *base;
	dac32_reference_voltage_source_t reference;
	bool buffered;
	bool low_power;
};

struct mcux_dac32_data {
	bool configured;
};

static int mcux_dac32_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct mcux_dac32_config *config = dev->config;
	struct mcux_dac32_data *data = dev->data;
	dac32_config_t dac32_config;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != 12) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	DAC32_GetDefaultConfig(&dac32_config);
	dac32_config.enableLowPowerMode = config->low_power;
	dac32_config.referenceVoltageSource = config->reference;

	DAC32_Init(config->base, &dac32_config);
	DAC32_EnableBufferOutput(config->base, config->buffered);

	DAC32_EnableTestOutput(config->base,
			       IS_ENABLED(CONFIG_DAC_MCUX_DAC32_TESTOUT));

	data->configured = true;

	return 0;
}

static int mcux_dac32_write_value(const struct device *dev, uint8_t channel,
				  uint32_t value)
{
	const struct mcux_dac32_config *config = dev->config;
	struct mcux_dac32_data *data = dev->data;

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
	DAC32_EnableBuffer(config->base, false);

	DAC32_SetBufferValue(config->base, 0, value);
	DAC32_Enable(config->base, true);

	return 0;
}

static int mcux_dac32_init(const struct device *dev)
{
	return 0;
}

static const struct dac_driver_api mcux_dac32_driver_api = {
	.channel_setup = mcux_dac32_channel_setup,
	.write_value = mcux_dac32_write_value,
};

#define TO_DAC32_VREF_SRC(val) \
	_DO_CONCAT(kDAC32_ReferenceVoltageSourceVref, val)

#define MCUX_DAC32_INIT(n) \
	static struct mcux_dac32_data mcux_dac32_data_##n;		\
									\
	static const struct mcux_dac32_config mcux_dac32_config_##n = {	\
		.base = (DAC_Type *)DT_INST_REG_ADDR(n),		\
		.reference =						\
		TO_DAC32_VREF_SRC(DT_INST_PROP(n, voltage_reference)),	\
		.buffered = DT_INST_PROP(n, buffered),			\
		.low_power = DT_INST_PROP(n, low_power_mode),		\
	};								\
									\
	DEVICE_AND_API_INIT(mcux_dac32_##n, DT_INST_LABEL(n),		\
			mcux_dac32_init, &mcux_dac32_data_##n,		\
			&mcux_dac32_config_##n,				\
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
			&mcux_dac32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_DAC32_INIT)
