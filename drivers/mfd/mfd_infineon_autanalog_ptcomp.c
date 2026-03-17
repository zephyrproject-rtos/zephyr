/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for Infineon AutAnalog PTComp (Programmable Threshold Comparator).
 *
 * The PTComp contains two comparators (Comp0, Comp1) that share a combined
 * PDL configuration structure (cy_stc_autanalog_ptcomp_t).  This MFD driver
 * aggregates both comparators into the single struct and calls
 * Cy_AutAnalog_PTComp_LoadConfig to apply the combined configuration.
 *
 * Dynamic mux-routing configurations:
 *   Basic mode  – 1 slot per comparator auto-generated from each comp child's
 *                 positive-mux / negative-mux properties.
 *   Advanced mode – N slots per comparator from dyn-cfg child nodes under each
 *                   comp; comp0 slots first, then comp1 slots.
 *
 * Post-processing is configured via optional pp-* properties on each comp
 * child node and is loaded into the shared static config.
 *
 * Child comparator nodes (compatible "infineon,autanalog-ptcomp-comp") call the
 * ifx_autanalog_ptcomp_* APIs to update their individual settings.
 */

#define DT_DRV_COMPAT infineon_autanalog_ptcomp

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>
#include <infineon_autanalog_ptcomp.h>

#include "cy_pdl.h"

LOG_MODULE_REGISTER(mfd_infineon_autanalog_ptcomp, CONFIG_MFD_LOG_LEVEL);

#define IFX_AUTANALOG_PTCOMP_COMP_COUNT 2

struct ifx_autanalog_ptcomp_config {
	const struct device *parent;
};

struct ifx_autanalog_ptcomp_child {
	const struct device *dev;
	ifx_autanalog_ptcomp_child_isr_t isr;
};

struct ifx_autanalog_ptcomp_data {
	cy_stc_autanalog_ptcomp_comp_sta_t static_cfg;
	cy_stc_autanalog_ptcomp_comp_pp_t pp_cfg[CY_AUTANALOG_PTCOMP_PP_CFG_MAX];
	cy_stc_autanalog_ptcomp_comp_dyn_t dyn_cfg[CY_AUTANALOG_PTCOMP_DYN_CFG_MAX];
	cy_stc_autanalog_ptcomp_t ptcomp_cfg;
	struct ifx_autanalog_ptcomp_child children[IFX_AUTANALOG_PTCOMP_COMP_COUNT];
};

int ifx_autanalog_ptcomp_set_static_cfg(const struct device *dev, uint8_t comp_idx,
					uint8_t power_mode, uint8_t hysteresis, uint8_t int_mode)
{
	struct ifx_autanalog_ptcomp_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (comp_idx >= IFX_AUTANALOG_PTCOMP_COMP_COUNT) {
		return -EINVAL;
	}

	if (comp_idx == 0) {
		data->static_cfg.powerModeComp0 = (cy_en_autanalog_ptcomp_comp_pwr_t)power_mode;
		data->static_cfg.compHystComp0 = (cy_en_autanalog_ptcomp_comp_hyst_t)hysteresis;
		data->static_cfg.compEdgeComp0 = (cy_en_autanalog_ptcomp_comp_int_t)int_mode;
	} else {
		data->static_cfg.powerModeComp1 = (cy_en_autanalog_ptcomp_comp_pwr_t)power_mode;
		data->static_cfg.compHystComp1 = (cy_en_autanalog_ptcomp_comp_hyst_t)hysteresis;
		data->static_cfg.compEdgeComp1 = (cy_en_autanalog_ptcomp_comp_int_t)int_mode;
	}

	result = Cy_AutAnalog_PTComp_LoadConfig(0, &data->ptcomp_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load PTComp config: %d", result);
		return -EIO;
	}

	return 0;
}

int ifx_autanalog_ptcomp_set_dynamic_cfg(const struct device *dev, uint8_t dyn_cfg_idx,
					 uint8_t positive_mux, uint8_t negative_mux)
{
	struct ifx_autanalog_ptcomp_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (dyn_cfg_idx >= CY_AUTANALOG_PTCOMP_DYN_CFG_MAX) {
		return -EINVAL;
	}

	data->dyn_cfg[dyn_cfg_idx].ninvInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)positive_mux;
	data->dyn_cfg[dyn_cfg_idx].invInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)negative_mux;

	result = Cy_AutAnalog_PTComp_LoadConfig(0, &data->ptcomp_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load PTComp config: %d", result);
		return -EIO;
	}

	return 0;
}

