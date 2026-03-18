/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for Infineon AutAnalog CTB (Continuous Time Block).
 *
 * The CTB contains two opamps (OA0, OA1) that share a combined PDL
 * configuration structure (cy_stc_autanalog_ctb_t).  This MFD driver
 * aggregates both opamps into the single struct and calls
 * Cy_AutAnalog_CTB_LoadConfig to apply the combined configuration.
 *
 * Each opamp can be configured in various topologies (comparator, PGA, TIA,
 * buffer, etc.) via devicetree.  The child opamp driver implements the
 * Zephyr opamp API.
 *
 * Dynamic switch-matrix configurations:
 *   Basic mode  – 1 slot per opamp auto-generated from each opamp child's
 *                 DT switch-matrix properties.
 *   Advanced mode – N slots per opamp from dyn-cfg child nodes under each
 *                   opamp; opamp0 slots first, then opamp1 slots.
 *
 * Child opamp nodes (compatible "infineon,autanalog-ctb-opamp") call the
 * ifx_autanalog_ctb_* APIs to update their individual settings.
 */

#define DT_DRV_COMPAT infineon_autanalog_ctb

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>
#include <infineon_autanalog_ctb.h>

#include "cy_pdl.h"

LOG_MODULE_REGISTER(mfd_infineon_autanalog_ctb, CONFIG_MFD_LOG_LEVEL);

#define IFX_AUTANALOG_CTB_OA_COUNT 2

/*
 * Derive the CTB hardware index from the register offset within the
 * AutAnalog subsystem.  CTBL0 sits at offset 0x00000 and CTBL1 at 0x10000,
 * giving a stride of 0x10000.
 */
#define CTB_REG_STRIDE 0x10000U
#define CTB_IDX(n)     ((uint8_t)(DT_INST_REG_ADDR(n) / CTB_REG_STRIDE))

struct ifx_autanalog_ctb_config {
	const struct device *parent;
	uint8_t ctb_idx;
};

struct ifx_autanalog_ctb_child {
	const struct device *dev;
	ifx_autanalog_ctb_child_isr_t isr;
};

struct ifx_autanalog_ctb_data {
	cy_stc_autanalog_ctb_sta_t static_cfg;
	cy_stc_autanalog_ctb_dyn_t dyn_cfg[CY_AUTANALOG_CTB_DYN_CFG_MAX];
	cy_stc_autanalog_ctb_t ctb_cfg;
	struct ifx_autanalog_ctb_child children[IFX_AUTANALOG_CTB_OA_COUNT];
};

int ifx_autanalog_ctb_set_static_cfg(const struct device *dev, uint8_t oa_idx, uint8_t power_mode,
				     uint8_t int_mode)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;
	struct ifx_autanalog_ctb_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (oa_idx >= IFX_AUTANALOG_CTB_OA_COUNT) {
		return -EINVAL;
	}

	if (oa_idx == 0) {
		data->static_cfg.pwrOpamp0 = (cy_en_autanalog_ctb_oa_pwr_t)power_mode;
		data->static_cfg.intComp0 = (cy_en_autanalog_ctb_comp_int_t)int_mode;
	} else {
		data->static_cfg.pwrOpamp1 = (cy_en_autanalog_ctb_oa_pwr_t)power_mode;
		data->static_cfg.intComp1 = (cy_en_autanalog_ctb_comp_int_t)int_mode;
	}

	result = Cy_AutAnalog_CTB_LoadConfig(cfg->ctb_idx, &data->ctb_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load CTB config: %d", result);
		return -EIO;
	}

	return 0;
}

