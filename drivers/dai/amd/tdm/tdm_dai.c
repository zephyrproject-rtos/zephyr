/**
 * SPDX-FileCopyrightText: Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#endif

#define DT_DRV_COMPAT amd_tdm_dai

/** @brief Number of TDM DAI instances supported: HS=0, SP=1, BT=2. */
#define ACP_TDM_DAI_INSTANCES   3
#define ACP_DEFAULT_SAMPLE_RATE 48000U /**< Default sample rate, in Hz. */
#define ACP_DEFAULT_TDM_SLOTS   2U    /**< Default number of TDM slots per frame. */

/* Invalid/unassigned sentinel value */
#define ACP_TDM_INVALID_16	0xFFFFU

/**
 * @brief ACP-specific TDM DAI parameters.
 *
 * Passed as @p spec_config to dai_config_set(). Matches the layout of
 * @c sof_ipc_dai_acp_params in the SOF reference driver.
 */
struct dai_amd_tdm_params {
	uint32_t reserved0;  /**< Reserved, must be zero. */
	uint32_t fsync_rate; /**< Sample rate in Hz (e.g. 48000). */
	uint32_t tdm_slots;  /**< Number of TDM slots per frame (e.g. 2, 4, 8). */
	uint32_t tdm_mode;   /**< TDM mode selector. */
} __packed __aligned(4);

/**
 * @brief Per-instance TDM DAI configuration (stored in @c dev->config).
 *
 * Holds runtime-mutable DAI state set by dai_config_set() and read back
 * by dai_config_get().
 */
struct acp_tdm_dai_config {
	enum dai_type type;                    /**< DAI type; always @c DAI_AMD_TDM. */
	uint32_t dai_index;                    /**< Instance index: 0=HS, 1=SP, 2=BT. */
	struct dai_properties *tx_props;       /**< TX DMA handshake properties. */
	struct dai_properties *rx_props;       /**< RX DMA handshake properties. */
	struct dai_amd_tdm_params *acp_params; /**< Pointer to runtime TDM parameters. */
};

LOG_MODULE_REGISTER(acp_tdm_dai, CONFIG_DAI_LOG_LEVEL);

static int acp_tdm_dai_get_config(const struct device *dev, struct dai_config *cfg,
				   enum dai_dir dir)
{
	struct acp_tdm_dai_config *acp_cfg = (struct acp_tdm_dai_config *)dev->config;

	cfg->channels = acp_cfg->acp_params->tdm_slots;
	cfg->type = acp_cfg->type;
	cfg->dai_index = acp_cfg->dai_index;
	cfg->rate = acp_cfg->acp_params->fsync_rate;

	return 0;
}

