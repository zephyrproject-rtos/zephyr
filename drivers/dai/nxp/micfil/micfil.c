/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include "fsl_pdm.h"

#define DT_DRV_COMPAT nxp_dai_micfil
LOG_MODULE_REGISTER(nxp_dai_micfil);

#define MICFIL_CLK_ROOT 24576000
#define MICFIL_OSR_DEFAULT	16

#define UINT_TO_MICFIL(x) ((PDM_Type *)(uintptr_t)(x))

#define MICFIL_FIFO_BASE(inst) \
	POINTER_TO_UINT(&(UINT_TO_MICFIL(DT_INST_REG_ADDR(inst))->DATACH[0]))

#define MICFIL_DMA_HANDSHAKE(inst) \
	((DT_INST_DMAS_CELL_BY_IDX(inst, 0, channel) & GENMASK(7, 0)) | \
	((DT_INST_DMAS_CELL_BY_IDX(inst, 0, mux) << 8) & GENMASK(15, 8)))

struct dai_nxp_micfil_data {
	struct dai_config cfg;
};

struct dai_nxp_micfil_config {
	PDM_Type *base;
	const struct dai_properties *rx_props;
	const struct pinctrl_dev_config *pincfg;
};

/* this needs to match SOF struct sof_ipc_dai_micfil_params */
struct micfil_bespoke_config {
	uint32_t pdm_rate;
	uint32_t pdm_ch;
};

static void dai_nxp_micfil_trigger_start(const struct device *dev)
{
	const struct dai_nxp_micfil_config *cfg = dev->config;

	/* enable DMA requests */
	PDM_EnableDMA(cfg->base, true);
	/* enable the module */
	PDM_Enable(cfg->base, true);
}

static void dai_nxp_micfil_trigger_stop(const struct device *dev)
{
	const struct dai_nxp_micfil_config *cfg = dev->config;

	/* disable DMA requests */
	PDM_EnableDMA(cfg->base, false);
	/* disable module */
	PDM_Enable(cfg->base, false);
}

static const struct dai_properties
*dai_nxp_micfil_get_properties(const struct device *dev, enum dai_dir dir, int stream_id)
{
	const struct dai_nxp_micfil_config *cfg = dev->config;

	if (dir == DAI_DIR_RX) {
		return cfg->rx_props;
	}

	LOG_ERR("invalid direction %d", dir);
	return NULL;
}

static int dai_nxp_micfil_trigger(const struct device *dev, enum dai_dir dir,
				  enum dai_trigger_cmd cmd)
{
	if (dir != DAI_DIR_RX) {
		LOG_ERR("invalid direction %d", dir);
		return -EINVAL;
	}

	switch (cmd) {
	case DAI_TRIGGER_START:
		dai_nxp_micfil_trigger_start(dev);
		break;
	case DAI_TRIGGER_STOP:
	case DAI_TRIGGER_PAUSE:
		dai_nxp_micfil_trigger_stop(dev);
		break;
	case DAI_TRIGGER_PRE_START:
	case DAI_TRIGGER_COPY:
		return 0;
	default:
		LOG_ERR("invalid trigger cmd %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static int dai_nxp_micfil_get_config(const struct device *dev, struct dai_config *cfg,
				     enum dai_dir dir)
{
	struct dai_nxp_micfil_data *micfil_data = dev->data;

	memcpy(cfg, &micfil_data->cfg, sizeof(*cfg));
	return 0;
}

static int dai_nxp_micfil_set_config(const struct device *dev,
		const struct dai_config *cfg, const void *bespoke_cfg)

{
	const struct micfil_bespoke_config *bespoke = bespoke_cfg;
	const struct dai_nxp_micfil_config *micfil_cfg = dev->config;
	pdm_channel_config_t chan_config = { 0 };
	pdm_config_t global_config = { 0 };
	int ret, i;

	if (cfg->type != DAI_IMX_MICFIL) {
		LOG_ERR("wrong DAI type: %d", cfg->type);
		return -EINVAL;
	}

	global_config.fifoWatermark = micfil_cfg->rx_props->fifo_depth - 1;
	global_config.qualityMode = kPDM_QualityModeVeryLow0;
	global_config.cicOverSampleRate = MICFIL_OSR_DEFAULT;

	PDM_Init(micfil_cfg->base, &global_config);

	for (i = 0; i < bespoke->pdm_ch; i++) {
		chan_config.gain = kPDM_DfOutputGain2;
		chan_config.cutOffFreq = kPDM_DcRemoverBypass;
		PDM_SetChannelConfig(micfil_cfg->base, i, &chan_config);
	}

	ret = PDM_SetSampleRateConfig(micfil_cfg->base, MICFIL_CLK_ROOT, bespoke->pdm_rate);
	if (ret == kStatus_Fail) {
		LOG_ERR("Failure to set samplerate config rate %d", bespoke->pdm_rate);
		return -EINVAL;
	}

	return 0;
}

static int dai_nxp_micfil_probe(const struct device *dev)
{
	/* nothing do to here, but mandatory to exist */
	return 0;
}

static int dai_nxp_micfil_remove(const struct device *dev)
{
	/* nothing do to here, but mandatory to exist */
	return 0;
}

const struct dai_driver_api dai_nxp_micfil_ops = {
	.probe			= dai_nxp_micfil_probe,
	.remove			= dai_nxp_micfil_remove,
	.config_set		= dai_nxp_micfil_set_config,
	.config_get		= dai_nxp_micfil_get_config,
	.get_properties		= dai_nxp_micfil_get_properties,
	.trigger		= dai_nxp_micfil_trigger,
};

static int micfil_init(const struct device *dev)
{
	const struct dai_nxp_micfil_config *cfg = dev->config;
	int ret;

	/* pinctrl is optional so do not return an error if not defined */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

#define DAI_NXP_MICFIL_INIT(inst)						\
PINCTRL_DT_INST_DEFINE(inst);							\
static struct dai_nxp_micfil_data dai_nxp_micfil_data_##inst = {		\
	.cfg.type = DAI_IMX_MICFIL,						\
	.cfg.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),			\
};										\
										\
static const struct dai_properties micfil_rx_props_##inst = {			\
	.fifo_address = MICFIL_FIFO_BASE(inst),					\
	.fifo_depth = DT_INST_PROP(inst, fifo_depth),			\
	.dma_hs_id = MICFIL_DMA_HANDSHAKE(inst),				\
};										\
										\
static const struct dai_nxp_micfil_config dai_nxp_micfil_config_##inst = {	\
	.base = UINT_TO_MICFIL(DT_INST_REG_ADDR(inst)),				\
	.rx_props = &micfil_rx_props_##inst,					\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
};										\
										\
DEVICE_DT_INST_DEFINE(inst, &micfil_init, NULL,					\
	      &dai_nxp_micfil_data_##inst, &dai_nxp_micfil_config_##inst,	\
	      POST_KERNEL, CONFIG_DAI_INIT_PRIORITY,				\
	      &dai_nxp_micfil_ops);						\

DT_INST_FOREACH_STATUS_OKAY(DAI_NXP_MICFIL_INIT)
