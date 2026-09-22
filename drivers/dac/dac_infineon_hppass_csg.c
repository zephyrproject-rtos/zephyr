/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief HPPASS CSG 10-bit DAC driver.
 *
 * Owns DAC_CFG and DAC_VAL within a single CSG slice.  Buffered, slope,
 * and LUT modes are not yet supported.
 *
 * HPPASS is shared across multiple Infineon device families.  Citations
 * refer to PSOC Control C3 Mainline documentation:
 *   - <em>Architecture TRM</em> 002-39348 (§27.2.3.1 10-bit DAC)
 *   - <em>Registers TRM</em> 002-39445 (§20.1.43–20.1.50)
 */

#define DT_DRV_COMPAT infineon_hppass_csg_dac

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <infineon_kconfig.h>
#include <cy_device.h>

#include "mfd_infineon_hppass_regs.h"

LOG_MODULE_REGISTER(dac_infineon_hppass_csg, CONFIG_DAC_LOG_LEVEL);

#define IFX_CSG_DAC_RESOLUTION   10U
#define IFX_CSG_DAC_VAL_MAX      ((1U << IFX_CSG_DAC_RESOLUTION) - 1U)

#define IFX_CSG_DAC_SLICE_OF(n)                                                          \
	((uint8_t)((DT_INST_REG_ADDR(n) - DT_REG_ADDR(DT_INST_PARENT(n))) /              \
		   DT_PROP(DT_INST_PARENT(n), infineon_slice_reg_spacing)))

/* DAC_CFG field masks (Regs TRM §20.1.43). */
#define IFX_CSG_DAC_CFG_TR_START_SEL  GENMASK(3, 0)
#define IFX_CSG_DAC_CFG_TR_UPDATE_SEL GENMASK(7, 4)
#define IFX_CSG_DAC_CFG_MODE          GENMASK(10, 8)
#define IFX_CSG_DAC_CFG_CONT          BIT(12)
#define IFX_CSG_DAC_CFG_SKIP_TR_EN    BIT(13)
#define IFX_CSG_DAC_CFG_CASCADE_EN    BIT(14)
#define IFX_CSG_DAC_CFG_PARAM_SYNC_EN BIT(15)
#define IFX_CSG_DAC_CFG_STEP          GENMASK(25, 20)
#define IFX_CSG_DAC_CFG_DEGLITCH      GENMASK(30, 28)

struct ifx_csg_dac_config {
	const struct device *parent;
	mem_addr_t dac_base;
	uint32_t dac_cfg;
	uint16_t initial_value;
};

struct ifx_csg_dac_data {
	bool channel_setup_done;
};

static int ifx_csg_dac_channel_setup(const struct device *dev,
				     const struct dac_channel_cfg *channel_cfg)
{
	struct ifx_csg_dac_data *data = dev->data;

	if (channel_cfg->channel_id != 0U) {
		return -EINVAL;
	}

	if (channel_cfg->resolution != IFX_CSG_DAC_RESOLUTION) {
		return -ENOTSUP;
	}

	/* CSG DAC has no external pin and no programmable output buffer. */
	if (channel_cfg->buffered) {
		return -ENOTSUP;
	}

	data->channel_setup_done = true;

	return 0;
} /* ifx_csg_dac_channel_setup() */

static int ifx_csg_dac_write_value(const struct device *dev, uint8_t channel,
				   uint32_t value)
{
	const struct ifx_csg_dac_config *cfg = dev->config;
	const struct ifx_csg_dac_data *data = dev->data;

	if (channel != 0U) {
		return -EINVAL;
	}

	if (!data->channel_setup_done) {
		return -EINVAL;
	}

	if (value > IFX_CSG_DAC_VAL_MAX) {
		return -EINVAL;
	}

	sys_write32(value & HPPASS_CSG_SLICE_DAC_VAL_VALUE_Msk,
		    cfg->dac_base + IFX_HPPASS_CSG_SLICE_DAC_VAL);

	return 0;
} /* ifx_csg_dac_write_value() */

static DEVICE_API(dac, ifx_csg_dac_api) = {
	.channel_setup = ifx_csg_dac_channel_setup,
	.write_value   = ifx_csg_dac_write_value,
};

/* Per-slice init.  Runs after the CSG MFD parent. */
static int ifx_csg_dac_init(const struct device *dev)
{
	const struct ifx_csg_dac_config *cfg = dev->config;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("CSG MFD parent not ready");
		return -ENODEV;
	}

	/* Program DAC_CFG (deglitch setting; mode=0 for direct-write). */
	sys_write32(cfg->dac_cfg, cfg->dac_base + IFX_HPPASS_CSG_SLICE_DAC_CFG);

	/* Set initial DAC output value. */
	sys_write32((uint32_t)cfg->initial_value & HPPASS_CSG_SLICE_DAC_VAL_VALUE_Msk,
		    cfg->dac_base + IFX_HPPASS_CSG_SLICE_DAC_VAL);

	return 0;
} /* ifx_csg_dac_init() */

/* Compose DAC_CFG for one DAC child.  All advanced-mode fields stay zero. */
#define IFX_CSG_DAC_CFG_INIT(node_id)                                                    \
	FIELD_PREP(IFX_CSG_DAC_CFG_DEGLITCH, DT_PROP(node_id, deglitch))

#define IFX_CSG_DAC_INIT(n)                                                              \
	BUILD_ASSERT(IFX_CSG_DAC_SLICE_OF(n) <                                           \
			DT_PROP(DT_INST_PARENT(n), infineon_num_slices),                \
		     "CSG DAC slice index out of range");                                \
	static struct ifx_csg_dac_data ifx_csg_dac_data_##n;                             \
	static const struct ifx_csg_dac_config ifx_csg_dac_config_##n = {                \
		.parent        = DEVICE_DT_GET(DT_INST_PARENT(n)),                      \
		.dac_base      = (mem_addr_t)DT_INST_REG_ADDR(n),                        \
		.dac_cfg       = IFX_CSG_DAC_CFG_INIT(DT_DRV_INST(n)),                   \
		.initial_value = (uint16_t)DT_INST_PROP_OR(n, initial_value, 0),         \
	};                                                                               \
	static int ifx_csg_dac_init_##n(const struct device *dev)                        \
	{                                                                                \
		return ifx_csg_dac_init(dev);                                            \
	}                                                                                \
	DEVICE_DT_INST_DEFINE(n, ifx_csg_dac_init_##n, NULL,                             \
			      &ifx_csg_dac_data_##n, &ifx_csg_dac_config_##n,            \
			      POST_KERNEL,                                               \
			      CONFIG_DAC_INFINEON_HPPASS_CSG_INIT_PRIORITY,              \
			      &ifx_csg_dac_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_CSG_DAC_INIT)
