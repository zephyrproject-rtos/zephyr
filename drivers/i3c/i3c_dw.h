/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_I3C_DW_H_
#define ZEPHYR_DRIVERS_I3C_I3C_DW_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

/**
 * @brief Get the DW I3C core register base address for a device instance.
 *
 * The full per-instance config struct is kept private so its layout can
 * evolve without breaking vendor ports.
 *
 * @param dev  I3C controller device pointer.
 * @return I3C block base address (value of the DTS reg property).
 */
uint32_t dw_i3c_get_regs(const struct device *dev);

/**
 * @brief Optional vendor/SoC integration hooks for the Synopsys DW I3C core.
 *
 * Needed only when the SoC requires register accesses outside the DW IP space
 * (wrapper gates, group clocks, errata, etc.).  Every member is optional; NULL
 * means "use the generic default behaviour"
 */
struct dw_i3c_platform_ops {
	/** Get I3C protocol clock rate in Hz.  Default: clock_control_get_rate(). */
	int (*get_clock_rate)(const struct device *dev, uint32_t *rate);

	/** Turn on the I3C protocol clock.  Default: clock_control_on(). */
	int (*clock_on)(const struct device *dev);

	/** Pre-init after clock-on but before any DW register access.  Default: no-op. */
	int (*pre_init)(const struct device *dev);

	/** Post-reset: re-apply DW register writes cleared by RESET_CTRL_SOFT.  Default: no-op. */
	int (*post_reset)(const struct device *dev);

	/** Called between RESET_CTRL flush and DEV_CTRL_RESUME in dw_i3c_end_xfer().
	 * Default: no-op.
	 */
	void (*pre_resume_ctrl)(const struct device *dev);

	/** PM resume: restore DW core register accessibility after deep sleep.  Default: no-op. */
	int (*pm_resume)(const struct device *dev);

	/** Override secondary-controller detection.  Default: read DEVICE_CTRL_EXTENDED. */
	bool (*is_secondary_controller)(const struct device *dev);
};

#endif /* ZEPHYR_DRIVERS_I3C_I3C_DW_H_ */