static int acp_tdm_dai_set_config(const struct device *dev, const struct dai_config *cfg,
				 const void *spec_config, size_t spec_config_size)
{
	struct acp_tdm_dai_config *acp_cfg = (struct acp_tdm_dai_config *)dev->config;
	const struct dai_amd_tdm_params *params = spec_config;

	union acp_tdm_iter tdm_iter;
	union acp_tdm_irer tdm_irer;
	union acp_tdm_mstrclkgen tdm_mstrclkgen;
	uint32_t tdm_inst = ACP_TDM_INVALID_16;
	uint32_t tdm_offset = 0U;
	uint32_t mstrclkgen_off = 0U;

	if (cfg->type != DAI_AMD_TDM) {
		LOG_ERR("unsupported DAI type %u", cfg->type);
		return -EINVAL;
	}

	acp_cfg->type = cfg->type;
	acp_cfg->dai_index = cfg->dai_index;

	if (params && params->fsync_rate) {
		acp_cfg->acp_params->fsync_rate = params->fsync_rate;
	}
	acp_cfg->acp_params->tdm_slots = params->tdm_slots;
	LOG_DBG("tdm config: tdm_inst=%u rate=%u slots=%u format=0x%x", acp_cfg->dai_index,
		acp_cfg->acp_params->fsync_rate, acp_cfg->acp_params->tdm_slots, cfg->format);

	tdm_inst = acp_cfg->dai_index;
	if (tdm_inst >= ACP_TDM_DAI_INSTANCES) {
		LOG_ERR("tdm config: tdm_inst=%u out of range", tdm_inst);
		return -EINVAL;
	}

	switch (tdm_inst) {
	case 0: /* HS */
		tdm_offset = ACP_TDM0_OFFSET;
		mstrclkgen_off = ACP_TDM0_OFFSET + 8U;
		break;
	case 1: /* SP */
		tdm_offset = ACP_TDM1_OFFSET;
		mstrclkgen_off = ACP_TDM0_OFFSET;
		break;
	case 2: /* BT */
		tdm_offset = ACP_TDM2_OFFSET;
		mstrclkgen_off = ACP_TDM0_OFFSET + 4U;
		break;
	default:
		LOG_ERR("tdm config: invalid tdm_inst %u", tdm_inst);
		return -EINVAL;
	}

	/* 1. Configure MSTRCLKGEN first (clock must be set before protocol) */
	tdm_mstrclkgen.u32all = io_reg_read(PU_REGISTER_BASE + ACP_TDM_MSTRCLKGEN + mstrclkgen_off);
	tdm_mstrclkgen.bits.tdm_master_mode = 1U;
	switch (cfg->format & DAI_FORMAT_PROTOCOL_MASK) {
	case DAI_PROTO_DSP_A:
		tdm_mstrclkgen.bits.tdm_format_mode = 1U;
		switch (acp_cfg->acp_params->tdm_slots) {
		case 2:
			tdm_mstrclkgen.bits.tdm_lrclk_div_val = 0x20U;
			tdm_mstrclkgen.bits.tdm_bclk_div_val = 0x80U;
			break;
		case 4:
			tdm_mstrclkgen.bits.tdm_lrclk_div_val = 0x40U;
			tdm_mstrclkgen.bits.tdm_bclk_div_val = 0x40U;
			break;
		case 6:
			tdm_mstrclkgen.bits.tdm_lrclk_div_val = 0x60U;
			tdm_mstrclkgen.bits.tdm_bclk_div_val = 0x30U;
			break;
		case 8:
			tdm_mstrclkgen.bits.tdm_lrclk_div_val = 0x80U;
			tdm_mstrclkgen.bits.tdm_bclk_div_val = 0x20U;
			break;
		default:
			LOG_ERR("unsupported tdm_slots %u for DSP_A",
				acp_cfg->acp_params->tdm_slots);
			return -EINVAL;
		}
		break;

	case DAI_PROTO_I2S:
	default:
		tdm_mstrclkgen.bits.tdm_format_mode = 0U;
		tdm_mstrclkgen.bits.tdm_lrclk_div_val = 0x20U;
		tdm_mstrclkgen.bits.tdm_bclk_div_val = 0x80U;
		break;
	}
	io_reg_write(PU_REGISTER_BASE + ACP_TDM_MSTRCLKGEN + mstrclkgen_off, tdm_mstrclkgen.u32all);

	/* 2. Read ITER/IRER */
	tdm_iter.u32all = io_reg_read(PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset);
	tdm_irer.u32all = io_reg_read(PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset);

	switch (cfg->format & DAI_FORMAT_PROTOCOL_MASK) {
	case DAI_PROTO_DSP_A: {
		/* 3a. Write FRMT registers (only HS and SP have them) */
		union acp_tdm_frmt tdm_txfrmt;
		union acp_tdm_frmt tdm_rxfrmt;

		tdm_txfrmt.u32all =
			io_reg_read(PU_REGISTER_BASE + ACP_TDM_TXFRMT + tdm_offset);
		tdm_txfrmt.bits.num_slots = acp_cfg->acp_params->tdm_slots;
		tdm_txfrmt.bits.slot_len = 16U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_TXFRMT + tdm_offset, tdm_txfrmt.u32all);

		tdm_rxfrmt.u32all =
			io_reg_read(PU_REGISTER_BASE + ACP_TDM_RXFRMT + tdm_offset);
		tdm_rxfrmt.bits.num_slots = acp_cfg->acp_params->tdm_slots;
		tdm_rxfrmt.bits.slot_len = 16U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_RXFRMT + tdm_offset, tdm_rxfrmt.u32all);

		tdm_iter.bits.tdm_tx_protocol_mode = 1U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset, tdm_iter.u32all);

		tdm_irer.bits.tdm_rx_protocol_mode = 1U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset, tdm_irer.u32all);
		break;
	}

	case DAI_PROTO_I2S:
	default:
		tdm_iter.bits.tdm_tx_protocol_mode = 0U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset, tdm_iter.u32all);

		tdm_irer.bits.tdm_rx_protocol_mode = 0U;
		io_reg_write(PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset, tdm_irer.u32all);
		break;
	}

	return 0;
}

