/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_dac

#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include "r_dac_api.h"
#include "r_dac.h"

LOG_MODULE_REGISTER(dac_renesas_ra, CONFIG_DAC_LOG_LEVEL);

#define HAS_CHARGEPUMP       DT_PROP(DT_PARENT(DT_DRV_INST(0)), has_chargepump)
#define HAS_OUTPUT_AMPLIFIER DT_PROP(DT_PARENT(DT_DRV_INST(0)), has_output_amplifier)
#define HAS_INTERNAL_OUTPUT  DT_PROP(DT_PARENT(DT_DRV_INST(0)), has_internal_output)

struct dac_renesas_ra_config {
	const struct pinctrl_dev_config *pcfg;
};

struct dac_renesas_ra_data {
	const struct device *dev;
	dac_instance_ctrl_t dac;
	struct st_dac_cfg f_config;
};

static int dac_renesas_ra_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	struct dac_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;

	if (channel != 0) {
		LOG_ERR("wrong channel id '%hhu'", channel);
		return -ENOTSUP;
	}

	fsp_err = R_DAC_Write(&data->dac, value);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	return 0;
}

static int dac_renesas_ra_channel_setup(const struct device *dev,
					const struct dac_channel_cfg *channel_cfg)
{
	struct dac_renesas_ra_data *data = dev->data;
#if (HAS_OUTPUT_AMPLIFIER || HAS_CHARGEPUMP || HAS_INTERNAL_OUTPUT)
	dac_extended_cfg_t *config_extend = (dac_extended_cfg_t *)data->f_config.p_extend;
#endif
	fsp_err_t fsp_err;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("wrong channel id '%hhu'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != 12) {
		LOG_ERR("Resolution not supported");
		return -ENOTSUP;
	}

	if (data->dac.channel_opened != 0) {
		fsp_err = R_DAC_Close(&data->dac);
		if (FSP_SUCCESS != fsp_err) {
			return -EIO;
		}
	}

#if HAS_OUTPUT_AMPLIFIER
	config_extend->output_amplifier_enabled = channel_cfg->buffered;
#elif HAS_CHARGEPUMP
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(moco))
	config_extend->enable_charge_pump = channel_cfg->buffered;
#else
	if (channel_cfg->buffered) {
		LOG_ERR("Requires MOCO clock enabled to support the buffer feature");
		return -ENOTSUP;
	}
#endif
#else
	if (channel_cfg->buffered) {
		LOG_ERR("The MCU doesn't support the buffer feature");
		return -ENOTSUP;
	}
#endif

#if HAS_INTERNAL_OUTPUT
	config_extend->internal_output_enabled = channel_cfg->internal;
#else
	if (channel_cfg->internal) {
		LOG_ERR("The MCU doesn't support the internal output feature");
		return -ENOTSUP;
	}
#endif

	fsp_err = R_DAC_Open(&data->dac, &data->f_config);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	fsp_err = R_DAC_Start(&data->dac);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	return 0;
}

static int dac_renesas_ra_init(const struct device *dev)
{
	const struct dac_renesas_ra_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(dac, dac_renesas_ra_api) = {
	.channel_setup = dac_renesas_ra_channel_setup,
	.write_value = dac_renesas_ra_write_value,
};

#ifdef CONFIG_DAC_RENESAS_RA_DAVREFCR_AVCC0_AVSS0
#define DAC_RENESAS_RA_DAVREFCR DAC_VREF_AVCC0_AVSS0
#elif defined(CONFIG_DAC_RENESAS_RA_DAVREFCR_VREFH_VREFL)
#define DAC_RENESAS_RA_DAVREFCR DAC_VREF_VREFH_VREFL
#elif defined(CONFIG_DAC_RENESAS_RA_DAVREFCR_NONE)
#define DAC_RENESAS_RA_DAVREFCR DAC_VREF_NONE
#else
#define DAC_RENESAS_RA_DAVREFCR 0
#endif

#define DAC_RENESAS_RA_INIT(idx)                                                                   \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static dac_extended_cfg_t g_dac_cfg_extend_##idx = {                                       \
		.data_format = DAC_DATA_FORMAT_FLUSH_RIGHT,                                        \
		.enable_charge_pump = true,                                                        \
		.output_amplifier_enabled = true,                                                  \
		.internal_output_enabled = false,                                                  \
		.ref_volt_sel = DAC_RENESAS_RA_DAVREFCR,                                           \
	};                                                                                         \
	static const struct dac_renesas_ra_config dac_renesas_ra_config_##idx = {                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
	};                                                                                         \
	static struct dac_renesas_ra_data dac_renesas_ra_data_##idx = {                            \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.f_config =                                                                        \
			{                                                                          \
				.channel = DT_INST_REG_ADDR(idx),                                  \
				.ad_da_synchronized =                                              \
					IS_ENABLED(CONFIG_DAC_RENESAS_RA_DA_AD_SYNCHRONIZE),       \
				.p_extend = &g_dac_cfg_extend_##idx,                               \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, dac_renesas_ra_init, NULL, &dac_renesas_ra_data_##idx,          \
			      &dac_renesas_ra_config_##idx, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY, \
			      &dac_renesas_ra_api)

DT_INST_FOREACH_STATUS_OKAY(DAC_RENESAS_RA_INIT);
