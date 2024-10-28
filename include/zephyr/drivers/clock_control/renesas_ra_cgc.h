/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ra_clock.h>

#define RA_CGC_PROP_HAS_STATUS_OKAY_OR(node_id, prop, default_value)                               \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), (DT_PROP(node_id, prop)), (default_value))

#define RA_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (UTIL_CAT(BSP_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))),        \
		    (BSP_CLOCKS_CLOCK_DISABLED))

#define RA_CGC_CLK_DIV(clk, prop, default_value)                                                   \
	UTIL_CAT(RA_CGC_DIV_, DT_NODE_FULL_NAME_UPPER_TOKEN(clk))                                  \
	(RA_CGC_PROP_HAS_STATUS_OKAY_OR(clk, prop, default_value))

#define RA_CGC_DIV_BCLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_CANFDCLK(n)   UTIL_CAT(BSP_CLOCKS_CANFD_CLOCK_DIV_, n)
#define RA_CGC_DIV_CECCLK(n)     UTIL_CAT(BSP_CLOCKS_CEC_CLOCK_DIV_, n)
#define RA_CGC_DIV_CLKOUT(n)     UTIL_CAT(BSP_CLOCKS_CLKOUT_DIV_, n)
#define RA_CGC_DIV_CPUCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_FCLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_I3CCLK(n)     UTIL_CAT(BSP_CLOCKS_I3C_CLOCK_DIV_, n)
#define RA_CGC_DIV_ICLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_LCDCLK(n)     UTIL_CAT(BSP_CLOCKS_LCD_CLOCK_DIV_, n)
#define RA_CGC_DIV_OCTASPICLK(n) UTIL_CAT(BSP_CLOCKS_OCTA_CLOCK_DIV_, n)
#define RA_CGC_DIV_PCLKA(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_PCLKB(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_PCLKC(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_PCLKD(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_PCLKE(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
#define RA_CGC_DIV_PLL(n)        UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RA_CGC_DIV_PLL2(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
#define RA_CGC_DIV_SCICLK(n)     UTIL_CAT(BSP_CLOCKS_SCI_CLOCK_DIV_, n)
#define RA_CGC_DIV_SPICLK(n)     UTIL_CAT(BSP_CLOCKS_SPI_CLOCK_DIV_, n)
#define RA_CGC_DIV_U60CLK(n)     UTIL_CAT(BSP_CLOCKS_USB60_CLOCK_DIV_, n)
#define RA_CGC_DIV_UCLK(n)       UTIL_CAT(BSP_CLOCKS_USB_CLOCK_DIV_, n)

#define BSP_CLOCKS_SOURCE_PLL  BSP_CLOCKS_SOURCE_CLOCK_PLL
#define BSP_CLOCKS_SOURCE_PLL2 BSP_CLOCKS_SOURCE_CLOCK_PLL

#define BSP_CLOCKS_CLKOUT_DIV_1   (0)
#define BSP_CLOCKS_CLKOUT_DIV_2   (1)
#define BSP_CLOCKS_CLKOUT_DIV_4   (2)
#define BSP_CLOCKS_CLKOUT_DIV_8   (3)
#define BSP_CLOCKS_CLKOUT_DIV_16  (4)
#define BSP_CLOCKS_CLKOUT_DIV_32  (5)
#define BSP_CLOCKS_CLKOUT_DIV_64  (6)
#define BSP_CLOCKS_CLKOUT_DIV_128 (7)

struct clock_control_ra_pclk_cfg {
	uint32_t clk_src;
	uint32_t clk_div;
};

struct clock_control_ra_subsys_cfg {
	uint32_t mstp;
	uint32_t stop_bit;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_ */
