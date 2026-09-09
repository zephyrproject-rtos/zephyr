/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_AON_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_AON_CLOCK_H_

#include <zephyr/dt-bindings/clock/nxp_mcxl_clock_common.h>

/* AON gate index */
#define MCXL_AON_GATE_NONE     0
#define MCXL_AON_GATE_APB      1
#define MCXL_AON_GATE_UART     2
#define MCXL_AON_GATE_I2C      3
#define MCXL_AON_GATE_PORT     4
#define MCXL_AON_GATE_GPIO     5
#define MCXL_AON_GATE_QTMR0    6
#define MCXL_AON_GATE_QTMR1    7
#define MCXL_AON_GATE_LPTMR    8
#define MCXL_AON_GATE_LPADC    9
#define MCXL_AON_GATE_SYS      10
#define MCXL_AON_GATE_ROOT_AUX 11

/* AON ROOT source index */
#define MCXL_AON_CLK_ROOT_SRC_NONE     0
#define MCXL_AON_CLK_ROOT_SRC_FRODIV1  1
#define MCXL_AON_CLK_ROOT_SRC_FRODIV2  2
#define MCXL_AON_CLK_ROOT_SRC_FRODIV4  3
#define MCXL_AON_CLK_ROOT_SRC_ROOT_AUX 4
#define MCXL_AON_CLK_ROOT_SRC_XTAL32K  5

/* AON selector index */
#define MCXL_AON_SEL_NONE     0
#define MCXL_AON_SEL_ROOT     1
#define MCXL_AON_SEL_ROOT_AUX 2
#define MCXL_AON_SEL_COM      3
#define MCXL_AON_SEL_TMR      4
#define MCXL_AON_SEL_LPADC    5

/* AON ROOT_AUX selector src values */
#define MCXL_AON_ROOT_AUX_SRC_XTAL32K 0
#define MCXL_AON_ROOT_AUX_SRC_AUX     1

/* AON COM selector src values */
#define MCXL_AON_COM_SRC_FRODIV1  0
#define MCXL_AON_COM_SRC_FRODIV2  1
#define MCXL_AON_COM_SRC_FRODIV4  2
#define MCXL_AON_COM_SRC_ROOT_AUX 3

/* AON TMR selector src values */
#define MCXL_AON_TMR_SRC_FRODIV1  0
#define MCXL_AON_TMR_SRC_FRODIV2  1
#define MCXL_AON_TMR_SRC_FRODIV4  2
#define MCXL_AON_TMR_SRC_ROOT_AUX 3

/* AON LPADC selector src values */
#define MCXL_AON_LPADC_SRC_FRODIV1  0
#define MCXL_AON_LPADC_SRC_FRODIV2  1
#define MCXL_AON_LPADC_SRC_FRODIV4  2
#define MCXL_AON_LPADC_SRC_ROOT_AUX 3
#define MCXL_AON_LPADC_SRC_XTAL32K  4
#define MCXL_AON_LPADC_SRC_FRO16K   5

/* AON divider index */
#define MCXL_AON_DIV_NONE 0
#define MCXL_AON_DIV_CPU  1
#define MCXL_AON_DIV_CMP  2
#define MCXL_AON_DIV_SYS  3

/* Flags (aliases of the shared bits). */
#define MCXL_AON_CLK_FLAG_NONE     MCXL_CLOCK_FLAG_NONE
#define MCXL_AON_CLK_FLAG_FIXED    MCXL_CLOCK_FLAG_FIXED_BIT
#define MCXL_AON_CLK_FLAG_DIV_HALT MCXL_CLOCK_FLAG_DIV_HALT_BIT

/* Encode an AON clock specifier into a single 32-bit DT cell. */
#define MCXL_AON_CLOCK(gate_idx, sel_idx, src, div_idx, div_val, flags) \
	MCXL_CLOCK_ENCODE(gate_idx, sel_idx, src, div_idx, div_val, flags)

/* Field extraction wrappers. */
#define MCXL_AON_CLOCK_GATE_IDX(spec) MCXL_CLOCK_EXTRACT_GATE_IDX(spec)
#define MCXL_AON_CLOCK_SEL_IDX(spec)  MCXL_CLOCK_EXTRACT_SEL_IDX(spec)
#define MCXL_AON_CLOCK_SRC(spec)      MCXL_CLOCK_EXTRACT_SRC(spec)
#define MCXL_AON_CLOCK_DIV_IDX(spec)  MCXL_CLOCK_EXTRACT_DIV_IDX(spec)
#define MCXL_AON_CLOCK_DIV_VAL(spec)  MCXL_CLOCK_EXTRACT_DIV_VAL(spec)
#define MCXL_AON_CLOCK_FLAGS(spec)    MCXL_CLOCK_EXTRACT_FLAGS(spec)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCXL_AON_CLOCK_H_ */