/** Interrupt cause masks for each comparator */
static const uint32_t ptcomp_comp_intr_mask[IFX_AUTANALOG_PTCOMP_COMP_COUNT] = {
	CY_AUTANALOG_INT_PTC0_COMP0 | CY_AUTANALOG_INT_PTC0_RANGE0,
	CY_AUTANALOG_INT_PTC0_COMP1 | CY_AUTANALOG_INT_PTC0_RANGE1,
};

/**
 * @brief PTCOMP MFD ISR — dispatches to the correct child comparator
 *
 * Called by the parent AutAnalog MFD when any PTCOMP interrupt fires.
 * Reads the interrupt cause and fans out to comp0 and/or comp1 handlers.
 */
static void autanalog_ptcomp_isr(const struct device *dev)
{
	struct ifx_autanalog_ptcomp_data *data = dev->data;
	uint32_t int_source = Cy_AutAnalog_GetInterruptCause();

	for (int i = 0; i < IFX_AUTANALOG_PTCOMP_COMP_COUNT; i++) {
		if ((int_source & ptcomp_comp_intr_mask[i]) != 0 && data->children[i].isr != NULL) {
			data->children[i].isr(data->children[i].dev);
		}
	}
}

void ifx_autanalog_ptcomp_set_irq_handler(const struct device *dev, const struct device *child_dev,
					  uint8_t comp_idx,
					  ifx_autanalog_ptcomp_child_isr_t handler)
{
	struct ifx_autanalog_ptcomp_data *data = dev->data;

	if (comp_idx >= IFX_AUTANALOG_PTCOMP_COMP_COUNT) {
		LOG_ERR("Invalid comp_idx: %u", comp_idx);
		return;
	}

	data->children[comp_idx].dev = child_dev;
	data->children[comp_idx].isr = handler;
}

int ifx_autanalog_ptcomp_read(const struct device *dev, uint8_t comp_idx)
{
	ARG_UNUSED(dev);

	if (comp_idx >= IFX_AUTANALOG_PTCOMP_COMP_COUNT) {
		return -EINVAL;
	}

	return Cy_AutAnalog_PTComp_Read(0, comp_idx) ? 1 : 0;
}

/* clang-format off */

/**
 * @brief Check if a comp child node exists and is enabled at a given index
 */
#define COMP_CHILD_NODE(n, idx)   DT_CHILD(DT_DRV_INST(n), comp_##idx)
#define COMP_CHILD_EXISTS(n, idx) DT_NODE_EXISTS(COMP_CHILD_NODE(n, idx))
#define COMP_CHILD_OKAY(n, idx)                                                                \
	(COMP_CHILD_EXISTS(n, idx) && DT_NODE_HAS_STATUS(COMP_CHILD_NODE(n, idx), okay))

/** Get a DT property from the comp child, or use a default */
#define COMP_CHILD_PROP_OR(n, idx, prop, default_val)                                          \
	COND_CODE_1(COMP_CHILD_EXISTS(n, idx), \
		    (DT_PROP(COMP_CHILD_NODE(n, idx), prop)), \
		    (default_val))

/** Get a boolean DT property from the comp child */
#define COMP_CHILD_BOOL(n, idx, prop)                                                          \
	COND_CODE_1(COMP_CHILD_EXISTS(n, idx), \
		    (DT_PROP(COMP_CHILD_NODE(n, idx), prop)), \
		    (0))

/* ===== Dynamic configuration pool macros ===== */

/**
 * Build one dynamic config struct from a dyn-cfg child node.
 * Used with DT_FOREACH_CHILD to iterate over all dyn-cfg children.
 */
#define DYN_CFG_FROM_CHILD(child)                                                      \
	{                                                                                  \
		.ninvInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)DT_PROP(child, positive_mux), \
		.invInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)DT_PROP(child, negative_mux),  \
	},

