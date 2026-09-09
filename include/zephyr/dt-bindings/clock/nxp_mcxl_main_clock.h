/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_MAIN_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_MAIN_CLOCK_H_

#include <zephyr/dt-bindings/clock/nxp_mcxl_clock_common.h>

/* MAIN_CLK frequency used during FIRC reconfiguration. */
#define MCXL_MAIN_SIRC_FREQ_HZ DT_FREQ_M(12)

/* FIRC enable mode */
#define MCXL_MAIN_CLK_FIRC_DISABLE      0
#define MCXL_MAIN_CLK_FIRC_ENABLE       1
#define MCXL_MAIN_CLK_FIRC_ENABLE_IN_LP 2

/* MAIN_CLK source index */
#define MCXL_MAIN_CLK_SYS_SRC_SIRC   0
#define MCXL_MAIN_CLK_SYS_SRC_FIRC   1
#define MCXL_MAIN_CLK_SYS_SRC_ROSC   2
#define MCXL_MAIN_CLK_SYS_SRC_PMUIRC 3
#define MCXL_MAIN_CLK_SYS_SRC_LPIRC  4

/* Main-domain gate index */
#define MCXL_MAIN_GATE_NONE          0
#define MCXL_MAIN_GATE_LPUART0       1
#define MCXL_MAIN_GATE_LPUART1       2
#define MCXL_MAIN_GATE_LPI2C0        3
#define MCXL_MAIN_GATE_LPI2C1        4
#define MCXL_MAIN_GATE_LPSPI0        5
#define MCXL_MAIN_GATE_LPSPI1        6
#define MCXL_MAIN_GATE_GPIO1         7
#define MCXL_MAIN_GATE_GPIO2         8
#define MCXL_MAIN_GATE_GPIO3         9
#define MCXL_MAIN_GATE_PORT1         10
#define MCXL_MAIN_GATE_PORT2         11
#define MCXL_MAIN_GATE_PORT3         12
#define MCXL_MAIN_GATE_PERIPH_GROUP0 13
#define MCXL_MAIN_GATE_PERIPH_GROUP1 14
#define MCXL_MAIN_GATE_CRC           15
#define MCXL_MAIN_GATE_DMA0          16
#define MCXL_MAIN_GATE_DMA1          17
#define MCXL_MAIN_GATE_ADC0          18

/* Main-domain selector index */
#define MCXL_MAIN_SEL_NONE          0
#define MCXL_MAIN_SEL_SCGSCS        1
#define MCXL_MAIN_SEL_FIRC          2
#define MCXL_MAIN_SEL_PERIPH_GROUP0 3
#define MCXL_MAIN_SEL_PERIPH_GROUP1 4
#define MCXL_MAIN_SEL_ADC0          5
#define MCXL_MAIN_SEL_CLKOUT        6

/* Placeholder src value when sel_idx = MCXL_MAIN_SEL_NONE (field is ignored). */
#define MCXL_MAIN_CLK_SRC_NONE      0

/* SCGSCS selector src values */
#define MCXL_MAIN_SCGSCS_SRC_SIRC   2
#define MCXL_MAIN_SCGSCS_SRC_FIRC   3
#define MCXL_MAIN_SCGSCS_SRC_ROSC   4
#define MCXL_MAIN_SCGSCS_SRC_PMUIRC 9
#define MCXL_MAIN_SCGSCS_SRC_LPIRC  10

/* FIRC selector src values */
#define MCXL_MAIN_FIRC_SRC_DIRECT 0
#define MCXL_MAIN_FIRC_SRC_DIV    1

/* PERIPH_GROUP0 / PERIPH_GROUP1 selector src values */
#define MCXL_MAIN_PERIPH_GRP_SRC_FRO12M    0
#define MCXL_MAIN_PERIPH_GRP_SRC_XTAL32K   1
#define MCXL_MAIN_PERIPH_GRP_SRC_CLK16K    2
#define MCXL_MAIN_PERIPH_GRP_SRC_FROHF_DIV 3

/* ADC0 selector src values */
#define MCXL_MAIN_ADC0_SRC_FRO12M    0
#define MCXL_MAIN_ADC0_SRC_XTAL32K   1
#define MCXL_MAIN_ADC0_SRC_FROHF_DIV 3

/* CLKOUT selector src values */
#define MCXL_MAIN_CLKOUT_SRC_FRO12M    0
#define MCXL_MAIN_CLKOUT_SRC_SLOW_CLK  1
#define MCXL_MAIN_CLKOUT_SRC_CLK16K    3
#define MCXL_MAIN_CLKOUT_SRC_FRO10M    5
#define MCXL_MAIN_CLKOUT_SRC_FROHF_DIV 7

/* Main-domain divider index */
#define MCXL_MAIN_DIV_NONE          0
#define MCXL_MAIN_DIV_FRO_HF_DIV    1
#define MCXL_MAIN_DIV_AHBCLK        2
#define MCXL_MAIN_DIV_AHBAIPSCLK    3
#define MCXL_MAIN_DIV_PERIPH_GROUP0 4
#define MCXL_MAIN_DIV_PERIPH_GROUP1 5
#define MCXL_MAIN_DIV_ADC0          6

/* Flags */
#define MCXL_MAIN_CLK_FLAG_NONE     MCXL_CLOCK_FLAG_NONE
#define MCXL_MAIN_CLK_FLAG_FIXED    MCXL_CLOCK_FLAG_FIXED_BIT
#define MCXL_MAIN_CLK_FLAG_DIV_HALT MCXL_CLOCK_FLAG_DIV_HALT_BIT

/* Encode a main-domain clock specifier into a single 32-bit DT cell. */
#define MCXL_MAIN_CLOCK(gate_idx, sel_idx, src, div_idx, div_val, flags) \
	MCXL_CLOCK_ENCODE(gate_idx, sel_idx, src, div_idx, div_val, flags)

/* Field extraction wrappers. */
#define MCXL_MAIN_CLOCK_GATE_IDX(spec) MCXL_CLOCK_EXTRACT_GATE_IDX(spec)
#define MCXL_MAIN_CLOCK_SEL_IDX(spec)  MCXL_CLOCK_EXTRACT_SEL_IDX(spec)
#define MCXL_MAIN_CLOCK_SRC(spec)      MCXL_CLOCK_EXTRACT_SRC(spec)
#define MCXL_MAIN_CLOCK_DIV_IDX(spec)  MCXL_CLOCK_EXTRACT_DIV_IDX(spec)
#define MCXL_MAIN_CLOCK_DIV_VAL(spec)  MCXL_CLOCK_EXTRACT_DIV_VAL(spec)
#define MCXL_MAIN_CLOCK_FLAGS(spec)    MCXL_CLOCK_EXTRACT_FLAGS(spec)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_MAIN_CLOCK_H_ */
