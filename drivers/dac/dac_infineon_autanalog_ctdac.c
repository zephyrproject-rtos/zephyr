/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DAC driver for Infineon AutAnalog CT DAC.
 *
 * This driver supports:
 * - FW channel (channel 15): Direct value writes via the Zephyr DAC
 *   API. This is the default channel for simple DAC output.
 * - Waveform channels (channels 0-14): Autonomous waveform output
 *   via the AC state machine. Channel IDs match the HW channel
 *   numbers directly. These channels use look-up table (LUT) ranges
 *   configured via DTS child nodes and waveform data loaded at
 *   runtime through the custom CTDAC API.
 *
 * The CTDAC is part of the AutAnalog subsystem and shares the
 * Autonomous Controller (AC) with other sub-peripherals (SAR ADC,
 * CTB, PTComp, PRB).
 */

#define DT_DRV_COMPAT infineon_autanalog_ctdac

#include <errno.h>

#include <zephyr/drivers/dac.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <infineon_autanalog.h>

#include "cy_pdl.h"

LOG_MODULE_REGISTER(ifx_autanalog_ctdac, CONFIG_DAC_LOG_LEVEL);

#define IFX_AUTANALOG_CTDAC_RESOLUTION            12
#define IFX_AUTANALOG_CTDAC_FW_CHANNEL            15
#define IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS 15

/* VDDA flag used in the combined vref-source encoding (see dt-bindings header) */
#define IFX_AUTANALOG_CTDAC_VREF_VDDA_FLAG 0x100

/* Per-waveform-channel DTS configuration (from child nodes) */
struct dac_ifx_autanalog_wf_ch_cfg {
	uint8_t reg; /* HW channel 0-14 (from DTS reg property) */
	uint16_t start_addr;
	uint16_t end_addr;
	uint8_t op_mode;
	bool sample_and_hold;
	uint8_t step_sel;
	uint8_t stat_sel;
};

/* Per-limit-slot DTS configuration */
struct dac_ifx_autanalog_ch_limit_cfg {
	uint8_t cond;
	int16_t low;
	int16_t high;
};

/* Read-only driver configuration */
struct dac_ifx_autanalog_config {
	const struct device *mfd;
	uint8_t dac_idx;
	uint16_t vref_source;
	uint16_t lp_div_dac;
	uint8_t topology;
	uint8_t ref_buf_pwr;
	uint8_t out_buf_pwr;
	bool deglitch;
	uint8_t deglitch_time;
	bool sign;
	bool bottom_sel;
	bool disabled_mode;
	uint8_t sample_time;
	uint8_t step_val[3];
	uint8_t limit_cfg_mask;
	struct dac_ifx_autanalog_ch_limit_cfg limit_cfg[3];
	uint8_t num_wf_channels;
	const struct dac_ifx_autanalog_wf_ch_cfg *wf_channels;
};

/* Runtime driver data */
struct dac_ifx_autanalog_data {
	uint8_t resolution;
	bool channel_setup_done;
	struct ifx_cat1_clock clock;
	/* PDL channel config structures for waveform channels (sized per instance) */
	cy_stc_autanalog_dac_ch_t *pdl_ch_cfg;
	/* PDL limit config structures (up to 3 limit slots) */
	cy_stc_autanalog_dac_ch_limit_t pdl_limit_cfg[3];
	/* PDL configuration structures */
	cy_stc_autanalog_dac_sta_t pdl_dac_static;
	cy_stc_autanalog_dac_t pdl_dac_cfg;
};

/**
 * @brief Initialize PDL structures for the DAC including waveform channel configs.
 */
