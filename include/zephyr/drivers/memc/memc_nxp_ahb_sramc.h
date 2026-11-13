/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver API for the NXP AHB SRAM Controller.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MEMC_NXP_AHB_SRAMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MEMC_NXP_AHB_SRAMC_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The controller keeps the settings shared between chip selects (data-port
 * width, address mode, ADV# polarity, bus timeout, clock gating) in MCR, and a
 * pair of timing registers (CFG0_CSn, CFG1_CSn) per chip select.
 */

/** Number of bus timings the controller takes per chip select. */
#define MEMC_NXP_AHB_SRAMC_TIMING_COUNT 9

/** Bus timings of one chip select, in the order the driver converts them. */
enum memc_nxp_ahb_sramc_timing {
	/** Time CE# is asserted before the access starts (CFG0_CS[CES]). */
	MEMC_NXP_AHB_SRAMC_TIMING_CE_SETUP,
	/** Time CE# stays asserted after the access ends (CFG0_CS[CEH]). */
	MEMC_NXP_AHB_SRAMC_TIMING_CE_HOLD,
	/** Time the address is driven before the access strobe (CFG0_CS[AS]). */
	MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_SETUP,
	/** Time the address is held after its setup phase (CFG0_CS[AH]). */
	MEMC_NXP_AHB_SRAMC_TIMING_ADDRESS_HOLD,
	/** Time neither side drives the data lines (CFG0_CS[TA]). */
	MEMC_NXP_AHB_SRAMC_TIMING_TURNAROUND,
	/** WE# asserted time, the write pulse width (CFG1_CS[WEL]). */
	MEMC_NXP_AHB_SRAMC_TIMING_WE_LOW,
	/** WE# deasserted time between writes (CFG1_CS[WEH]). */
	MEMC_NXP_AHB_SRAMC_TIMING_WE_HIGH,
	/** OE# asserted time (CFG1_CS[REL]). */
	MEMC_NXP_AHB_SRAMC_TIMING_RE_LOW,
	/** OE# deasserted time between reads (CFG1_CS[REH]). */
	MEMC_NXP_AHB_SRAMC_TIMING_RE_HIGH,
};

/** One external device, as the controller needs to see it. */
struct memc_nxp_ahb_sramc_target {
	/** Bus timings in nanoseconds, indexed by enum memc_nxp_ahb_sramc_timing. */
	uint32_t timing_ns[MEMC_NXP_AHB_SRAMC_TIMING_COUNT];
	/** Chip-select index the device is wired to. */
	uint8_t chip_select;
	/** SRAMC_WP wait pin index the device drives. */
	uint8_t wait_pin;
	/** Wait mode as the register encodes it (CFG0_CS[WM_SEL]). */
	uint8_t wait_mode;
	/** Whether wait-state monitoring is enabled for this chip select. */
	bool wait_enable;
	/** Whether the wait pin is asserted high. */
	bool wait_active_high;
};

/**
 * @brief Program one chip select of the controller for an external device.
 *
 * Uses the finest prescaler the target's timings fit in, converts every
 * nanosecond value into its register field and writes CFG0_CSn/CFG1_CSn. MCR
 * keeps the controller-wide settings of the controller device.
 *
 * @param dev AHB SRAM controller device.
 * @param target Device configuration and timings.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p target is NULL or names a chip select the controller
 *         does not implement.
 * @retval -ENODEV if @p dev is not ready.
 * @retval -EBUSY if that chip select was already configured.
 * @retval -ERANGE if the timings do not fit any prescaler.
 */
int memc_nxp_ahb_sramc_configure_target(const struct device *dev,
					const struct memc_nxp_ahb_sramc_target *target);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MEMC_NXP_AHB_SRAMC_H_ */
