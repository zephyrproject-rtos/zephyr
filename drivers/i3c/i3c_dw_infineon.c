/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon-specific glue for the Synopsys DW I3C driver.
 *
 * On Infineon SoCs the DW I3C core sits behind a wrapper block with its own
 * CTRL register (I3C_Type.CTRL).  The wrapper gates the protocol clock and
 * MMIO bus access: until CTRL.ENABLED is set, any DW core register access
 * faults.  This file provides the dw_i3c_platform_ops hooks that manage
 * the wrapper gate, clock routing, and Primary Controller mode selection.
 *
 * Populates dw_i3c_infineon_ops for DT instances with the "infineon,i3c"
 * compat string.
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <cy_sysclk.h>

#include "i3c_dw.h"

LOG_MODULE_DECLARE(i3c_dw, CONFIG_I3C_DW_LOG_LEVEL);

/*
 * DEV_OPERATION_MODE field [1:0], SW R/W:
 *   0x0  Primary Controller   (starts as Active Controller, configures bus)
 *   0x1  Secondary Controller (starts as Target, can become Active)
 * Default Value: 0x1.
 *
 * TRM-prescribed init sequence: DEV_OPERATION_MODE = 0x0 must be written
 * while the block is in the pre-enabled state (I3C_CTRL[ENABLED] = 1 but
 * I3C_CORE_DEVICE_CTRL[ENABLE] = 0).  Because the default is 0x1
 * (Secondary), this write is required even on a pristine reset and again
 * after every soft reset / deep-sleep resume.
 *
 * The TRM further notes that the field is "automatically updated by the
 * block once a role change takes place," i.e. the block's own role state
 * machine drives the visible value.  On current Infineon silicon the
 * field reads back 0x1 even after a successful write in the pre-enable
 * window; the hardware nonetheless functions correctly as Active
 * Controller, which is why dw_i3c_infineon_is_secondary_controller()
 * overrides the generic probe and reports Primary unconditionally.
 */
#define DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL 0x0U
#define DW_I3C_INFINEON_CTRL_ENABLED                BIT(31)
#define DW_I3C_INFINEON_RESET_CTRL_SOFT             BIT(0)
#define DW_I3C_INFINEON_RESET_CTRL_CMD_QUEUE        BIT(1)
#define DW_I3C_INFINEON_RESET_CTRL_RESP_QUEUE       BIT(2)
#define DW_I3C_INFINEON_RESET_CTRL_TX_FIFO          BIT(3)
#define DW_I3C_INFINEON_RESET_CTRL_RX_FIFO          BIT(4)

/* Per-instance peri-div parameters from each I3C node's clocks phandle.
 * The peri-div driver lacks .get_rate, so the glue queries the PDL directly.
 */
struct dw_i3c_infineon_div_info {
	const struct device *dev;
	uint8_t div_type;
	uint8_t channel;
};

#define DW_I3C_INFINEON_DIV_INFO_ENTRY(node)                                                       \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node),                                                        \
		.div_type = DT_PROP(DT_CLOCKS_CTLR(node), div_type),                               \
		.channel = DT_PROP(DT_CLOCKS_CTLR(node), channel),                                 \
	},

static const struct dw_i3c_infineon_div_info dw_i3c_infineon_divs[] = {
	DT_FOREACH_STATUS_OKAY(infineon_i3c, DW_I3C_INFINEON_DIV_INFO_ENTRY)};

static const struct dw_i3c_infineon_div_info *dw_i3c_infineon_div_lookup(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(dw_i3c_infineon_divs); i++) {
		if (dw_i3c_infineon_divs[i].dev == dev) {
			return &dw_i3c_infineon_divs[i];
		}
	}
	return NULL;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pre_init.
 *
 * Called by the generic DW I3C driver after clock_on but before pinctrl is
 * applied or any DW core register is touched.
 *
 * @param dev   I3C controller device pointer.
 * @return 0 on success, -ENODEV if @p dev is not a known Infineon I3C
 *         instance.
 */
static int dw_i3c_infineon_pre_init(const struct device *dev)
{
	const struct dw_i3c_infineon_div_info *info = dw_i3c_infineon_div_lookup(dev);
	I3C_Type *base;

	if (info == NULL) {
		return -ENODEV;
	}

	base = (I3C_Type *)dw_i3c_get_regs(dev);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_I3C_PERI_NR, CY_MMIO_I3C_GROUP_NR,
				     CY_MMIO_I3C_SLAVE_NR, CY_MMIO_I3C_CLK_HF_NR);
	Cy_SysClk_PeriPclkAssignDivider((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN, info->div_type,
					info->channel);
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);

	LOG_DBG("Infineon I3C clocks and wrapper gate configured");

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.post_reset.
 *
 * Reopen the wrapper gate so the soft reset can complete, then select
 * Primary Controller mode before the generic driver re-enables the core.
 *
 * @param dev   I3C controller device pointer.
 * @return 0 on success, -ETIMEDOUT if SOFT_RST does not self-clear.
 */
