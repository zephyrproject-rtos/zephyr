/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW7X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW7X_H_

/**
 * @file
 * @brief DT binding macros for NXP MCXW7x clock controller
 *
 * These macros are used by device tree clock binding properties to describe
 * clock modes, sources, dividers and control flags for MCXW devices.
 */

/** @name OSC32K Mode */
/*@{*/
#define MCXW_CLK_OSC32K_DISABLE 0 /**< Disable 32K oscillator mode. */
#define MCXW_CLK_OSC32K_ENABLE  1 /**< Enable 32K oscillator mode. */
#define MCXW_CLK_OSC32K_BYPASS  3 /**< Bypass 32K oscillator (external clock). */
/*@}*/

/** @name FIRC Mode */
/*@{*/
#define MCXW_CLK_FIRC_DISABLE      0 /**< Disable FIRC. */
#define MCXW_CLK_FIRC_ENABLE       1 /**< Enable FIRC. */
#define MCXW_CLK_FIRC_ENABLE_IN_LP 2 /**< Enable FIRC in low-power mode. */
/*@}*/

/** @name FIRC Range */
/*@{*/
#define MCXW_CLK_FIRC_RANGE_48MHZ  0 /**< FIRC range: 48 MHz. */
#define MCXW_CLK_FIRC_RANGE_64MHZ  1 /**< FIRC range: 64 MHz. */
#define MCXW_CLK_FIRC_RANGE_96MHZ  2 /**< FIRC range: 96 MHz. */
#define MCXW_CLK_FIRC_RANGE_192MHZ 3 /**< FIRC range: 192 MHz. */
/*@}*/

/** @name System clock source */
/*@{*/
#define MCXW_CLK_SYSTEM_CLK_SRC_SOSC 1 /**< System clock: SOSC (external oscillator). */
#define MCXW_CLK_SYSTEM_CLK_SRC_SIRC 2 /**< System clock: SIRC (internal slow RC). */
#define MCXW_CLK_SYSTEM_CLK_SRC_FIRC 3 /**< System clock: FIRC (fast internal RC). */
#define MCXW_CLK_SYSTEM_CLK_SRC_ROSC 4 /**< System clock: ROSC (ring oscillator). */
/*@}*/

/** @name System clock dividers
 * Values map to kSCG_SysClkDivByX definitions used by MCXW platform code.
 */
/*@{*/
#define MCXW_CLK_DIV_NONE  0  /**< No divider change. */
#define MCXW_CLK_DIV_BY_1  0  /**< Divide by 1 (kSCG_SysClkDivBy1). */
#define MCXW_CLK_DIV_BY_2  1  /**< Divide by 2 (kSCG_SysClkDivBy2). */
#define MCXW_CLK_DIV_BY_3  2  /**< Divide by 3 (kSCG_SysClkDivBy3). */
#define MCXW_CLK_DIV_BY_4  3  /**< Divide by 4 (kSCG_SysClkDivBy4). */
#define MCXW_CLK_DIV_BY_5  4  /**< Divide by 5 (kSCG_SysClkDivBy5). */
#define MCXW_CLK_DIV_BY_6  5  /**< Divide by 6 (kSCG_SysClkDivBy6). */
#define MCXW_CLK_DIV_BY_7  6  /**< Divide by 7 (kSCG_SysClkDivBy7). */
#define MCXW_CLK_DIV_BY_8  7  /**< Divide by 8 (kSCG_SysClkDivBy8). */
#define MCXW_CLK_DIV_BY_10 9  /**< Divide by 10 (kSCG_SysClkDivBy10). */
#define MCXW_CLK_DIV_BY_11 10 /**< Divide by 11 (kSCG_SysClkDivBy11). */
#define MCXW_CLK_DIV_BY_12 11 /**< Divide by 12 (kSCG_SysClkDivBy12). */
#define MCXW_CLK_DIV_BY_13 12 /**< Divide by 13 (kSCG_SysClkDivBy13). */
#define MCXW_CLK_DIV_BY_14 13 /**< Divide by 14 (kSCG_SysClkDivBy14). */
#define MCXW_CLK_DIV_BY_15 14 /**< Divide by 15 (kSCG_SysClkDivBy15). */
#define MCXW_CLK_DIV_BY_16 15 /**< Divide by 16 (kSCG_SysClkDivBy16). */
/*@}*/

