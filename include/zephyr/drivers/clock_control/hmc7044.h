/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HMC7044 clock generator -- clock_control extension API.
 *
 * The HMC7044 is driven through the standard Zephyr clock_control API:
 *
 *   clock_control_get_rate(dev, HMC7044_CLK_OUT(12), &hz)
 *   clock_control_get_status(dev, CLOCK_CONTROL_SUBSYS_ALL)
 *   clock_control_on(dev, HMC7044_CLK_OUT(3))
 *   clock_control_off(dev, HMC7044_CLK_OUT(3))
 *
 * set_rate, async_on and configure are deliberately not implemented -- the
 * public wrappers return -ENOSYS for them. Every output rate comes from
 * devicetree and retuning a divider under a live JESD204 link would break the
 * link.
 *
 * This header adds the two things the standard API cannot express: a SYSREF
 * pulse request and PLL-level status detail, following the established pattern
 * for vendor-specific clock_control extension ops.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HMC7044_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HMC7044_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HMC7044 clock generator interface
 * @defgroup hmc7044_interface HMC7044 clock generator interface
 * @ingroup clock_control_interface
 * @{
 */

/** Number of clock outputs on an HMC7044. */
#define HMC7044_NUM_CLK_OUT 14

/**
 * @brief Build a clock_control_subsys_t handle for output @p n (0..13).
 *
 * The handle is the output number biased by one: output 0 is a real output, and
 * an unbiased handle of 0 would be indistinguishable from
 * CLOCK_CONTROL_SUBSYS_ALL (which is NULL) in the subsystem API.
 */
#define HMC7044_CLK_OUT(n) ((clock_control_subsys_t)(uintptr_t)((n) + 1))

/**
 * @brief Request a single SYSREF pulse stream.
 *
 * SYSREF is not a clock: it is a JESD204 subclass-1 alignment pulse whose
 * *phase* relative to the link's LMFC matters, so it does not fit
 * clock_control_subsys_t and is not reachable through clock_control_on().
 *
 * This toggles PULSE_GEN_REQ, which emits the number of pulses selected by
 * adi,pulse-generator-mode on every output configured for dynamic startup.
 * It is a no-op on outputs running in continuous SYSREF mode.
 *
 * NOTE: untested on hardware. On a board whose SYSREF outputs run continuously
 * -- the common JESD204 subclass-1 arrangement, where alignment happens against
 * a free-running SYSREF and nothing ever requests a pulse -- this path is never
 * taken. It exists for a board that gates SYSREF instead.
 *
 * @param dev HMC7044 device.
 * @retval 0 on success.
 * @retval -EINVAL if @p dev is not an HMC7044.
 * @retval -EIO on an SPI transfer failure.
 */
int hmc7044_sysref_request(const struct device *dev);

/** @brief PLL and reference status, as read back from the chip. */
struct hmc7044_status {
	/** PLL1 FSM state, register 0x82 bits [2:0]. 2 means locked. */
	uint8_t pll1_fsm_state;
	/** Human-readable form of @ref pll1_fsm_state. */
	const char *pll1_fsm_state_str;
	/** Which CLKINx PLL1 has currently selected, 0-3. */
	uint8_t pll1_active_clkin;
	/** Frequency in Hz of the active CLKINx, from devicetree. */
	uint32_t pll1_active_clkin_freq;
	/** PLL1 phase-frequency-detector rate in kHz, as solved at init. */
	uint32_t pll1_pfd_khz;
	/** True when PLL1 reports the locked FSM state. */
	bool pll1_locked;
	/** True when PLL2's lock detect is asserted (alarm register 0x7D). */
	bool pll2_locked;
	/** PLL2 VCO frequency in Hz, from devicetree. */
	uint32_t pll2_freq;
};

/**
 * @brief Read back PLL1/PLL2 lock and reference-selection detail.
 *
 * clock_control_get_status() collapses this to a single
 * enum clock_control_status; this returns the underlying detail, which is what
 * a JESD204 bring-up failure needs in order to be diagnosable.
 *
 * Performs SPI reads, so it must be called from thread context.
 *
 * @param dev    HMC7044 device.
 * @param status Destination, populated on success only.
 * @retval 0 on success.
 * @retval -EINVAL if @p dev is not an HMC7044, or @p status is NULL.
 * @retval -EIO on an SPI transfer failure.
 */
int hmc7044_get_status(const struct device *dev, struct hmc7044_status *status);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HMC7044_H_ */