static int dw_i3c_infineon_post_reset(const struct device *dev)
{
	I3C_Type *base = (I3C_Type *)(uintptr_t)dw_i3c_get_regs(dev);
	uint32_t timeout = 10000U;

	/* Reopen the wrapper gate so SOFT_RST can self-clear. */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);
	while ((sys_read32((mm_reg_t)&base->CORE.RESET_CTRL) & DW_I3C_INFINEON_RESET_CTRL_SOFT) &&
	       --timeout) {
	}
	if (timeout == 0U) {
		LOG_ERR("Infineon I3C: RESET_CTRL_SOFT did not self-clear");
		return -ETIMEDOUT;
	}

	sys_write32(DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL,
		    (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);

	LOG_DBG("Infineon I3C post-reset complete (SOFT_RST cleared in %u iterations)",
		10000U - timeout);

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.clock_on.
 *
 * No-op: the peri-div driver automatically enables the divider.  It does
 * not implement clock_control .on.
 *
 * @param dev  I3C controller device pointer.
 * @return 0 always.
 */
static int dw_i3c_infineon_clock_on(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pm_resume.
 *
 * Reopen the Infineon wrapper clock gate and re-assert Primary Controller
 * mode after deep-sleep resume.
 *
 * @param dev   I3C controller device pointer.
 * @return 0 on success, -ENODEV if @p dev is not a known Infineon I3C
 *         instance.
 */
static int dw_i3c_infineon_pm_resume(const struct device *dev)
{
	const struct dw_i3c_infineon_div_info *info = dw_i3c_infineon_div_lookup(dev);
	I3C_Type *base;

	if (info == NULL) {
		return -ENODEV;
	}

	base = (I3C_Type *)dw_i3c_get_regs(dev);

	Cy_SysClk_PeriPclkEnableDivider((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN, info->div_type,
					info->channel);
	Cy_SysClk_PeriPclkAssignDivider((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN, info->div_type,
					info->channel);

	/* Reopen the Infineon wrapper clock gate, allow the hardware to settle,
	 * and then re-apply configuration
	 */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);
	k_busy_wait(200);
	sys_write32(DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL,
		    (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.is_secondary_controller.
 *
 * WIP: this currently only implements the I3C Controller  (Primary Controller)
 * role.  Target / Secondary Controller support in  Zephyr for Infineon I3C
 * isn't currently implemented.
 *
 * @param dev   I3C controller device pointer.
 * @return false always — Infineon I3C operates as Primary Controller.
 */
static bool dw_i3c_infineon_is_secondary_controller(const struct device *dev)
{
	/* The generic-driver default reads DEVICE_CTRL_EXTENDED.DEV_OPERATION_MODE,
	 * which on current Infineon silicon does not reflect the software-selected
	 * role.  See the block comment above DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL
	 * for the full rationale.
	 */
	ARG_UNUSED(dev);
	return false;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.get_clock_rate.
 *
 * @param dev   I3C controller device pointer.
 * @param rate  Output: I3C core clock frequency in Hz.
 * @return 0 on success, -ENODEV if @p dev is not a known Infineon I3C
 *         instance.
 */
static int dw_i3c_infineon_get_clock_rate(const struct device *dev, uint32_t *rate)
{
	const struct dw_i3c_infineon_div_info *info = dw_i3c_infineon_div_lookup(dev);

	if (info == NULL) {
		return -ENODEV;
	}

	*rate = Cy_SysClk_PeriPclkGetFrequency((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN, info->div_type,
					       info->channel);
	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pre_resume_ctrl.
 *
 * Re-open the Infineon wrapper clock gate so the FIFO-flush bits written
 * to RESET_CTRL by the generic driver can self-clear, then wait for them
 * to do so before the generic driver writes DEV_CTRL_RESUME.
 *
 * @param dev   I3C controller device pointer.
 */
static void dw_i3c_infineon_pre_resume_ctrl(const struct device *dev)
{
	I3C_Type *base = (I3C_Type *)dw_i3c_get_regs(dev);
	uint32_t timeout = 10000U;
	const uint32_t fifo_mask =
		DW_I3C_INFINEON_RESET_CTRL_SOFT | DW_I3C_INFINEON_RESET_CTRL_RX_FIFO |
		DW_I3C_INFINEON_RESET_CTRL_TX_FIFO | DW_I3C_INFINEON_RESET_CTRL_RESP_QUEUE |
		DW_I3C_INFINEON_RESET_CTRL_CMD_QUEUE;

	/* Re-open the wrapper clock gate */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);

	/* Poll until all FIFO reset bits (and SOFT if it was set incidentally)
	 * self-clear.  Only when they clear is the DW core bus interface fully
	 * responsive and DEVICE_CTRL safe to access.
	 */
	while ((sys_read32((mm_reg_t)&base->CORE.RESET_CTRL) & fifo_mask) && --timeout) {
	}
	if (timeout == 0U) {
		LOG_ERR("Infineon I3C: RESET_CTRL flush bits did not self-clear");
	}
}

const struct dw_i3c_platform_ops dw_i3c_infineon_ops = {
	.get_clock_rate = dw_i3c_infineon_get_clock_rate,
	.clock_on = dw_i3c_infineon_clock_on,
	.pre_init = dw_i3c_infineon_pre_init,
	.post_reset = dw_i3c_infineon_post_reset,
	.pre_resume_ctrl = dw_i3c_infineon_pre_resume_ctrl,
	.pm_resume = dw_i3c_infineon_pm_resume,
	.is_secondary_controller = dw_i3c_infineon_is_secondary_controller,
};