int ifx_autanalog_ctb_set_dynamic_cfg(const struct device *dev, uint8_t dyn_cfg_idx,
				      const struct ifx_autanalog_ctb_dyn_cfg *dyn_cfg)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;
	struct ifx_autanalog_ctb_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (dyn_cfg_idx >= CY_AUTANALOG_CTB_DYN_CFG_MAX) {
		return -EINVAL;
	}

	data->dyn_cfg[dyn_cfg_idx].ninvInpPin =
		(cy_en_autanalog_ctb_oa_ninv_pin_t)dyn_cfg->ninv_inp_pin;
	data->dyn_cfg[dyn_cfg_idx].ninvInpRef =
		(cy_en_autanalog_ctb_oa_ninv_ref_t)dyn_cfg->ninv_inp_ref;
	data->dyn_cfg[dyn_cfg_idx].invInpPin =
		(cy_en_autanalog_ctb_oa_inv_pin_t)dyn_cfg->inv_inp_pin;
	data->dyn_cfg[dyn_cfg_idx].resInpPin =
		(cy_en_autanalog_ctb_oa_res_pin_t)dyn_cfg->res_inp_pin;
	data->dyn_cfg[dyn_cfg_idx].resInpRef =
		(cy_en_autanalog_ctb_oa_res_ref_t)dyn_cfg->res_inp_ref;
	data->dyn_cfg[dyn_cfg_idx].sharedMuxIn =
		(cy_en_autanalog_ctb_oa_mux_in_t)dyn_cfg->shared_mux_in;
	data->dyn_cfg[dyn_cfg_idx].sharedMuxOut =
		(cy_en_autanalog_ctb_oa_mux_out_t)dyn_cfg->shared_mux_out;
	data->dyn_cfg[dyn_cfg_idx].outToPin = dyn_cfg->out_to_pin;

	result = Cy_AutAnalog_CTB_LoadConfig(cfg->ctb_idx, &data->ctb_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load CTB config: %d", result);
		return -EIO;
	}

	return 0;
}

uint8_t ifx_autanalog_ctb_get_index(const struct device *dev)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;

	return cfg->ctb_idx;
}

/** Interrupt cause masks for each CTB's comparators, indexed [ctb_idx][oa_idx] */
static const uint32_t ctb_comp_intr_mask[2][IFX_AUTANALOG_CTB_OA_COUNT] = {
	{CY_AUTANALOG_INT_CTB0_COMP0, CY_AUTANALOG_INT_CTB0_COMP1},
	{CY_AUTANALOG_INT_CTB1_COMP0, CY_AUTANALOG_INT_CTB1_COMP1},
};

/**
 * @brief CTB MFD ISR — dispatches to the correct child opamp
 *
 * Called by the parent AutAnalog MFD when any CTB interrupt fires.
 * Reads the interrupt cause and fans out to opamp0 and/or opamp1 handlers.
 */
static void autanalog_ctb_isr(const struct device *dev)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;
	struct ifx_autanalog_ctb_data *data = dev->data;
	uint32_t int_source = Cy_AutAnalog_GetInterruptCause();

	for (int i = 0; i < IFX_AUTANALOG_CTB_OA_COUNT; i++) {
		if ((int_source & ctb_comp_intr_mask[cfg->ctb_idx][i]) != 0 &&
		    data->children[i].isr != NULL) {
			data->children[i].isr(data->children[i].dev);
		}
	}
}

void ifx_autanalog_ctb_set_irq_handler(const struct device *dev, const struct device *child_dev,
				       uint8_t oa_idx, ifx_autanalog_ctb_child_isr_t handler)
{
	struct ifx_autanalog_ctb_data *data = dev->data;

	if (oa_idx >= IFX_AUTANALOG_CTB_OA_COUNT) {
		LOG_ERR("Invalid oa_idx: %u", oa_idx);
		return;
	}

	data->children[oa_idx].dev = child_dev;
	data->children[oa_idx].isr = handler;
}

int ifx_autanalog_ctb_comp_read(const struct device *dev, uint8_t oa_idx)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;

	if (oa_idx >= IFX_AUTANALOG_CTB_OA_COUNT) {
		return -EINVAL;
	}

	return Cy_AutAnalog_CTB_Comp_Read(cfg->ctb_idx, oa_idx) ? 1 : 0;
}

/* clang-format off */

/**
 * @brief Check if an opamp child node exists at a given index
 */
#define OA_CHILD_NODE(n, idx)   DT_CHILD(DT_DRV_INST(n), opamp_##idx)
#define OA_CHILD_EXISTS(n, idx) DT_NODE_EXISTS(OA_CHILD_NODE(n, idx))

/** Get a DT property from the opamp child, or use a default */
#define OA_CHILD_PROP_OR(n, idx, prop, default_val)                                           \
	COND_CODE_1(OA_CHILD_EXISTS(n, idx),                                                      \
		    (DT_PROP(OA_CHILD_NODE(n, idx), prop)),                                        \
		    (default_val))

/** Get a boolean DT property from the opamp child */
#define OA_CHILD_BOOL(n, idx, prop)                                                           \
	COND_CODE_1(OA_CHILD_EXISTS(n, idx),                                                      \
		    (DT_PROP(OA_CHILD_NODE(n, idx), prop)),                                        \
		    (false))

/* ===== Dynamic configuration pool macros ===== */

