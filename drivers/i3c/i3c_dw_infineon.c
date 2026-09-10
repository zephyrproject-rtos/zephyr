/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
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
#include <stdbool.h>

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
 * Controller. Role selection therefore follows configured policy via
 * dw_i3c_is_secondary_requested(dev) rather than relying on this readback.
 */
#define DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL 0x0U
#define DW_I3C_INFINEON_OPERATION_MODE_SECONDARY_CTRL 0x1U
#define DW_I3C_INFINEON_CTRL_ENABLED                BIT(31)
#define DW_I3C_INFINEON_DEV_CTRL_RESUME              BIT(30)
#define DW_I3C_INFINEON_RESET_CTRL_SOFT             BIT(0)
#define DW_I3C_INFINEON_RESET_CTRL_CMD_QUEUE        BIT(1)
#define DW_I3C_INFINEON_RESET_CTRL_RESP_QUEUE       BIT(2)
#define DW_I3C_INFINEON_RESET_CTRL_TX_FIFO          BIT(3)
#define DW_I3C_INFINEON_RESET_CTRL_RX_FIFO          BIT(4)

#define DW_I3C_INFINEON_RESET_TIMEOUT_US             2000U
#define DW_I3C_INFINEON_RESUME_TIMEOUT_US            2000U

static int dw_i3c_infineon_wait_ctrl_enabled(const struct device *dev, I3C_Type *base,
						      uint32_t timeout_us)
{
	if (!WAIT_FOR((sys_read32((mm_reg_t)&base->CTRL) & DW_I3C_INFINEON_CTRL_ENABLED) != 0U,
		      timeout_us, k_busy_wait(1))) {
		LOG_ERR("%s: wrapper CTRL.ENABLED did not assert", dev->name);
		return -ETIMEDOUT;
	}

	return 0;
}

struct dw_i3c_infineon_div {
	const struct device *dev;
	uint8_t div_type;
	uint8_t channel;
};

/*
 * Resolve peri-div parameters from DT only for I3C nodes that actually have
 * a clocks phandle. Some board configurations omit clocks and rely on
 * secure-side preconfiguration.
 */
#define DW_I3C_INFINEON_DIV_ENTRY(node)                                                            \
	IF_ENABLED(DT_NODE_HAS_PROP(node, clocks),                                                 \
		   ({.dev = DEVICE_DT_GET(node),                                                   \
		     .div_type = DT_PROP(DT_CLOCKS_CTLR(node), div_type),                          \
		     .channel = DT_PROP(DT_CLOCKS_CTLR(node), channel)},))

static const struct dw_i3c_infineon_div dw_i3c_infineon_divs[] = {
	DT_FOREACH_STATUS_OKAY(infineon_i3c, DW_I3C_INFINEON_DIV_ENTRY)
};

static bool dw_i3c_infineon_div_lookup(const struct device *dev, uint8_t *div_type,
				       uint8_t *channel)
{
	for (size_t i = 0; i < ARRAY_SIZE(dw_i3c_infineon_divs); i++) {
		if (dw_i3c_infineon_divs[i].dev != dev) {
			continue;
		}

		*div_type = dw_i3c_infineon_divs[i].div_type;
		*channel = dw_i3c_infineon_divs[i].channel;

		return true;
	}

	return false;
}

static int dw_i3c_infineon_enable_pclk(en_clk_dst_t dst, uint8_t div_type, uint8_t channel,
					const char *name)
{
	cy_en_sysclk_status_t clk_status;
	bool enabled;

	clk_status = Cy_SysClk_PeriPclkAssignDivider(dst, div_type, channel);
	if (clk_status != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Infineon I3C: %s assign failed (status=0x%08x)", name,
			(uint32_t)clk_status);
		return -EIO;
	}

	clk_status = Cy_SysClk_PeriPclkEnableDivider(dst, div_type, channel);
	if (clk_status != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Infineon I3C: %s enable failed (status=0x%08x)", name,
			(uint32_t)clk_status);
		return -EIO;
	}

	enabled = Cy_SysClk_PeriPclkGetDividerEnabled(dst, div_type, channel);
	if (!enabled) {
		LOG_ERR("Infineon I3C: %s divider not enabled after programming", name);
		return -EIO;
	}

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pre_init
 *
 * Called by the generic DW I3C driver after clock_on but before pinctrl is
 * applied or any DW core register is touched
 *
 * @param dev   I3C controller device pointer.
 * @retval 0 on success
 * @retval -errno Peripheral clock could not be enabled
 */