/** @name IP mux clock sources
 * Values used to select the peripheral clock source via ip mux selection.
 */
/*@{*/
#define MCXW_CLK_IP_MUX_SLOW_CLK 1 /**< Slow clock source. */
/**
 * @note ERR052742: the FRO6M clock (kCLOCK_IpSrcFro6M) may be unstable on some
 * parts: it can output a lower-than-expected frequency after reset or low-
 * power wakeup. Avoid using FRO6M where stability is required; prefer
 * higher-frequency FRO sources when possible.
 */
#define MCXW_CLK_IP_MUX_FRO_6M   2 /**< FRO 6 MHz clock (kCLOCK_IpSrcFro6M). */
#define MCXW_CLK_IP_MUX_FRO_192M_DIV                                                               \
	3 /**< FRO 192 MHz divided clock (frequency depends on range). */
#define MCXW_CLK_IP_MUX_SOSC         4 /**< SOSC: RF oscillator clock (kCLOCK_IpSrcSoscClk). */
#define MCXW_CLK_IP_MUX_32K          5 /**< 32 kHz clock (kCLOCK_IpSrc32kClk). */
#define MCXW_CLK_IP_MUX_FRO_200M_DIV 6 /**< FRO 200 MHz (for CAN); only MCXW70 supports this. */
#define MCXW_CLK_IP_MUX_1M           7 /**< 1 MHz derived from FRO6M; only MCXW70 supports this. */
#define MCXW_CLK_IP_MUX_NONE         0xFF /**< No IP mux selection / invalid value. */
/*@}*/

/** @name Clock control flags
 * These flags match the kCLOCK_IpClkControl_xxx values used by the driver.
 */
/*@{*/
#define MCXW_CLK_CTRL_DISABLED 0  /**< Clock disabled (kCLOCK_IpClkControl_fun0). */
#define MCXW_CLK_CTRL_ENABLED  1U /**< Clock enabled (kCLOCK_IpClkControl_fun1). */
/*@}*/

/** @name Low power behavior flags */
/*@{*/
#define MCXW_CLK_LP_STALL_ALLOWED 0U /**< Clock can prevent low-power entry (will stall). */
#define MCXW_CLK_LP_NO_STALL      2U /**< Clock won't prevent low-power entry (no stall). */
/*@}*/

/** @name Combined control modes */
/*@{*/
#define MCXW_CLK_MODE_DISABLED (MCXW_CLK_CTRL_DISABLED) /**< Disabled mode. */
#define MCXW_CLK_MODE_ENABLED_LP_STALL                                                             \
	(MCXW_CLK_CTRL_ENABLED | MCXW_CLK_LP_STALL_ALLOWED) /**< Enabled; may stall LP. */
#define MCXW_CLK_MODE_ENABLED_LP_NO_STALL                                                          \
	(MCXW_CLK_CTRL_ENABLED | MCXW_CLK_LP_NO_STALL) /**< Enabled; won't stall LP. */
/*@}*/

/**
 * Helper macro to encode a clock configuration into a 32-bit integer used in
 * device tree bindings.
 *
 * Bit layout (from MSB to LSB):
 * - [31:16] mrcc_offset (16 bits)
 * - [15:8]  clk_mux     (8 bits)
 * - [7:4]   clk_div     (4 bits)
 * - [3:0]   flag        (4 bits)
 *
 * @param mrcc_offset MRCC register offset (16-bit)
 * @param clk_mux      Clock multiplexer selection (8-bit)
 * @param clk_div      Clock divider value (4-bit)
 * @param flag         Control/behavior flags (4-bit)
 */
#define MCXW_CLOCK(mrcc_offset, clk_mux, clk_div, flag)                                            \
	((((mrcc_offset) & 0xFFFF) << 16) | (((clk_mux) & 0xFF) << 8) | (((clk_div) & 0xF) << 4) | \
	 (((flag) & 0xF)))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW7X_H_ */
