/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal header shared between i3c_dw.c and vendor glue files such as
 * i3c_dw_infineon.c.  Not part of the public Zephyr driver API.
 */

#ifndef ZEPHYR_DRIVERS_I3C_I3C_DW_H_
#define ZEPHYR_DRIVERS_I3C_I3C_DW_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i3c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Transfer operation kind for timeout policy queries.
 */
enum dw_i3c_timeout_op {
	DW_I3C_TIMEOUT_OP_XFERS,
	DW_I3C_TIMEOUT_OP_CCC,
	DW_I3C_TIMEOUT_OP_DAA,
};

/**
 * @brief Retry action returned by vendor timeout policy hooks.
 */
enum dw_i3c_retry_action {
	DW_I3C_RETRY_NONE,
	DW_I3C_RETRY_LIGHT_RECOVER,
	DW_I3C_RETRY_FULL_RECOVER,
};

/**
 * @brief ISR transfer-error recovery action.
 *
 * Used by vendor policy to decide whether generic ISR error handling should
 * perform immediate RESET_CTRL/RESUME writes, or defer recovery to a later
 * non-ISR recovery path.
 */
enum dw_i3c_isr_error_action {
	DW_I3C_ISR_ERROR_RECOVER_NOW,
	DW_I3C_ISR_ERROR_DEFER_RECOVER,
};

/**
 * @brief Vendor-specific platform operations called by the generic DW I3C driver.
 *
 * Vendors may populate this struct and point dw_i3c_config.ops at it to hook
 * into the driver lifecycle without modifying the shared i3c_dw.c source.
 * All members are optional; NULL pointers are silently skipped.
 */
struct dw_i3c_platform_ops {
	/**
	 * Called after clock_control_on() but before any DW core register
	 * access.  Use this to open wrapper clock gates or apply SoC-specific
	 * power sequencing.
	 */
	int (*pre_init)(const struct device *dev);

	/**
	 * Called after RESET_CTRL_ALL to re-open vendor wrapper clock gates
	 * and poll SOFT_RST self-clear before the driver re-enables the core.
	 */
	int (*post_reset)(const struct device *dev);

	/**
	 * Called just before writing DEV_CTRL_RESUME.  Use this to re-open
	 * clock gates so the FIFO-flush bits in RESET_CTRL can self-clear.
	 *
	 * Return 0 when pre-resume conditioning completed, or a negative error
	 * to abort recovery/conditioning paths immediately.
	 */
	int (*pre_resume_ctrl)(const struct device *dev);

	/**
	 * Called when clock control is requested by the driver.
	 * Return 0 if clock is already running (e.g. peri-div auto-enable).
	 */
	int (*clock_on)(const struct device *dev);

	/**
	 * Called after PM_DEVICE_ACTION_RESUME to re-open wrapper clock gates
	 * and re-apply vendor-specific controller configuration.
	 */
	int (*pm_resume)(const struct device *dev);

	/**
	 * Set when writes to DEVICE_CTRL_EXTENDED.DEV_OPERATION_MODE do not read
	 * back on this integration, leaving the field unusable as a role source.
	 * The boot role is then taken from the target-mode property instead.
	 */
	bool dev_operation_mode_write_only;

	/**
	 * Return the I3C core clock rate in Hz.
	 */
	int (*get_clock_rate)(const struct device *dev, uint32_t *rate);

	/**
	 * Called after the controller is enabled (primary or secondary).
	 * is_secondary is true when the controller started in target/secondary
	 * role.
	 */
	void (*post_enable)(const struct device *dev, bool is_secondary);

	/**
	 * Optional target-mode re-arm hook.
	 *
	 * Intended narrowly for target-role RSTDAA handling after DA-valid drops,
	 * where some wrappers need a platform kick before accepting the next
	 * ENTDAA. Not a generic recovery callback.
	 */
	void (*re_arm_target)(const struct device *dev);

	/**
	 * Optional timeout retry policy hook.
	 *
	 * Called on xfer timeout to decide whether to retry and which recovery
	 * level to apply.
	 */
	int (*should_retry_timeout)(const struct device *dev,
			     enum dw_i3c_timeout_op op,
			     bool retried_after_recover,
			     bool first_cmd_addr_nack,
			     enum dw_i3c_retry_action *action);

	/**
	 * Optional CCC-timeout retry policy hook.
	 *
	 * Called on CCC timeout when additional command-signature context is
	 * needed to choose a vendor retry or recovery action.
	 */
	int (*should_retry_ccc_timeout)(const struct device *dev,
					 bool retried_after_recover,
					 bool first_cmd_error_none,
					 bool is_setdasa_direct,
					 bool is_enec_broadcast,
					 enum dw_i3c_retry_action *action);

	/**
	 * Optional ISR transfer-error policy hook.
	 *
	 * Called from generic ISR transfer-completion error path to decide whether
	 * immediate local RESET_CTRL/RESUME recovery is safe in IRQ context.
	 */
	enum dw_i3c_isr_error_action (*isr_error_action)(const struct device *dev,
							 int xfer_error);
};

/**
 * @brief Return true when this instance is configured to start in
 * secondary/target role.
 */
bool dw_i3c_is_secondary_requested(const struct device *dev);

/**
 * @brief Return the MMIO base address of the DW I3C controller.
 *
 * Accessor because struct dw_i3c_config is private to i3c_dw.c.
 *
 * @param dev  Device pointer for a DW I3C controller instance.
 * @return     MMIO base address.
 */
uint32_t dw_i3c_get_regs(const struct device *dev);

/* Resolves to a compile-time NULL when no vendor glue is built, so every hook
 * guard folds to false and the dispatch drops out instead of costing a runtime
 * check on integrations that have no hooks.
 */
#ifdef CONFIG_I3C_DW_PLATFORM_OPS
#define DW_I3C_OPS(cfg) ((cfg)->ops)
#else
#define DW_I3C_OPS(cfg) ((const struct dw_i3c_platform_ops *)NULL)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I3C_I3C_DW_H_ */