static int acp_tdm_dai_trigger(const struct device *dev, enum dai_dir dir, enum dai_trigger_cmd cmd)
{
	return 0;
}

static int acp_tdm_dai_probe(const struct device *dev)
{
	return 0;
}

static int acp_tdm_dai_remove(const struct device *dev)
{
	return 0;
}

static const struct dai_properties *acp_tdm_dai_get_properties(const struct device *dev,
							      enum dai_dir direction, int stream_id)
{
	struct acp_tdm_dai_config *acp_cfg = (struct acp_tdm_dai_config *)dev->config;

	switch (direction) {
	case DAI_DIR_TX:
		acp_cfg->tx_props->dma_hs_id = acp_cfg->dai_index * 2U;
		return acp_cfg->tx_props;
	case DAI_DIR_RX:
		acp_cfg->rx_props->dma_hs_id = acp_cfg->dai_index * 2U + 1U;
		return acp_cfg->rx_props;
	default:
		return NULL;
	}
}

static DEVICE_API(dai, acp_tdm_dai_driver_ops) = {
	.probe = acp_tdm_dai_probe,
	.remove = acp_tdm_dai_remove,
	.config_set = acp_tdm_dai_set_config,
	.config_get = acp_tdm_dai_get_config,
	.get_properties = acp_tdm_dai_get_properties,
	.trigger = acp_tdm_dai_trigger,
};

static int acp_tdm_dai_init(const struct device *dev)
{
	return 0;
}

#define ACP_TDM_DAI_INIT(inst)                                                     \
	static struct dai_amd_tdm_params acp_params_##inst = {                            \
		.fsync_rate = ACP_DEFAULT_SAMPLE_RATE,                                      \
		.tdm_slots  = ACP_DEFAULT_TDM_SLOTS,                                       \
		.tdm_mode   = 0,                                                            \
	};                                                                                \
	static struct dai_properties acp_tx_props_##inst = {                              \
		.dma_hs_id = DT_INST_PROP_OR(inst, dai_index, 0) * 2,                      \
	};                                                                                \
	static struct dai_properties acp_rx_props_##inst = {                              \
		.dma_hs_id = DT_INST_PROP_OR(inst, dai_index, 0) * 2 + 1,                  \
	};                                                                                \
	static struct acp_tdm_dai_config acp_tdm_cfg_##inst = {                           \
		.type       = DAI_AMD_TDM,                                                  \
		.dai_index  = DT_INST_PROP_OR(inst, dai_index, 0),                          \
		.tx_props   = &acp_tx_props_##inst,                                         \
		.rx_props   = &acp_rx_props_##inst,                                         \
		.acp_params = &acp_params_##inst,                                           \
	};                                                                                \
	DEVICE_DT_INST_DEFINE(inst, &acp_tdm_dai_init, NULL, NULL,                       \
			      &acp_tdm_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_DAI_INIT_PRIORITY, &acp_tdm_dai_driver_ops);

DT_INST_FOREACH_STATUS_OKAY(ACP_TDM_DAI_INIT);