/**
 * Build one dynamic config struct from a dyn-cfg child node.
 * Used with DT_FOREACH_CHILD to iterate over all dyn-cfg children.
 */
#define DYN_CFG_FROM_CHILD(child)                                                              \
	{                                                                                          \
		.ninvInpPin = (cy_en_autanalog_ctb_oa_ninv_pin_t)DT_PROP(child, ninv_inp_pin),     \
		.ninvInpRef = (cy_en_autanalog_ctb_oa_ninv_ref_t)DT_PROP(child, ninv_inp_ref),     \
		.invInpPin = (cy_en_autanalog_ctb_oa_inv_pin_t)DT_PROP(child, inv_inp_pin),        \
		.resInpPin = (cy_en_autanalog_ctb_oa_res_pin_t)DT_PROP(child, res_inp_pin),        \
		.resInpRef = (cy_en_autanalog_ctb_oa_res_ref_t)DT_PROP(child, res_inp_ref),        \
		.sharedMuxIn = (cy_en_autanalog_ctb_oa_mux_in_t)DT_PROP(child, shared_mux_in),    \
		.sharedMuxOut = (cy_en_autanalog_ctb_oa_mux_out_t)DT_PROP(child, shared_mux_out), \
		.outToPin = DT_PROP(child, out_to_pin),                                            \
	},

/**
 * Number of dynamic config slots contributed by opamp child idx.
 * Advanced mode: number of dyn-cfg child nodes under the opamp.
 * Basic mode (no children): 1 slot from the opamp's own properties.
 * Missing opamp: 1 zero-initialized slot (hardware always requires
 * at least IFX_AUTANALOG_CTB_OA_COUNT slots).
 */
#define OA_DYN_CFG_COUNT(n, idx)                                                               \
	COND_CODE_1(OA_CHILD_EXISTS(n, idx),                                                       \
		    (COND_CODE_0(UTIL_BOOL(DT_CHILD_NUM(OA_CHILD_NODE(n, idx))),                   \
				(1),                                                               \
				(DT_CHILD_NUM(OA_CHILD_NODE(n, idx))))),                           \
		    (1))

/**
 * Total number of dynamic config slots across both opamps.
 */
#define NUM_DYN_CONFIGS(n) (OA_DYN_CFG_COUNT(n, 0) + OA_DYN_CFG_COUNT(n, 1))

/**
 * Emit dynamic config initializers for opamp child idx.
 * Advanced mode: iterate dyn-cfg child nodes.
 * Basic mode: single slot from the opamp's own switch-matrix properties.
 */
#define OA_DYN_CFGS(n, idx)                                                                    \
	COND_CODE_1(OA_CHILD_EXISTS(n, idx),                                                       \
		    (COND_CODE_0(UTIL_BOOL(DT_CHILD_NUM(OA_CHILD_NODE(n, idx))),                   \
				(BASIC_DYN_CFG(n, idx),),                                          \
				(DT_FOREACH_CHILD(OA_CHILD_NODE(n, idx), DYN_CFG_FROM_CHILD)))),   \
		    (BASIC_DYN_CFG(n, idx),))

/** Basic mode: slot 0 from opamp0 DT props, slot 1 from opamp1 DT props */
#define BASIC_DYN_CFG(n, idx)                                                                  \
	{                                                                                          \
		.ninvInpPin = (cy_en_autanalog_ctb_oa_ninv_pin_t)OA_CHILD_PROP_OR(                 \
			n, idx, ninv_inp_pin, 0),                                                  \
		.ninvInpRef = (cy_en_autanalog_ctb_oa_ninv_ref_t)OA_CHILD_PROP_OR(                 \
			n, idx, ninv_inp_ref, 0),                                                  \
		.invInpPin = (cy_en_autanalog_ctb_oa_inv_pin_t)OA_CHILD_PROP_OR(n, idx,            \
										inv_inp_pin, 0),   \
		.resInpPin = (cy_en_autanalog_ctb_oa_res_pin_t)OA_CHILD_PROP_OR(n, idx,            \
										res_inp_pin, 0),   \
		.resInpRef = (cy_en_autanalog_ctb_oa_res_ref_t)OA_CHILD_PROP_OR(n, idx,            \
										res_inp_ref, 0),   \
		.sharedMuxIn = (cy_en_autanalog_ctb_oa_mux_in_t)OA_CHILD_PROP_OR(                  \
			n, idx, shared_mux_in, 0),                                                 \
		.sharedMuxOut = (cy_en_autanalog_ctb_oa_mux_out_t)OA_CHILD_PROP_OR(                \
			n, idx, shared_mux_out, 0),                                                \
		.outToPin = OA_CHILD_BOOL(n, idx, out_to_pin),                                     \
	}

