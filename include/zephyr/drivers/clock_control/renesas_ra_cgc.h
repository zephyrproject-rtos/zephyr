/*
 * Copyright (c) 2024-2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA Clock Generator Circuit (CGC) header file
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ra_clock.h>

/**
 * @name Clock source and divider helpers for Renesas RA devices.
 *
 * @{
 */

/**
 * @brief Conditional property getter based on devicetree status.
 *
 * Returns the value of a devicetree property if the node is marked as `okay`.
 * Otherwise, the provided default value is used.
 */
#define RA_CGC_PROP_HAS_STATUS_OKAY_OR(node_id, prop, default_value)                               \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), \
		(DT_PROP(node_id, prop)), (default_value))

/**
 * @brief Helper to get clock source form device tree.
 *
 * Expands to a BSP_CLOCKS_SOURCE_* constant when the node is enabled, or
 * BSP_CLOCKS_CLOCK_DISABLED otherwise.
 */
#define RA_CGC_CLK_SRC(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay), \
		(UTIL_CAT(BSP_CLOCKS_SOURCE_, DT_NODE_FULL_NAME_UPPER_TOKEN(node_id))), \
		(BSP_CLOCKS_CLOCK_DISABLED))

/**
 * @brief Helper to compute a clock divider.
 *
 * This macro expands to a RA_CGC_DIV_* prefix combined with a devicetree value.
 */
#define RA_CGC_CLK_DIV(clk, prop, default_value)                                                   \
	UTIL_CAT(RA_CGC_DIV_, DT_NODE_FULL_NAME_UPPER_TOKEN(clk))                                  \
	(RA_CGC_PROP_HAS_STATUS_OKAY_OR(clk, prop, default_value))

/**
 * @defgroup ra_cgc_div Renesas RA Clock Divider Generators
 * @brief Divider generator macros for multiple clock domains.
 * @{
 */

/** BCLK divider. */
#define RA_CGC_DIV_BCLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** CANFD divider. */
#define RA_CGC_DIV_CANFDCLK(n)   UTIL_CAT(BSP_CLOCKS_CANFD_CLOCK_DIV_, n)
/** CEC divider. */
#define RA_CGC_DIV_CECCLK(n)     UTIL_CAT(BSP_CLOCKS_CEC_CLOCK_DIV_, n)
/** CLKOUT divider. */
#define RA_CGC_DIV_CLKOUT(n)     UTIL_CAT(BSP_CLOCKS_CLKOUT_DIV_, n)
/** CPUCLK0 divider. */
#define RA_CGC_DIV_CPUCLK0(n)    UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** CPUCLK1 divider. */
#define RA_CGC_DIV_CPUCLK1(n)    UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** MRPCLK divider. */
#define RA_CGC_DIV_MRPCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** CPUCLK divider. */
#define RA_CGC_DIV_CPUCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** FCLK divider. */
#define RA_CGC_DIV_FCLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** I3C divider. */
#define RA_CGC_DIV_I3CCLK(n)     UTIL_CAT(BSP_CLOCKS_I3C_CLOCK_DIV_, n)
/** ICLK divider. */
#define RA_CGC_DIV_ICLK(n)       UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** LCDCLK divider. */
#define RA_CGC_DIV_LCDCLK(n)     UTIL_CAT(BSP_CLOCKS_LCD_CLOCK_DIV_, n)
/** OCTASPI divider. */
#define RA_CGC_DIV_OCTASPICLK(n) UTIL_CAT(BSP_CLOCKS_OCTA_CLOCK_DIV_, n)
/** PCLKA divider. */
#define RA_CGC_DIV_PCLKA(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** PCLKB divider. */
#define RA_CGC_DIV_PCLKB(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** PCLKC divider. */
#define RA_CGC_DIV_PCLKC(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** PCLKD divider. */
#define RA_CGC_DIV_PCLKD(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** PCLKE divider. */
#define RA_CGC_DIV_PCLKE(n)      UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** PLL divider. */
#define RA_CGC_DIV_PLL(n)        UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLLP divider. */
#define RA_CGC_DIV_PLLP(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLLQ divider. */
#define RA_CGC_DIV_PLLQ(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLLR divider. */
#define RA_CGC_DIV_PLLR(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLL2 divider. */
#define RA_CGC_DIV_PLL2(n)       UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLL2P divider. */
#define RA_CGC_DIV_PLL2P(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLL2Q divider. */
#define RA_CGC_DIV_PLL2Q(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** PLL2R divider. */
#define RA_CGC_DIV_PLL2R(n)      UTIL_CAT(BSP_CLOCKS_PLL_DIV_, n)
/** SCICLK divider. */
#define RA_CGC_DIV_SCICLK(n)     UTIL_CAT(BSP_CLOCKS_SCI_CLOCK_DIV_, n)
/** SPICLK divider. */
#define RA_CGC_DIV_SPICLK(n)     UTIL_CAT(BSP_CLOCKS_SPI_CLOCK_DIV_, n)
/** U60CLK divider. */
#define RA_CGC_DIV_U60CLK(n)     UTIL_CAT(BSP_CLOCKS_USB60_CLOCK_DIV_, n)
/** UCLK divider. */
#define RA_CGC_DIV_UCLK(n)       UTIL_CAT(BSP_CLOCKS_USB_CLOCK_DIV_, n)
/** SCISPI divider. */
#define RA_CGC_DIV_SCISPICLK(n)  UTIL_CAT(BSP_CLOCKS_SCISPI_CLOCK_DIV_, n)
/** GPTCLK divider. */
#define RA_CGC_DIV_GPTCLK(n)     UTIL_CAT(BSP_CLOCKS_GPT_CLOCK_DIV_, n)
/** IICCLK divider. */
#define RA_CGC_DIV_IICCLK(n)     UTIL_CAT(BSP_CLOCKS_IIC_CLOCK_DIV_, n)
/** ADCCLK divider. */
#define RA_CGC_DIV_ADCCLK(n)     UTIL_CAT(BSP_CLOCKS_ADC_CLOCK_DIV_, n)
/** MRICLK divider. */
#define RA_CGC_DIV_MRICLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** NPUCLK divider. */
#define RA_CGC_DIV_NPUCLK(n)     UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** BCLKA divider. */
#define RA_CGC_DIV_BCLKA(n)      UTIL_CAT(BSP_CLOCKS_BCLKA_CLOCK_DIV_, n)
/** ESWCLK divider. */
#define RA_CGC_DIV_ESWCLK(n)     UTIL_CAT(BSP_CLOCKS_ESW_CLOCK_DIV_, n)
/** ESWPHYCLK divider. */
#define RA_CGC_DIV_ESWPHYCLK(n)  UTIL_CAT(BSP_CLOCKS_ESWPHY_CLOCK_DIV_, n)
/** ETHPHY divider. */
#define RA_CGC_DIV_ETHPHYCLK(n)  UTIL_CAT(BSP_CLOCKS_ETHPHY_CLOCK_DIV_, n)
/** ESCCLK divider. */
#define RA_CGC_DIV_ESCCLK(n)     UTIL_CAT(BSP_CLOCKS_ESC_CLOCK_DIV_, n)
/** DSMIFCLK divider. */
#define RA_CGC_DIV_DSMIFCLK(n)   UTIL_CAT(BSP_CLOCKS_DSMIF_CLOCK_DIV_, n)

