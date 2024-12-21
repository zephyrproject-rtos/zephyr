/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_gau_dac

#include <zephyr/drivers/dac.h>

#include <fsl_dac.h>

#define LOG_LEVEL CONFIG_DAC_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(nxp_gau_dac);

struct nxp_gau_dac_config {
	DAC_Type *base;
	dac_conversion_rate_t conversion_rate : 2;
	dac_reference_voltage_source_t voltage_ref : 1;
	dac_output_voltage_range_t output_range : 2;
};

static inline dac_channel_id_t convert_channel_id(uint8_t channel_id)
{
	switch (channel_id) {
	case 0: return kDAC_ChannelA;
	case 1: return kDAC_ChannelB;
	default:
		LOG_ERR("Invalid DAC channel ID");
		return -EINVAL;
	};
}

static int nxp_gau_dac_channel_setup(const struct device *dev,
			const struct dac_channel_cfg *channel_cfg)
{
	const struct nxp_gau_dac_config *config = dev->config;
	dac_channel_config_t dac_channel_config = {0};
	bool use_internal = true;

	if (channel_cfg->resolution != 10) {
		LOG_ERR("DAC only support 10 bit resolution");
		return -EINVAL;
	}

	if (channel_cfg->internal && channel_cfg->buffered) {
		LOG_ERR("DAC output can not be buffered and internal");
		return -EINVAL;
	} else if (channel_cfg->buffered) {
		/* External and internal output are mutually exclusive */
		LOG_WRN("Note: buffering DAC output to pad disconnects internal output");
		use_internal = false;
	}

	dac_channel_config.waveType = kDAC_WaveNormal;
	dac_channel_config.outMode =
		use_internal ? kDAC_ChannelOutputInternal : kDAC_ChannelOutputPAD;
	dac_channel_config.timingMode = kDAC_NonTimingCorrelated;
	dac_channel_config.enableTrigger = false;
	dac_channel_config.enableDMA = false;
	dac_channel_config.enableConversion = true;

	DAC_SetChannelConfig(config->base,
			(uint32_t)convert_channel_id(channel_cfg->channel_id),
			&dac_channel_config);

	return 0;
};

static int nxp_gau_dac_write_value(const struct device *dev,
				uint8_t channel, uint32_t value)
{
	const struct nxp_gau_dac_config *config = dev->config;

	DAC_SetChannelData(config->base,
			(uint32_t)convert_channel_id(channel),
			(uint16_t)value);
	return 0;
};

static DEVICE_API(dac, nxp_gau_dac_driver_api) = {
	.channel_setup = nxp_gau_dac_channel_setup,
	.write_value = nxp_gau_dac_write_value,
};

static int nxp_gau_dac_init(const struct device *dev)
{
	const struct nxp_gau_dac_config *config = dev->config;
	dac_config_t dac_cfg;

	DAC_GetDefaultConfig(&dac_cfg);

	dac_cfg.conversionRate = config->conversion_rate;
	dac_cfg.refSource = config->voltage_ref;
	dac_cfg.rangeSelect = config->output_range;

	DAC_Init(config->base, &dac_cfg);

	return 0;
};

#define NXP_GAU_DAC_INIT(inst)							\
										\
	const struct nxp_gau_dac_config nxp_gau_dac_##inst##_config = {		\
		.base = (DAC_Type *) DT_INST_REG_ADDR(inst),			\
		.voltage_ref = DT_INST_ENUM_IDX(inst, nxp_dac_reference),	\
		.conversion_rate = DT_INST_ENUM_IDX(inst, nxp_conversion_rate),	\
		.output_range = DT_INST_ENUM_IDX(inst,				\
						nxp_output_voltage_range),	\
	};									\
										\
										\
	DEVICE_DT_INST_DEFINE(inst, &nxp_gau_dac_init, NULL,			\
				NULL,						\
				&nxp_gau_dac_##inst##_config,			\
				POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,		\
				&nxp_gau_dac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_GAU_DAC_INIT)
