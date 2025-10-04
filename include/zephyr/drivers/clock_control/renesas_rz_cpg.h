/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_

#include <zephyr/drivers/clock_control.h>

#define RZ_CPG_DIV(id)            DT_PROP_OR(DT_NODELABEL(id), div, 1)
#define RZ_CPG_PLL_POSTSCALER(id) DT_CLOCKS_CELL(DT_NODELABEL(id), postscaler)
#define RZ_CPG_CLK_SRC(id)        DT_PROP(DT_CLOCKS_CTLR(DT_NODELABEL(id)), clock_frequency)

#define RZ_CPG_GET_CLOCK(id) (RZ_CPG_CLK_SRC(id) / RZ_CPG_PLL_POSTSCALER(id) / RZ_CPG_DIV(id))

#define RZ_CPG_CLK_DIV(clk)                                                                        \
	UTIL_CAT(RZ_CPG_DIV_, DT_NODE_FULL_NAME_UPPER_TOKEN(clk))                                  \
	(DT_PROP(clk, div))

#define RZ_CPG_DIV_ICLK(n)    UTIL_CAT(BSP_CLOCKS_PL1_DIV_, n)
#define RZ_CPG_DIV_P0CLK(n)   UTIL_CAT(BSP_CLOCKS_PL2A_DIV_, n)
#define RZ_CPG_DIV_I2CLK(n)   UTIL_CAT(BSP_CLOCKS_PL3CLK200FIX_DIV_, n)
#define RZ_CPG_DIV_P1CLK(n)   UTIL_CAT(BSP_CLOCKS_PL3B_DIV_, n)
#define RZ_CPG_DIV_P2CLK(n)   UTIL_CAT(BSP_CLOCKS_PL3A_DIV_, n)
#define RZ_CPG_DIV_SPI0CLK(n) UTIL_CAT(BSP_CLOCKS_PL3C_DIV_, n)
#define RZ_CPG_DIV_OC0CLK(n)  UTIL_CAT(BSP_CLOCKS_PL3F_DIV_, n)
#define RZ_CPG_DIV_M3CLK(n)   UTIL_CAT(BSP_CLOCKS_DSIA_DIV_, n)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_ */
