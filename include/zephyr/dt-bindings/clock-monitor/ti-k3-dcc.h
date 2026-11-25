/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_TI_K3_DCC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_TI_K3_DCC_H_

/**
 * @file
 * @brief CLKSRC mux cookies for the TI K3 Dual Clock Comparator (DCC).
 *
 * Numeric values are the DCCCLKSRC0 / DCCCLKSRC1 field encodings from the
 * SoC TRM input-source mapping tables. Reference sources use CLKSRC0 values;
 * target sources use CLKSRC1 values. The two axes are looked up in separate
 * DT source lists, so overlapping integers are fine.
 *
 * Macros below cover the MCU_DCC0 mapping on J722S / AM67 (SPRUJB3D
 * Table 4-406). Other DCC instances may reuse the same numbers for
 * different clocks; declare only the sources that apply to the instance.
 */

/** set_source() sentinel: leave that axis unchanged. */
#define K3_DCC_SOURCE_UNCHANGED 0xffffffffU

/* CLKSRC0 (counter 0 / reference) — MCU_DCC0 */
#define K3_DCC_CLK0_HFOSC0        0
#define K3_DCC_CLK0_CLK_32K_RC    1
#define K3_DCC_CLK0_CLK_12M_RC    2
#define K3_DCC_CLK0_MCU_SYSCLK0_4 3

/* CLKSRC1 (counter 1 / target) — MCU_DCC0 */
#define K3_DCC_CLK1_MCU_SYSCLK0_2      0
#define K3_DCC_CLK1_MCU_PLL0_HSDIV0    1
#define K3_DCC_CLK1_MCU_PLL0_HSDIV1    2
#define K3_DCC_CLK1_MCU_PLL0_HSDIV2    3
#define K3_DCC_CLK1_MCU_PLL0_HSDIV3_4  4
#define K3_DCC_CLK1_MCU_PLL0_HSDIV4    5
#define K3_DCC_CLK1_CLK_32K_RC         6
#define K3_DCC_CLK1_HFOSC0             7
#define K3_DCC_CLK1_MCU_EXT_REFCLK0    8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_TI_K3_DCC_H_ */