/** XTALDIV_CLK divider. */
#define RA_CGC_DIV_XTALDIV_CLK(n) UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** HOCODIV_CLK divider. */
#define RA_CGC_DIV_HOCODIV_CLK(n) UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** MOCODIV_CLK divider. */
#define RA_CGC_DIV_MOCODIV_CLK(n) UTIL_CAT(BSP_CLOCKS_SYS_CLOCK_DIV_, n)
/** SAU_CK00 divider. */
#define RA_CGC_DIV_SAU_CK00(n)    UTIL_CAT(BSP_CLOCKS_SAU_CLOCK_DIV_, n)
/** SAU_CK01 divider. */
#define RA_CGC_DIV_SAU_CK01(n)    UTIL_CAT(BSP_CLOCKS_SAU_CLOCK_DIV_, n)
/** SAU_CK10 divider. */
#define RA_CGC_DIV_SAU_CK10(n)    UTIL_CAT(BSP_CLOCKS_SAU_CLOCK_DIV_, n)
/** SAU_CK11 divider. */
#define RA_CGC_DIV_SAU_CK11(n)    UTIL_CAT(BSP_CLOCKS_SAU_CLOCK_DIV_, n)
/** TAU_CK01 divider. */
#define RA_CGC_DIV_TAU_CK01(n)    UTIL_CAT(TIMER_SOURCE_DIV_, n)
/** TAU_CK02 divider. */
#define RA_CGC_DIV_TAU_CK02(n)    UTIL_CAT(TIMER_SOURCE_DIV_, n)
/** TAU_CK03 divider. */
#define RA_CGC_DIV_TAU_CK03(n)    UTIL_CAT(TIMER_SOURCE_DIV_, n)
/** TAU_CK04 divider. */
#define RA_CGC_DIV_TAU_CK04(n)    UTIL_CAT(TIMER_SOURCE_DIV_, n)
/** @} end of ra_cgc_div group */

/**
 *  @defgroup bsp_clock_source Renesas RA BSP Clock Source Constants
 *  @brief Renesas RA BSP clock source constants.
 *  @{
 */