static void dac_init_pdl_structs(struct dac_ifx_autanalog_data *data,
				 const struct dac_ifx_autanalog_config *cfg)
{
	cy_en_autanalog_dac_vref_sel_t vref_sel;
	cy_en_autanalog_dac_vref_mux_t vref_mux;

	if (cfg->vref_source == IFX_AUTANALOG_CTDAC_VREF_VDDA_FLAG) {
		vref_sel = CY_AUTANALOG_DAC_VREF_VDDA;
		vref_mux = CY_AUTANALOG_DAC_VREF_MUX_VBGR; /* don't care */
	} else {
		vref_sel = CY_AUTANALOG_DAC_VREF_MUX_OUT;
		vref_mux = (cy_en_autanalog_dac_vref_mux_t)cfg->vref_source;
	}

	/* Build per-channel PDL structures from DTS child node configs */
	for (uint8_t i = 0; i < cfg->num_wf_channels; i++) {
		const struct dac_ifx_autanalog_wf_ch_cfg *wf = &cfg->wf_channels[i];

		data->pdl_ch_cfg[i] = (cy_stc_autanalog_dac_ch_t){
			.startAddr = wf->start_addr,
			.endAddr = wf->end_addr,
			.operMode = wf->op_mode,
			.sampleAndHold = wf->sample_and_hold,
			.stepSel = wf->step_sel,
			.statSel = wf->stat_sel,
		};
	}

	/* Static configuration */
	data->pdl_dac_static = (cy_stc_autanalog_dac_sta_t){
		.lpDivDac = cfg->lp_div_dac,
		.topology = cfg->topology,
		.vrefSel = vref_sel,
		.deGlitch = cfg->deglitch,
		.bottomSel = cfg->bottom_sel,
		.disabledMode = cfg->disabled_mode,
		.refBuffPwr = (cy_en_autanalog_dac_ref_buf_pwr_t)cfg->ref_buf_pwr,
		.outBuffPwr = (cy_en_autanalog_dac_out_buf_pwr_t)cfg->out_buf_pwr,
		.sign = cfg->sign,
		.vrefMux = vref_mux,
		.sampleTime = cfg->sample_time,
		.stepVal = {cfg->step_val[0], cfg->step_val[1], cfg->step_val[2]},
		.deGlitchTime = cfg->deglitch_time,
		.chCfg = {NULL},
		.chLimitCfg = {NULL},
	};

	/* Wire waveform channel pointers into the static config (indexed by HW channel) */
	for (uint8_t i = 0; i < cfg->num_wf_channels; i++) {
		data->pdl_dac_static.chCfg[cfg->wf_channels[i].reg] = &data->pdl_ch_cfg[i];
	}

	/* Wire channel limit configurations */
	for (uint8_t i = 0; i < 3; i++) {
		if (cfg->limit_cfg_mask & BIT(i)) {
			data->pdl_limit_cfg[i] = (cy_stc_autanalog_dac_ch_limit_t){
				.cond = (cy_en_autanalog_dac_limit_t)cfg->limit_cfg[i].cond,
				.low = cfg->limit_cfg[i].low,
				.high = cfg->limit_cfg[i].high,
			};
			data->pdl_dac_static.chLimitCfg[i] = &data->pdl_limit_cfg[i];
		}
	}

	/* Top-level DAC configuration */
	data->pdl_dac_cfg = (cy_stc_autanalog_dac_t){
		.dacStaCfg = &data->pdl_dac_static,
		.waveform = NULL,
	};
}

static int dac_ifx_autanalog_channel_setup(const struct device *dev,
					   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;

	if (channel_cfg->channel_id > IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS) {
		LOG_ERR("Channel %d out of range (0..%d)", channel_cfg->channel_id,
			IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS);
		return -EINVAL;
	}

	if (channel_cfg->resolution != IFX_AUTANALOG_CTDAC_RESOLUTION) {
		LOG_ERR("Only %d-bit resolution is supported", IFX_AUTANALOG_CTDAC_RESOLUTION);
		return -ENOTSUP;
	}

	data->resolution = channel_cfg->resolution;

	/* Load the DAC configuration (static + any waveform channel configs) via PDL */
	cy_en_autanalog_status_t result =
		Cy_AutAnalog_DAC_LoadConfig(cfg->dac_idx, &data->pdl_dac_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load DAC config (err %d)", result);
		return -EIO;
	}

	data->channel_setup_done = true;

	LOG_DBG("CTDAC%d channel %d setup succeeded (resolution=%d)", cfg->dac_idx,
		channel_cfg->channel_id, channel_cfg->resolution);

	return 0;
}

static int dac_ifx_autanalog_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;

	if (!data->channel_setup_done) {
		LOG_ERR("Channel not set up");
		return -EINVAL;
	}

	if (value >= BIT(data->resolution)) {
		LOG_ERR("Value %d exceeds %d-bit range", value, data->resolution);
		return -EINVAL;
	}

	if (channel == IFX_AUTANALOG_CTDAC_FW_CHANNEL) {
		Cy_AutAnalog_DAC_WriteNextSample(cfg->dac_idx, (int32_t)value);
	} else {
		LOG_ERR("Channel %d uses waveforms and is not directly FW-controlled", channel);
		return -EINVAL;
	}

	return 0;
}