/**
 * Number of dynamic config slots contributed by comp child idx.
 * Advanced mode: number of dyn-cfg child nodes under the comparator.
 * Basic mode (no children): 1 slot from the comp's own properties.
 * Missing comp: 1 zero-initialized slot (hardware always requires
 * at least IFX_AUTANALOG_PTCOMP_COMP_COUNT slots).
 */
#define COMP_DYN_CFG_COUNT(n, idx)                                                 \
	COND_CODE_1(COMP_CHILD_EXISTS(n, idx),                                         \
		    (COND_CODE_0(UTIL_BOOL(DT_CHILD_NUM(COMP_CHILD_NODE(n, idx))),         \
				(1),                                                               \
				(DT_CHILD_NUM(COMP_CHILD_NODE(n, idx))))),                         \
		    (1))

/**
 * Total number of dynamic config slots across both comparators.
 */
#define NUM_DYN_CONFIGS(n) (COMP_DYN_CFG_COUNT(n, 0) + COMP_DYN_CFG_COUNT(n, 1))

/**
 * Emit dynamic config initializers for comp child idx.
 * Advanced mode: iterate dyn-cfg child nodes.
 * Basic mode: single slot from the comp's own mux properties.
 */
#define COMP_DYN_CFGS(n, idx)                                                      \
	COND_CODE_1(COMP_CHILD_EXISTS(n, idx),                                         \
		    (COND_CODE_0(UTIL_BOOL(DT_CHILD_NUM(COMP_CHILD_NODE(n, idx))),         \
				(BASIC_DYN_CFG(n, idx),),                                          \
				(DT_FOREACH_CHILD(COMP_CHILD_NODE(n, idx), DYN_CFG_FROM_CHILD)))), \
		    (BASIC_DYN_CFG(n, idx),))

/** Basic mode: one slot from a comp child's own DT mux properties */
#define BASIC_DYN_CFG(n, idx)                                                      \
	{                                                                              \
		.ninvInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)COMP_CHILD_PROP_OR(       \
			n, idx, positive_mux, 0),                                              \
		.invInpMux = (cy_en_autanalog_ptcomp_comp_mux_t)COMP_CHILD_PROP_OR(        \
			n, idx, negative_mux, 0),                                              \
	}

/* ===== Post-processing configuration macros ===== */

/**
 * Number of active PP blocks: count comp children with non-zero pp-input-source.
 * Always allocate both entries; compPpCfgNum tells the PDL how many are valid.
 */
#define PP_SRC0(n) COMP_CHILD_PROP_OR(n, 0, pp_input_source, 0)
#define PP_SRC1(n) COMP_CHILD_PROP_OR(n, 1, pp_input_source, 0)
#define NUM_PP(n)  (((PP_SRC0(n) != 0) ? 1 : 0) + ((PP_SRC1(n) != 0) ? 1 : 0))

/** Initialize one PP config block from a comp child's pp-* DT properties */
#define PP_CFG_INIT(n, idx)                                                                    \
	{                                                                                          \
		.inpSrc = (cy_en_autanalog_ptcomp_pp_input_src_t)COMP_CHILD_PROP_OR(               \
			n, idx, pp_input_source, 0),                                               \
		.inputType = (cy_en_autanalog_ptcomp_pp_input_type_t)COMP_CHILD_PROP_OR(           \
			n, idx, pp_input_type, 0),                                                 \
		.cntMode = (cy_en_autanalog_ptcomp_pp_cnt_mode_t)COMP_CHILD_PROP_OR(               \
			n, idx, pp_count_mode, 0),                                                 \
		.dataFunction = (cy_en_autanalog_ptcomp_pp_data_func_t)COMP_CHILD_PROP_OR(         \
			n, idx, pp_data_function, 0),                                              \
		.windowSize = (cy_en_autanalog_ptcomp_pp_window_size_t)COMP_CHILD_PROP_OR(         \
			n, idx, pp_window_size, 0),                                                \
		.period = (uint16_t)COMP_CHILD_PROP_OR(n, idx, pp_period, 0),                      \
		.limitCondition = (cy_en_autanalog_ptcomp_pp_cond_t)COMP_CHILD_PROP_OR(            \
			n, idx, pp_limit_condition, 0),                                            \
		.thresholdLow = (uint16_t)COMP_CHILD_PROP_OR(n, idx, pp_threshold_low, 0),         \
		.thresholdHigh = (uint16_t)COMP_CHILD_PROP_OR(n, idx, pp_threshold_high, 0),       \
	}