/* clang-format on */

static int ifx_autanalog_ctb_init(const struct device *dev)
{
	const struct ifx_autanalog_ctb_config *cfg = dev->config;
	struct ifx_autanalog_ctb_data *data = dev->data;
	cy_en_autanalog_status_t result;
	enum ifx_autanalog_periph periph;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("AutAnalog MFD parent not ready");
		return -ENODEV;
	}

	/* Load the combined CTB config into hardware */
	result = Cy_AutAnalog_CTB_LoadConfig(cfg->ctb_idx, &data->ctb_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to initialize CTB%u: %d", cfg->ctb_idx, result);
		return -EIO;
	}

	/* Register a single ISR with the parent for all CTB interrupts */
	periph = (cfg->ctb_idx == 0) ? IFX_AUTANALOG_PERIPH_CTB0 : IFX_AUTANALOG_PERIPH_CTB1;
	ifx_autanalog_set_irq_handler(cfg->parent, dev, periph, autanalog_ctb_isr);

	LOG_DBG("AutAnalog CTB%u MFD initialized", cfg->ctb_idx);
	return 0;
}

/* clang-format off */

#define IFX_AUTANALOG_CTB_INIT(n)                                                              \
	static struct ifx_autanalog_ctb_data ifx_autanalog_ctb_data_##n = {                        \
		.static_cfg =                                                                      \
			{                                                                          \
				.pwrOpamp0 = (cy_en_autanalog_ctb_oa_pwr_t)OA_CHILD_PROP_OR(       \
					n, 0, power_mode, 0),                                      \
				.topologyOpamp0 = (cy_en_autanalog_ctb_oa_topo_t)OA_CHILD_PROP_OR( \
					n, 0, topology, 0),                                        \
				.intComp0 = (cy_en_autanalog_ctb_comp_int_t)OA_CHILD_PROP_OR(      \
					n, 0, interrupt_mode, 0),                                  \
				.capFbOpamp0 = (cy_en_autanalog_ctb_oa_fb_cap_t)OA_CHILD_PROP_OR(  \
					n, 0, fb_cap, 0),                                          \
				.capCcOpamp0 = (cy_en_autanalog_ctb_oa_cc_cap_t)OA_CHILD_PROP_OR(  \
					n, 0, cc_cap, 8),                                          \
				.pwrOpamp1 = (cy_en_autanalog_ctb_oa_pwr_t)OA_CHILD_PROP_OR(       \
					n, 1, power_mode, 0),                                      \
				.topologyOpamp1 = (cy_en_autanalog_ctb_oa_topo_t)OA_CHILD_PROP_OR( \
					n, 1, topology, 0),                                        \
				.intComp1 = (cy_en_autanalog_ctb_comp_int_t)OA_CHILD_PROP_OR(      \
					n, 1, interrupt_mode, 0),                                  \
				.capFbOpamp1 = (cy_en_autanalog_ctb_oa_fb_cap_t)OA_CHILD_PROP_OR(  \
					n, 1, fb_cap, 0),                                          \
				.capCcOpamp1 = (cy_en_autanalog_ctb_oa_cc_cap_t)OA_CHILD_PROP_OR(  \
					n, 1, cc_cap, 8),                                          \
			},                                                                         \
		.dyn_cfg = {OA_DYN_CFGS(n, 0) OA_DYN_CFGS(n, 1)},                              \
			.ctb_cfg =                                                                 \
				{                                                                  \
					.ctbStaCfg = &ifx_autanalog_ctb_data_##n.static_cfg,       \
					.ctbDynCfgNum = NUM_DYN_CONFIGS(n),                        \
					.ctbDynCfgArr = ifx_autanalog_ctb_data_##n.dyn_cfg,        \
				},                                                                 \
	};                                                                                         \
                                                                                               \
	static const struct ifx_autanalog_ctb_config ifx_autanalog_ctb_config_##n = {              \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
		.ctb_idx = CTB_IDX(n),                                                             \
	};                                                                                         \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &ifx_autanalog_ctb_init, NULL, &ifx_autanalog_ctb_data_##n,       \
			      &ifx_autanalog_ctb_config_##n, POST_KERNEL,                          \
			      CONFIG_MFD_INFINEON_AUTANALOG_CTB_INIT_PRIORITY, NULL);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_CTB_INIT)