static int dw_i3c_infineon_pre_init(const struct device *dev)
{
	uint8_t div_type;
	uint8_t channel;
	bool have_div_info = dw_i3c_infineon_div_lookup(dev, &div_type, &channel);
	const uint32_t slave_bit = 0x1UL << CY_MMIO_I3C_SLAVE_NR;
	cy_en_sysclk_status_t clk_status;
	I3C_Type *base;

	base = (I3C_Type *)dw_i3c_get_regs(dev);

	PERI_GR_SL_CTL2(CY_MMIO_I3C_PERI_NR, CY_MMIO_I3C_GROUP_NR) &= ~slave_bit;
	PERI_GR_SL_CTL(CY_MMIO_I3C_PERI_NR, CY_MMIO_I3C_GROUP_NR) |= slave_bit;

	/* Ensure parent HF clock is enabled before touching clocked DW core regs. */
	if (!IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE)) {
		/* Cy_SysClk_ClkHfEnable() writes SRSS, which is secure-only. */
		clk_status = Cy_SysClk_ClkHfEnable(CY_MMIO_I3C_CLK_HF_NR);
		ARG_UNUSED(clk_status);
	}

	if (have_div_info) {
		int pclk_ret;

		pclk_ret = dw_i3c_infineon_enable_pclk((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN,
						       div_type, channel, "I3C_EN");
		if (pclk_ret != 0) {
			return pclk_ret;
		}
	}

	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);

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
	uint32_t operation_mode = dw_i3c_is_secondary_requested(dev) ?
		DW_I3C_INFINEON_OPERATION_MODE_SECONDARY_CTRL :
		DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL;

	/* Reopen the wrapper gate so SOFT_RST can self-clear. */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);

	if (!WAIT_FOR((sys_read32((mm_reg_t)&base->CORE.RESET_CTRL) &
		       DW_I3C_INFINEON_RESET_CTRL_SOFT) == 0U,
		      DW_I3C_INFINEON_RESET_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	sys_write32(operation_mode,
		    (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);

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

static enum dw_i3c_isr_error_action dw_i3c_infineon_isr_error_action(const struct device *dev,
								     int xfer_error)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(xfer_error);

	/*
	 * On Infineon's wrappered DW I3C, touching RESET_CTRL from IRQ context
	 * after transfer errors can gate the register bus and fault at the
	 * controller base address. Defer recovery to explicit recover paths.
	 */
	return DW_I3C_ISR_ERROR_DEFER_RECOVER;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.should_retry_ccc_timeout
 *
 * @retval 0 on success, with @p action set
 * @retval -EINVAL @p action is NULL
 */
static int dw_i3c_infineon_should_retry_ccc_timeout(const struct device *dev,
						      bool retried_after_recover,
						      bool first_cmd_error_none,
						      bool is_setdasa_direct,
						      bool is_enec_broadcast,
						      enum dw_i3c_retry_action *action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(first_cmd_error_none);
	ARG_UNUSED(is_setdasa_direct);
	ARG_UNUSED(is_enec_broadcast);

	if (action == NULL) {
		return -EINVAL;
	}

	if (retried_after_recover) {
		*action = DW_I3C_RETRY_NONE;
		return 0;
	}

	/* Keep existing Infineon CCC policy: one guarded recover+retry attempt. */
	*action = DW_I3C_RETRY_FULL_RECOVER;

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.should_retry_timeout
 *
 * @retval 0 on success, with @p action set
 * @retval -EINVAL @p action is NULL
 */
static int dw_i3c_infineon_should_retry_timeout(const struct device *dev,
						 enum dw_i3c_timeout_op op,
						 bool retried_after_recover,
						 bool first_cmd_addr_nack,
						 enum dw_i3c_retry_action *action)
{
	ARG_UNUSED(dev);

	if (action == NULL) {
		return -EINVAL;
	}

	*action = DW_I3C_RETRY_NONE;

	if (retried_after_recover) {
		return 0;
	}

	if (op == DW_I3C_TIMEOUT_OP_DAA) {
		*action = DW_I3C_RETRY_FULL_RECOVER;
		return 0;
	}

	if (op != DW_I3C_TIMEOUT_OP_XFERS) {
		return 0;
	}

	if (first_cmd_addr_nack) {
		*action = DW_I3C_RETRY_LIGHT_RECOVER;
	} else {
		*action = DW_I3C_RETRY_FULL_RECOVER;
	}

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.post_enable.
 *
 * On PSE84, pulsing DEV_CTRL_RESUME immediately after enabling the core can
 * leave RESUME stuck and prevent the first CCC sequence from progressing.
 * Keep startup conservative: ensure RESUME is deasserted before bus init.
 */
static void dw_i3c_infineon_post_enable(const struct device *dev, bool is_secondary)
{
	I3C_Type *base = (I3C_Type *)dw_i3c_get_regs(dev);
	uint32_t dev_ctrl;
	uint32_t operation_mode = is_secondary ?
		DW_I3C_INFINEON_OPERATION_MODE_SECONDARY_CTRL :
		DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL;

	sys_write32(operation_mode, (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);

	dev_ctrl = sys_read32((mm_reg_t)&base->CORE.DEVICE_CTRL);
	if ((dev_ctrl & DW_I3C_INFINEON_DEV_CTRL_RESUME) == 0U) {
		return;
	}

	/* Clear any stale RESUME latch inherited from reset/clock-enable sequencing. */
	sys_write32(dev_ctrl & ~DW_I3C_INFINEON_DEV_CTRL_RESUME,
		    (mm_reg_t)&base->CORE.DEVICE_CTRL);

	(void)WAIT_FOR((sys_read32((mm_reg_t)&base->CORE.DEVICE_CTRL) &
			DW_I3C_INFINEON_DEV_CTRL_RESUME) == 0U,
		       DW_I3C_INFINEON_RESUME_TIMEOUT_US, k_busy_wait(1));
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pm_resume
 *
 * Reopen the Infineon wrapper clock gate and re-assert Primary Controller
 * mode after deep-sleep resume
 *
 * @param dev   I3C controller device pointer
 * @retval 0 on success.
 * @retval -ETIMEDOUT Wrapper clock gate did not report enabled
 * @retval -errno Peripheral clock could not be enabled
 */
static int dw_i3c_infineon_pm_resume(const struct device *dev)
{
	uint8_t div_type;
	uint8_t channel;
	bool have_div_info = dw_i3c_infineon_div_lookup(dev, &div_type, &channel);
	I3C_Type *base;
	int pclk_ret;

	base = (I3C_Type *)dw_i3c_get_regs(dev);

	if (have_div_info) {
		pclk_ret = dw_i3c_infineon_enable_pclk((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN,
						       div_type, channel, "I3C_EN");
		if (pclk_ret != 0) {
			LOG_ERR("Infineon I3C: pm_resume I3C_EN clock enable failed (%d)",
				pclk_ret);
			return pclk_ret;
		}
	}

	/* Reopen the Infineon wrapper clock gate, allow the hardware to settle,
	 * and then re-apply configuration
	 */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);
	if (dw_i3c_infineon_wait_ctrl_enabled(dev, base, 200U) != 0) {
		return -ETIMEDOUT;
	}
	sys_write32(dw_i3c_is_secondary_requested(dev) ?
		    DW_I3C_INFINEON_OPERATION_MODE_SECONDARY_CTRL :
		    DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL,
		    (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);

	return 0;
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
	uint8_t div_type;
	uint8_t channel;

	if (!dw_i3c_infineon_div_lookup(dev, &div_type, &channel)) {
		return -ENODEV;
	}

	*rate = Cy_SysClk_PeriPclkGetFrequency((en_clk_dst_t)PCLK_I3C_CLOCK_I3C_EN, div_type,
					       channel);
	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.pre_resume_ctrl
 *
 * Re-open the Infineon wrapper clock gate so the FIFO-flush bits written
 * to RESET_CTRL by the generic driver can self-clear, then wait for them
 * to do so before the generic driver writes DEV_CTRL_RESUME
 *
 * @param dev   I3C controller device pointer
 * @retval 0 on success.
 * @retval -ETIMEDOUT Wrapper gate or RESET_CTRL flush bits did not settle
 */
static int dw_i3c_infineon_pre_resume_ctrl(const struct device *dev)
{
	I3C_Type *base = (I3C_Type *)dw_i3c_get_regs(dev);
	uint32_t dev_ctrl;
	const uint32_t fifo_mask =
		DW_I3C_INFINEON_RESET_CTRL_SOFT | DW_I3C_INFINEON_RESET_CTRL_RX_FIFO |
		DW_I3C_INFINEON_RESET_CTRL_TX_FIFO | DW_I3C_INFINEON_RESET_CTRL_RESP_QUEUE |
		DW_I3C_INFINEON_RESET_CTRL_CMD_QUEUE;

	/* Re-open the wrapper clock gate */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);
	if (dw_i3c_infineon_wait_ctrl_enabled(dev, base, 100U) != 0) {
		return -ETIMEDOUT;
	}

	/* Poll until all FIFO reset bits (and SOFT if it was set incidentally)
	 * self-clear.  Only when they clear is the DW core bus interface fully
	 * responsive and DEVICE_CTRL safe to access.
	 */
	if (!WAIT_FOR((sys_read32((mm_reg_t)&base->CORE.RESET_CTRL) & fifo_mask) == 0U,
		      DW_I3C_INFINEON_RESET_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Infineon I3C: RESET_CTRL flush bits stuck (RESET_CTRL=0x%08x)",
			sys_read32((mm_reg_t)&base->CORE.RESET_CTRL));
		return -ETIMEDOUT;
	}

	/* Ensure recover_bus sees a clean RESUME edge instead of inheriting a stale set bit. */
	dev_ctrl = sys_read32((mm_reg_t)&base->CORE.DEVICE_CTRL);
	if ((dev_ctrl & DW_I3C_INFINEON_DEV_CTRL_RESUME) != 0U) {
		sys_write32(dev_ctrl & ~DW_I3C_INFINEON_DEV_CTRL_RESUME,
			    (mm_reg_t)&base->CORE.DEVICE_CTRL);

		(void)WAIT_FOR((sys_read32((mm_reg_t)&base->CORE.DEVICE_CTRL) &
				DW_I3C_INFINEON_DEV_CTRL_RESUME) == 0U,
			       DW_I3C_INFINEON_RESUME_TIMEOUT_US, k_busy_wait(1));
	}

	return 0;
}

/**
 * @brief Infineon implementation of dw_i3c_platform_ops.re_arm_target.
 *
 * After target-side RSTDAA, some variants may ignore the next ENTDAA until
 * the target detector path is nudged. Keep this lightweight for IRQ context:
 * reopen wrapper gate and re-assert role mode. In target mode this part
 * holds DEVICE_CTRL.RESUME permanently set, so pulsing it cannot produce
 * an edge and is not attempted.
 */
static void dw_i3c_infineon_re_arm_target(const struct device *dev)
{
	I3C_Type *base = (I3C_Type *)dw_i3c_get_regs(dev);
	uint32_t operation_mode = dw_i3c_is_secondary_requested(dev) ?
		DW_I3C_INFINEON_OPERATION_MODE_SECONDARY_CTRL :
		DW_I3C_INFINEON_OPERATION_MODE_PRIMARY_CTRL;

	/* Ensure wrapper access before touching core registers. */
	sys_write32(DW_I3C_INFINEON_CTRL_ENABLED, (mm_reg_t)&base->CTRL);
	if (dw_i3c_infineon_wait_ctrl_enabled(dev, base, 100U) != 0) {
		return;
	}
	sys_write32(operation_mode, (mm_reg_t)&base->CORE.DEVICE_CTRL_EXTENDED);
}

const struct dw_i3c_platform_ops dw_i3c_infineon_ops = {
	.get_clock_rate = dw_i3c_infineon_get_clock_rate,
	.clock_on = dw_i3c_infineon_clock_on,
	.should_retry_timeout = dw_i3c_infineon_should_retry_timeout,
	.should_retry_ccc_timeout = dw_i3c_infineon_should_retry_ccc_timeout,
	.isr_error_action = dw_i3c_infineon_isr_error_action,
	.pre_init = dw_i3c_infineon_pre_init,
	.post_reset = dw_i3c_infineon_post_reset,
	.post_enable = dw_i3c_infineon_post_enable,
	.pre_resume_ctrl = dw_i3c_infineon_pre_resume_ctrl,
	.re_arm_target = dw_i3c_infineon_re_arm_target,
	.pm_resume = dw_i3c_infineon_pm_resume,
	.dev_operation_mode_write_only = true,
};