int dac_ifx_autanalog_ctdac_write_waveform(const struct device *dev, uint8_t channel,
					   const int32_t *samples, uint16_t num_samples)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (!data->channel_setup_done) {
		LOG_ERR("Channel not set up");
		return -EINVAL;
	}

	if (channel >= IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS) {
		LOG_ERR("Waveform write not supported on channel %d", channel);
		return -EINVAL;
	}

	if (Cy_AutAnalog_IsBusy()) {
		LOG_ERR("AC is currently in running state, cannot write waveform");
		return -EBUSY;
	}

	const struct dac_ifx_autanalog_wf_ch_cfg *wf = NULL;

	for (uint8_t i = 0; i < cfg->num_wf_channels; i++) {
		if (cfg->wf_channels[i].reg == channel) {
			wf = &cfg->wf_channels[i];
			break;
		}
	}
	if (wf == NULL) {
		LOG_ERR("Waveform channel %d not configured in DTS", channel);
		return -EINVAL;
	}

	uint16_t lut_slots = wf->end_addr - wf->start_addr + 1;

	if (num_samples > lut_slots) {
		LOG_ERR("Waveform too large (%d samples, LUT range is %d)", num_samples, lut_slots);
		return -EINVAL;
	}

	result = Cy_AutAnalog_DAC_Data_WriteWaveform(cfg->dac_idx, wf->start_addr, num_samples,
						     samples);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("PDL WriteWaveform failed (err %d)", result);
		return -EIO;
	}

	/* Set drive mode to Enabled for all written samples */
	Cy_AutAnalog_DAC_Data_SetDriveModeAll(cfg->dac_idx, CY_AUTANALOG_DAC_OUT_DRIVE_MODE_EN);

	LOG_DBG("CTDAC%d: wrote %d samples to LUT[%d..%d] for ch%d", cfg->dac_idx, num_samples,
		wf->start_addr, wf->start_addr + num_samples - 1, channel);

	return 0;
}

int dac_ifx_autanalog_ctdac_get_limit_status(const struct device *dev, uint8_t channel,
					     uint16_t *status)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;

	if (status == NULL) {
		LOG_ERR("Status pointer is null");
		return -EINVAL;
	}

	if (!data->channel_setup_done) {
		LOG_ERR("Channel not set up");
		return -EINVAL;
	}

	if (channel >= IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	*status = Cy_AutAnalog_DAC_GetLimitStatus(cfg->dac_idx) & BIT(channel);

	return 0;
}

int dac_ifx_autanalog_ctdac_clear_limit_status(const struct device *dev, uint8_t channel)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;

	if (!data->channel_setup_done) {
		LOG_ERR("Channel not set up");
		return -EINVAL;
	}

	if (channel >= IFX_AUTANALOG_CTDAC_MAX_WAVEFORM_CHANNELS) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	Cy_AutAnalog_DAC_ClearLimitStatus(cfg->dac_idx, BIT(channel));

	return 0;
}

static int dac_ifx_autanalog_init(const struct device *dev)
{
	const struct dac_ifx_autanalog_config *cfg = dev->config;
	struct dac_ifx_autanalog_data *data = dev->data;
	en_clk_dst_t clk_dst;
	cy_rslt_t clk_result;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("AutAnalog MFD device not ready");
		return -ENODEV;
	}

	data->channel_setup_done = false;

	/* Assign the peripheral clock divider to this CTDAC */
	clk_dst = (cfg->dac_idx == 0) ? PCLK_PASS_CLOCK_DAC0 : PCLK_PASS_CLOCK_DAC1;
	clk_result = ifx_cat1_utils_peri_pclk_assign_divider(clk_dst, &data->clock);
	if (clk_result != CY_RSLT_SUCCESS) {
		LOG_ERR("Failed to assign CTDAC%d peripheral clock (err %d)", cfg->dac_idx,
			(int)clk_result);
		return -EIO;
	}

	/* Initialize PDL structures from devicetree configuration */
	dac_init_pdl_structs(data, cfg);

	LOG_DBG("CTDAC%d initialized (%d waveform channels configured)", cfg->dac_idx,
		cfg->num_wf_channels);
	return 0;
}

static DEVICE_API(dac, dac_ifx_autanalog_api) = {
	.channel_setup = dac_ifx_autanalog_channel_setup,
	.write_value = dac_ifx_autanalog_write_value,
};

/* clang-format off */

/* Determine DAC index from the register address offset */
#define IFX_AUTANALOG_CTDAC_IDX(n) ((DT_INST_REG_ADDR(n) == 0x60000) ? 0 : 1)

/* Extract peripheral clock divider info from the DTS clocks phandle */
#define CTDAC_PERI_CLOCK_INIT(n)                                                               \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                         \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 0),                 \
			DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),                 \
			DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                             \
		.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                            \
	}

/*
 * Macros to extract waveform channel child node configurations.
 * Waveform channels have the "start-addr" property set; the FW channel
 * (channel@f) does not, so DT_NODE_HAS_PROP filters them at compile time.
 */
#define CTDAC_WF_CH_CFG(child)                                                                 \
	{                                                                                          \
		.reg = DT_REG_ADDR(child),                                                         \
		.start_addr = DT_PROP(child, start_addr),                                          \
		.end_addr = DT_PROP(child, end_addr),                                              \
		.op_mode = DT_PROP(child, op_mode),                                                \
		.sample_and_hold = DT_PROP(child, sample_and_hold),                                \
		.step_sel = DT_PROP(child, step_sel),                                              \
		.stat_sel = DT_PROP(child, stat_sel),                                              \
	},

