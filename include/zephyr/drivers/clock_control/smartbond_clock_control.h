/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Renesas SmartBond devices.
 * @ingroup clock_control_smartbond
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

/**
 * @defgroup clock_control_smartbond Renesas SmartBond
 * @ingroup clock_control_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SmartBond oscillators and clocks. */
enum smartbond_clock {
	SMARTBOND_CLK_NONE,     /**< Not a clock; used for the error case only. */
	SMARTBOND_CLK_RC32K,    /**< 32 kHz RC oscillator. */
	SMARTBOND_CLK_RCX,      /**< RCX low-power oscillator. */
	SMARTBOND_CLK_XTAL32K,  /**< 32 kHz crystal oscillator. */
	SMARTBOND_CLK_RC32M,    /**< 32 MHz RC oscillator. */
	SMARTBOND_CLK_XTAL32M,  /**< 32 MHz crystal oscillator. */
	SMARTBOND_CLK_PLL96M,   /**< 96 MHz PLL. */
	SMARTBOND_CLK_USB,      /**< USB clock. */
	SMARTBOND_CLK_DIVN_32M, /**< Divided 32 MHz clock. */
	SMARTBOND_CLK_HCLK,     /**< AHB clock. */
	SMARTBOND_CLK_LP_CLK,   /**< Low-power clock. */
	SMARTBOND_CLK_SYS_CLK,  /**< System clock. */
};

/** @cond INTERNAL_HIDDEN */

/** @brief Selects system clock
 *
 * @param sys_clk system clock to switch to.
 *
 * @return 0 on success
 */
int z_smartbond_select_sys_clk(enum smartbond_clock sys_clk);

/** @brief Selects low power clock
 *
 * @param lp_clk low power clock to switch to.
 *
 * @return 0 on success
 */
int z_smartbond_select_lp_clk(enum smartbond_clock lp_clk);

/** @endcond */

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_ */