/** PLL clock source. */
#define BSP_CLOCKS_SOURCE_PLL   BSP_CLOCKS_SOURCE_CLOCK_PLL
/** PLLP clock source. */
#define BSP_CLOCKS_SOURCE_PLLP  BSP_CLOCKS_SOURCE_CLOCK_PLL
/** PLLQ clock source. */
#define BSP_CLOCKS_SOURCE_PLLQ  BSP_CLOCKS_SOURCE_CLOCK_PLL1Q
/** PLLR clock source. */
#define BSP_CLOCKS_SOURCE_PLLR  BSP_CLOCKS_SOURCE_CLOCK_PLL1R
/** PLL2 clock source. */
#define BSP_CLOCKS_SOURCE_PLL2  BSP_CLOCKS_SOURCE_CLOCK_PLL2
/** PLL2P clock source. */
#define BSP_CLOCKS_SOURCE_PLL2P BSP_CLOCKS_SOURCE_CLOCK_PLL2
/** PLL2Q clock source. */
#define BSP_CLOCKS_SOURCE_PLL2Q BSP_CLOCKS_SOURCE_CLOCK_PLL2Q
/** PLL2R clock source. */
#define BSP_CLOCKS_SOURCE_PLL2R BSP_CLOCKS_SOURCE_CLOCK_PLL2R

/** XTALDIV_CLK clock source */
#define BSP_CLOCKS_SOURCE_XTALDIV_CLK BSP_CLOCKS_SOURCE_CLOCK_MAIN_OSC
/** HOCODIV_CLK clock source */
#define BSP_CLOCKS_SOURCE_HOCODIV_CLK BSP_CLOCKS_SOURCE_CLOCK_HOCO
/** MOCODIV_CLK clock source */
#define BSP_CLOCKS_SOURCE_MOCODIV_CLK BSP_CLOCKS_SOURCE_CLOCK_MOCO

/** PCLK clock source. */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pclk))
#if DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(hoco))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_CLOCK_HOCO
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(moco))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_CLOCK_MOCO
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(loco))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_CLOCK_LOCO
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(xtal))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_CLOCK_MAIN_OSC
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(subclk))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_CLOCK_SUBCLOCK
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(hocodiv_clk))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_HOCODIV_CLK
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(mocodiv_clk))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_MOCODIV_CLK
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pclk)), DT_NODELABEL(xtaldiv_clk))
#define BSP_CLOCKS_SOURCE_PCLK BSP_CLOCKS_SOURCE_XTALDIV_CLK
#else
#error "Invalid pclk source clock"
#endif
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pclk)) */

/** FSXP clock source. */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fsxp))
#if DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(fsxp)), DT_NODELABEL(loco))
#define BSP_CLOCKS_SOURCE_FSXP BSP_CLOCKS_SOURCE_CLOCK_LOCO
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(fsxp)), DT_NODELABEL(subclk))
#define BSP_CLOCKS_SOURCE_FSXP BSP_CLOCKS_SOURCE_CLOCK_SUBCLOCK
#else
#error "Invalid fsxp source clock"
#endif /* DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(fsxp)), DT_NODELABEL(loco)) */
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fsxp)) */
/** @} end of bsp_clock_source group */

/**
 *  @defgroup bsp_clock_clkout Renesas RA BSP Clock Clkout Divider Constants
 *  @brief Renesas RA BSP clock clkout divider constants.
 *  @{
 */

#define BSP_CLOCKS_CLKOUT_DIV_1   (0) /**< Clkout div 1. */
#define BSP_CLOCKS_CLKOUT_DIV_2   (1) /**< Clkout div 2. */
#define BSP_CLOCKS_CLKOUT_DIV_4   (2) /**< Clkout div 4. */
#define BSP_CLOCKS_CLKOUT_DIV_8   (3) /**< Clkout div 8. */
#define BSP_CLOCKS_CLKOUT_DIV_16  (4) /**< Clkout div 16. */
#define BSP_CLOCKS_CLKOUT_DIV_32  (5) /**< Clkout div 32. */
#define BSP_CLOCKS_CLKOUT_DIV_64  (6) /**< Clkout div 64. */
#define BSP_CLOCKS_CLKOUT_DIV_128 (7) /**< Clkout div 128. */
/** @} end of bsp_clock_clkout group */

/**
 * @brief Peripheral clock configuration.
 */
struct clock_control_ra_pclk_cfg {
	uint32_t clk_src; /**< Clock source selection. */
	uint32_t clk_div; /**< Clock divider value. */
};

/**
 * @brief Subsystem clock control configuration.
 */
struct clock_control_ra_subsys_cfg {
	uint32_t mstp;     /**< MSTP register index. */
	uint32_t stop_bit; /**< Clock stop bit. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_ */
