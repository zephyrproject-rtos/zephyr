/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for Infineon AutAnalog PRB (Programmable Reference Block).
 *
 * The PRB contains two voltage references (Vref0, Vref1) that share a combined
 * PDL configuration structure (cy_stc_autanalog_prb_t).  This MFD driver
 * aggregates both references into the single struct and calls
 * Cy_AutAnalog_PRB_LoadConfig to apply the combined configuration.
 *
 * Child regulator nodes (compatible "infineon,autanalog-prb-vref") call the
 * ifx_autanalog_prb_set_vref API to update their individual settings.
 */

#define DT_DRV_COMPAT infineon_autanalog_prb

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>
#include <infineon_autanalog_prb.h>

#include "cy_pdl.h"

LOG_MODULE_REGISTER(mfd_infineon_autanalog_prb, CONFIG_MFD_LOG_LEVEL);

#define IFX_AUTANALOG_PRB_VREF_COUNT 2

struct ifx_autanalog_prb_config {
	const struct device *parent;
};

struct ifx_autanalog_prb_data {
	cy_stc_autanalog_prb_cfg_t vref_cfg[IFX_AUTANALOG_PRB_VREF_COUNT];
	cy_stc_autanalog_prb_t prb_cfg;
};

int ifx_autanalog_prb_set_vref(const struct device *dev, uint8_t vref_idx, bool enabled,
			       uint8_t src, uint8_t tap)
{
	struct ifx_autanalog_prb_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (vref_idx >= IFX_AUTANALOG_PRB_VREF_COUNT) {
		return -EINVAL;
	}

	data->vref_cfg[vref_idx].enable = enabled;
	data->vref_cfg[vref_idx].src = (cy_en_autanalog_prb_src_t)src;
	data->vref_cfg[vref_idx].tap = (cy_en_autanalog_prb_tap_t)tap;

	result = Cy_AutAnalog_PRB_LoadConfig(0, &data->prb_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to load PRB config: %d", result);
		return -EIO;
	}

	return 0;
}

int ifx_autanalog_prb_get_vref(const struct device *dev, uint8_t vref_idx, bool *enabled,
			       uint8_t *src, uint8_t *tap)
{
	struct ifx_autanalog_prb_data *data = dev->data;

	if (vref_idx >= IFX_AUTANALOG_PRB_VREF_COUNT) {
		return -EINVAL;
	}

	*enabled = data->vref_cfg[vref_idx].enable;
	*src = (uint8_t)data->vref_cfg[vref_idx].src;
	*tap = (uint8_t)data->vref_cfg[vref_idx].tap;

	return 0;
}

/* clang-format off */

/**
 * @brief Check if a vref child node exists and is enabled at a given index
 */
#define VREF_CHILD_NODE(n, idx)   DT_CHILD(DT_DRV_INST(n), vref_##idx)
#define VREF_CHILD_EXISTS(n, idx) DT_NODE_EXISTS(VREF_CHILD_NODE(n, idx))
#define VREF_CHILD_OKAY(n, idx)                                                                \
	(VREF_CHILD_EXISTS(n, idx) && DT_NODE_HAS_STATUS(VREF_CHILD_NODE(n, idx), okay))

/** Get a DT property from the vref child, or use a default */
#define VREF_CHILD_PROP_OR(n, idx, prop, default_val)                                          \
	COND_CODE_1(VREF_CHILD_EXISTS(n, idx), \
		    (DT_PROP(VREF_CHILD_NODE(n, idx), prop)), \
		    (default_val))

/* clang-format on */

static int ifx_autanalog_prb_init(const struct device *dev)
{
	const struct ifx_autanalog_prb_config *cfg = dev->config;
	struct ifx_autanalog_prb_data *data = dev->data;
	cy_en_autanalog_status_t result;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("AutAnalog MFD parent not ready");
		return -ENODEV;
	}

	/* Load the combined PRB config into hardware */
	result = Cy_AutAnalog_PRB_LoadConfig(0, &data->prb_cfg);
	if (result != CY_AUTANALOG_SUCCESS) {
		LOG_ERR("Failed to initialize PRB: %d", result);
		return -EIO;
	}

	LOG_DBG("AutAnalog PRB MFD initialized");
	return 0;
}

/* clang-format off */

#define IFX_AUTANALOG_PRB_INIT(n)                                                              \
	static struct ifx_autanalog_prb_data ifx_autanalog_prb_data_##n = {                        \
		.vref_cfg =                                                                        \
			{                                                                          \
				{                                                                  \
					.enable = VREF_CHILD_OKAY(n, 0),                           \
					.src = (cy_en_autanalog_prb_src_t)VREF_CHILD_PROP_OR(      \
						n, 0, source, 0),                                  \
					.tap = (cy_en_autanalog_prb_tap_t)VREF_CHILD_PROP_OR(      \
						n, 0, tap, 0),                                     \
				},                                                                 \
				{                                                                  \
					.enable = VREF_CHILD_OKAY(n, 1),                           \
					.src = (cy_en_autanalog_prb_src_t)VREF_CHILD_PROP_OR(      \
						n, 1, source, 0),                                  \
					.tap = (cy_en_autanalog_prb_tap_t)VREF_CHILD_PROP_OR(      \
						n, 1, tap, 0),                                     \
				},                                                                 \
			},                                                                         \
		.prb_cfg =                                                                         \
			{                                                                          \
				.prb =                                                             \
					{                                                          \
						&ifx_autanalog_prb_data_##n.vref_cfg[0],           \
						&ifx_autanalog_prb_data_##n.vref_cfg[1],           \
					},                                                         \
			},                                                                         \
	};                                                                                         \
                                                                                               \
	static const struct ifx_autanalog_prb_config ifx_autanalog_prb_config_##n = {              \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
	};                                                                                         \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &ifx_autanalog_prb_init, NULL, &ifx_autanalog_prb_data_##n,       \
			      &ifx_autanalog_prb_config_##n, POST_KERNEL,                          \
			      CONFIG_MFD_INFINEON_AUTANALOG_PRB_INIT_PRIORITY, NULL);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_PRB_INIT)
