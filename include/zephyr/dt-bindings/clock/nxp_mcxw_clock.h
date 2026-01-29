/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW_H_

/* OSC32K Mode */
#define MCXW_CLK_OSC32K_DISABLE 0
#define MCXW_CLK_OSC32K_ENABLE  1
#define MCXW_CLK_OSC32K_BYPASS  3

/* FIRC Mode */
#define MCXW_CLK_FIRC_DISABLE      0
#define MCXW_CLK_FIRC_ENABLE       1
#define MCXW_CLK_FIRC_ENABLE_IN_LP 2

/* FIRC Range */
#define MCXW_CLK_FIRC_RANGE_48MHZ  0
#define MCXW_CLK_FIRC_RANGE_64MHZ  1
#define MCXW_CLK_FIRC_RANGE_96MHZ  2
#define MCXW_CLK_FIRC_RANGE_192MHZ 3

/* System clock source. */
#define MCXW_CLK_SYSTEM_CLK_SRC_SOSC 1
#define MCXW_CLK_SYSTEM_CLK_SRC_SIRC 2
#define MCXW_CLK_SYSTEM_CLK_SRC_FIRC 3
#define MCXW_CLK_SYSTEM_CLK_SRC_ROSC 4

#define MCXW_CLK_DIV_NONE  0  /* No divider change */
#define MCXW_CLK_DIV_BY_1  0  /* kSCG_SysClkDivBy1 */
#define MCXW_CLK_DIV_BY_2  1  /* kSCG_SysClkDivBy2 */
#define MCXW_CLK_DIV_BY_3  2  /* kSCG_SysClkDivBy3 */
#define MCXW_CLK_DIV_BY_4  3  /* kSCG_SysClkDivBy4 */
#define MCXW_CLK_DIV_BY_5  4  /* kSCG_SysClkDivBy5 */
#define MCXW_CLK_DIV_BY_6  5  /* kSCG_SysClkDivBy6 */
#define MCXW_CLK_DIV_BY_7  6  /* kSCG_SysClkDivBy7 */
#define MCXW_CLK_DIV_BY_8  7  /* kSCG_SysClkDivBy8 */
#define MCXW_CLK_DIV_BY_10 9  /* kSCG_SysClkDivBy10 */
#define MCXW_CLK_DIV_BY_11 10 /* kSCG_SysClkDivBy11 */
#define MCXW_CLK_DIV_BY_12 11 /* kSCG_SysClkDivBy12 */
#define MCXW_CLK_DIV_BY_13 12 /* kSCG_SysClkDivBy13 */
#define MCXW_CLK_DIV_BY_14 13 /* kSCG_SysClkDivBy14 */
#define MCXW_CLK_DIV_BY_15 14 /* kSCG_SysClkDivBy15 */
#define MCXW_CLK_DIV_BY_16 15 /* kSCG_SysClkDivBy16 */

#define MCXW_CLK_IP_MUX_SLOW_CLK 1 /* Slow clock. */
/*
 * ERR052742: FRO6M clock(kCLOCK_IpSrcFro6M) is not stable.
 * The FRO6M clock is not stable on some parts. FRO6M outputs lower frequency
 * signal instead of 6MHz when device is reset or wakes up from low power.
 * It can impact peripherals using it as a clock source. Please use clock source
 * other than the FRO6M. For example, use FRO192M instead of FRO6M as clock
 * source for peripherals.
 */
#define MCXW_CLK_IP_MUX_FRO_6M   2 /* kCLOCK_IpSrcFro6M, FRO 6M clock. */
#define MCXW_CLK_IP_MUX_FRO_192M_DIV                                                               \
	3 /* FRO 192M Divided clock(The frequency is dependent on range). */
#define MCXW_CLK_IP_MUX_SOSC          4 /* kCLOCK_IpSrcSoscClk, OSC RF clock. */
#define MCXW_CLK_IP_MUX_32K           5 /* kCLOCK_IpSrc32kClk, 32k Clk clock. */
#define MCXW_CLK_IP_MUX_FRO_200M_DIV 6 /* FRO 200M clock for CAN, only MCXW70 support this. */
#define MCXW_CLK_IP_MUX_1M            7 /* 1MHz derived out of FRO6M, only MCXW70 support this. */
#define MCXW_CLK_IP_MUX_NONE          0xFF

/* clock control flags - matches kCLOCK_IpClkControl_xxx from clock driver. */
#define MCXW_CLK_CTRL_DISABLED 0  /* kCLOCK_IpClkControl_fun0  */
#define MCXW_CLK_CTRL_ENABLED  1U /* kCLOCK_IpClkControl_fun1 */

/* Low power mode behavior flags */
#define MCXW_CLK_LP_STALL_ALLOWED 0U /* Can prevent low power entry */
#define MCXW_CLK_LP_NO_STALL      2U /* Won't prevent low power entry */

/* Combined control modes (commonly used combinations) */
#define MCXW_CLK_MODE_DISABLED            (MCXW_CLK_CTRL_DISABLED)
#define MCXW_CLK_MODE_ENABLED_LP_STALL    (MCXW_CLK_CTRL_ENABLED | MCXW_CLK_LP_STALL_ALLOWED)
#define MCXW_CLK_MODE_ENABLED_LP_NO_STALL (MCXW_CLK_CTRL_ENABLED | MCXW_CLK_LP_NO_STALL)

#define MCXW_CLOCK(mrcc_offset, clk_mux, clk_div, flag)                                            \
	((((mrcc_offset) & 0xFFFF) << 16) | (((clk_mux) & 0xFF) << 8) | (((clk_div) & 0xF) << 4) | \
	 (((flag) & 0xF)))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXW_H_ */