#define CTDAC_WF_CH_CFG_IF_WAVEFORM(child)                                                     \
	COND_CODE_1(DT_NODE_HAS_PROP(child, start_addr), (CTDAC_WF_CH_CFG(child)), ())

#define CTDAC_COUNT_WF_IF_WAVEFORM(child)                                                      \
	COND_CODE_1(DT_NODE_HAS_PROP(child, start_addr), (+1), (+0))

#define CTDAC_NUM_WF_CHANNELS(n)                                                               \
	(0 DT_INST_FOREACH_CHILD_STATUS_OKAY(n, CTDAC_COUNT_WF_IF_WAVEFORM))

/* Ensure at least 1 element so the per-instance array is never zero-length */
#define CTDAC_PDL_CH_CFG_COUNT(n) (CTDAC_NUM_WF_CHANNELS(n) + ((CTDAC_NUM_WF_CHANNELS(n)) == 0))

#define CTDAC_CH_LIMIT_INIT(n, idx)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, limit_cfg_##idx),                                  \
		({ .cond = DT_INST_PROP_BY_IDX(n, limit_cfg_##idx, 0),                         \
		   .low = (int16_t)DT_INST_PROP_BY_IDX(n, limit_cfg_##idx, 1),                 \
		   .high = (int16_t)DT_INST_PROP_BY_IDX(n, limit_cfg_##idx, 2) }),             \
		({0}))

#define IFX_AUTANALOG_CTDAC_INIT(n)                                                            \
	static const struct dac_ifx_autanalog_wf_ch_cfg dac_ifx_autanalog_wf_cfg_##n[] = {         \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, CTDAC_WF_CH_CFG_IF_WAVEFORM) {            \
			0} /* sentinel for zero-length case */                                     \
	};                                                                                         \
	static cy_stc_autanalog_dac_ch_t dac_ifx_pdl_ch_cfg_##n[CTDAC_PDL_CH_CFG_COUNT(n)];        \
	static const struct dac_ifx_autanalog_config dac_ifx_autanalog_config_##n = {              \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.dac_idx = IFX_AUTANALOG_CTDAC_IDX(n),                                             \
		.vref_source = DT_INST_PROP(n, vref_source),                                       \
		.lp_div_dac = DT_INST_PROP(n, lp_div_dac),                                         \
		.topology = DT_INST_PROP(n, topology),                                             \
		.ref_buf_pwr = DT_INST_PROP(n, ref_buf_pwr),                                       \
		.out_buf_pwr = DT_INST_PROP(n, out_buf_pwr),                                       \
		.deglitch = DT_INST_PROP(n, deglitch),                                             \
		.deglitch_time = DT_INST_PROP(n, deglitch_time),                                   \
		.sign = DT_INST_PROP(n, sign),                                                     \
		.bottom_sel = DT_INST_PROP(n, bottom_sel),                                         \
		.disabled_mode = DT_INST_PROP(n, disabled_mode),                                   \
		.sample_time = DT_INST_PROP(n, sample_time),                                       \
		.step_val = {DT_INST_PROP_BY_IDX(n, step_val, 0),                                  \
			     DT_INST_PROP_BY_IDX(n, step_val, 1),                                  \
			     DT_INST_PROP_BY_IDX(n, step_val, 2)},                                 \
		.limit_cfg_mask = (DT_INST_NODE_HAS_PROP(n, limit_cfg_0) ? BIT(0) : 0) |           \
				  (DT_INST_NODE_HAS_PROP(n, limit_cfg_1) ? BIT(1) : 0) |           \
				  (DT_INST_NODE_HAS_PROP(n, limit_cfg_2) ? BIT(2) : 0),            \
		.limit_cfg = {CTDAC_CH_LIMIT_INIT(n, 0), CTDAC_CH_LIMIT_INIT(n, 1),                \
			      CTDAC_CH_LIMIT_INIT(n, 2)},                                          \
		.num_wf_channels = CTDAC_NUM_WF_CHANNELS(n),                                       \
		.wf_channels = dac_ifx_autanalog_wf_cfg_##n,                                       \
	};                                                                                         \
	static struct dac_ifx_autanalog_data dac_ifx_autanalog_data_##n = {                        \
		CTDAC_PERI_CLOCK_INIT(n),                                                          \
		.pdl_ch_cfg = dac_ifx_pdl_ch_cfg_##n,                                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &dac_ifx_autanalog_init, NULL, &dac_ifx_autanalog_data_##n,       \
			      &dac_ifx_autanalog_config_##n, POST_KERNEL,                          \
			      CONFIG_DAC_INFINEON_AUTANALOG_INIT_PRIORITY,                         \
			      &dac_ifx_autanalog_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_CTDAC_INIT)