/* clang-format on */

static int ifx_autanalog_ptcomp_init(const struct device *dev)
{
	const struct ifx_autanalog_ptcomp_config *cfg = dev->config;
	struct ifx_autanalog_ptcomp_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("AutAnalog MFD parent not ready");
		return -ENODEV;
	}

	/* Load the combined PTComp config into hardware */
	result = Cy_AutAnalog_PTComp_LoadConfig(0, &data->ptcomp_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to initialize PTComp: %d", result);
		return -EIO;
	}

	/* Register a single ISR with the parent for all PTCOMP interrupts */
	ifx_autanalog_set_irq_handler(cfg->parent, dev, IFX_AUTANALOG_PERIPH_PTCOMP0,
				      autanalog_ptcomp_isr);

	LOG_DBG("AutAnalog PTComp MFD initialized");
	return 0;
}

/* clang-format off */

#define IFX_AUTANALOG_PTCOMP_INIT(n)                                                           \
	static struct ifx_autanalog_ptcomp_data ifx_autanalog_ptcomp_data_##n = {                  \
		.static_cfg =                                                                      \
			{                                                                          \
				.lpDivPtcomp = 0,                                                  \
				.powerModeComp0 = (cy_en_autanalog_ptcomp_comp_pwr_t)              \
					COMP_CHILD_PROP_OR(n, 0, power_mode, 0),                   \
				.compHystComp0 = (cy_en_autanalog_ptcomp_comp_hyst_t)              \
					COMP_CHILD_BOOL(n, 0, hysteresis),                         \
				.compEdgeComp0 = (cy_en_autanalog_ptcomp_comp_int_t)               \
					COMP_CHILD_PROP_OR(n, 0, interrupt_mode, 0),               \
				.powerModeComp1 = (cy_en_autanalog_ptcomp_comp_pwr_t)              \
					COMP_CHILD_PROP_OR(n, 1, power_mode, 0),                   \
				.compHystComp1 = (cy_en_autanalog_ptcomp_comp_hyst_t)              \
					COMP_CHILD_BOOL(n, 1, hysteresis),                         \
				.compEdgeComp1 = (cy_en_autanalog_ptcomp_comp_int_t)               \
					COMP_CHILD_PROP_OR(n, 1, interrupt_mode, 0),               \
				.compPpCfgNum = NUM_PP(n),                                         \
				.compPpCfgArr =                                                    \
					NUM_PP(n) ? ifx_autanalog_ptcomp_data_##n.pp_cfg : NULL,   \
			},                                                                         \
		.pp_cfg =                                                                          \
			{                                                                          \
				PP_CFG_INIT(n, 0),                                                 \
				PP_CFG_INIT(n, 1),                                                 \
			},                                                                         \
		.dyn_cfg = {COMP_DYN_CFGS(n, 0) COMP_DYN_CFGS(n, 1)},                          \
			.ptcomp_cfg =                                                              \
				{                                                                  \
					.ptcompStaCfg = &ifx_autanalog_ptcomp_data_##n.static_cfg, \
					.ptcompDynCfgNum = NUM_DYN_CONFIGS(n),                     \
					.ptcompDynCfgArr = ifx_autanalog_ptcomp_data_##n.dyn_cfg,  \
				},                                                                 \
	};                                                                                         \
                                                                                               \
	static const struct ifx_autanalog_ptcomp_config ifx_autanalog_ptcomp_config_##n = {        \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
	};                                                                                         \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &ifx_autanalog_ptcomp_init, NULL, &ifx_autanalog_ptcomp_data_##n, \
			      &ifx_autanalog_ptcomp_config_##n, POST_KERNEL,                       \
			      CONFIG_MFD_INFINEON_AUTANALOG_PTCOMP_INIT_PRIORITY, NULL);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_PTCOMP_INIT)
