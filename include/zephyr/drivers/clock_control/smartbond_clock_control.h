/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Smartbond clocks
 *
 * Enum oscillators and clocks.
 */
enum smartbond_clock {
	/* Not a clock, used for error case only */
	SMARTBOND_CLK_NONE,
	SMARTBOND_CLK_RC32K,
	SMARTBOND_CLK_RCX,
	SMARTBOND_CLK_XTAL32K,
	SMARTBOND_CLK_RC32M,
	SMARTBOND_CLK_XTAL32M,
	SMARTBOND_CLK_PLL96M,
	SMARTBOND_CLK_USB,
	SMARTBOND_CLK_DIVN_32M,
	SMARTBOND_CLK_HCLK,
	SMARTBOND_CLK_LP_CLK,
	SMARTBOND_CLK_SYS_CLK,
};

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SMARTBOND_CLOCK_CONTROL_H_ */
