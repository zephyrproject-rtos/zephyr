/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AD9523(-1) clock generator -- clock_control extension API.
 *
 * The AD9523-1 is driven through the standard Zephyr clock_control API:
 *
 *   clock_control_get_rate(dev, AD9523_CLK_OUT(13), &hz)
 *   clock_control_get_status(dev, CLOCK_CONTROL_SUBSYS_ALL)
 *   clock_control_on(dev, AD9523_CLK_OUT(1))
 *   clock_control_off(dev, AD9523_CLK_OUT(1))
 *
 * set_rate, async_on and configure are deliberately not implemented -- the
 * public wrappers return -ENOSYS for them. Every output rate comes from
 * devicetree and retuning a divider under a live JESD204 link would break the
 * link.
 *
 * This header adds the two things the standard API cannot express: a chip-level
 * SYNC and PLL-level lock status detail.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AD9523_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AD9523_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AD9523 clock generator interface
 * @defgroup ad9523_interface AD9523 clock generator interface
 * @ingroup clock_control_interface
 * @{
 */

/** Number of clock outputs on an AD9523. */
#define AD9523_NUM_CLK_OUT 14

/**
 * @brief Build a clock_control_subsys_t handle for output @p n (0..13).
 *
 * The handle is the output number biased by one: output 0 is a real output, and
 * an unbiased handle of 0 would be indistinguishable from
 * CLOCK_CONTROL_SUBSYS_ALL (which is NULL) in the subsystem API.
 */
#define AD9523_CLK_OUT(n) ((clock_control_subsys_t)(uintptr_t)((n) + 1))

/** @brief PLL, VCXO and VCO status, as read back from the chip. */
struct ad9523_status {
	/** True when the VCXO status bit is asserted (readback 0x22C bit 5). */
	bool vcxo_locked;
	/** True when PLL1 lock detect is asserted (readback 0x22C bit 0). */
	bool pll1_locked;
	/** True when PLL2 lock detect is asserted (readback 0x22C bit 1). */
	bool pll2_locked;
	/**
	 * True when PLL1 is bypassed (single-loop mode). PLL1 lock has no
	 * meaning in this mode, so @ref pll1_locked is not evaluated.
	 */
	bool pll1_bypassed;
	/** PLL2 VCO frequency in Hz, as solved at init. */
	uint32_t vco_freq;
};

/**
 * @brief Read back VCXO/PLL1/PLL2 lock detail.
 *
 * clock_control_get_status() collapses this to a single
 * enum clock_control_status; this returns the underlying detail, which is what
 * a JESD204 bring-up failure needs in order to be diagnosable.
 *
 * Performs SPI reads, so it must be called from thread context.
 *
 * @param dev    AD9523 device.
 * @param status Destination, populated on success only.
 * @retval 0 on success.
 * @retval -EINVAL if @p dev is not an AD9523, or @p status is NULL.
 * @retval -EIO on an SPI transfer failure.
 */
int ad9523_get_status(const struct device *dev, struct ad9523_status *status);

/**
 * @brief Issue a chip-level SYNC.
 *
 * Toggles SYNC_MAN_CTRL (status-signals register 0x232 bit 16) with an
 * io_update on each edge, re-aligning every output divider to its programmed
 * phase.
 *
 * Performs SPI transfers, so it must be called from thread context.
 *
 * @param dev AD9523 device.
 * @retval 0 on success.
 * @retval -EINVAL if @p dev is not an AD9523.
 * @retval -EIO on an SPI transfer failure.
 */
int ad9523_sync(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AD9523_H_ */
