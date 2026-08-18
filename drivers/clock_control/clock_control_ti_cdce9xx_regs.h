/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_REGS_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_REGS_H_

#include <zephyr/sys/util.h>

/*
 * CDCE9xx / CDCEL9xx register offsets
 *
 * Generic configuration: 0x00 - 0x0f
 * PLLx configuration:    0xx0 - 0xxf
 */

#define CDCE9XX_REG_GENERIC_BASE 0x00

#define CDCE9XX_VENDOR_ID 0x01

/* -------------------------------------------------------------------------- */
/* Generic configuration registers                                           */
/* -------------------------------------------------------------------------- */

#define CDCE9XX_REG_ID 0x00

#define CDCE9XX_ID_E_EL BIT(7)
#define CDCE9XX_ID_RID  GENMASK(6, 4)
#define CDCE9XX_ID_VID  GENMASK(3, 0)

#define CDCE9XX_REG_CTRL 0x01

#define CDCE9XX_CTRL_EEPIP     BIT(6)
#define CDCE9XX_CTRL_EELOCK    BIT(5)
#define CDCE9XX_CTRL_PWDN      BIT(4)
#define CDCE9XX_CTRL_INCLK     GENMASK(3, 2)
#define CDCE9XX_CTRL_SLAVE_ADR GENMASK(1, 0)

#define CDCE9XX_INCLK_XTAL   0
#define CDCE9XX_INCLK_VCXO   1
#define CDCE9XX_INCLK_LVCMOS 2

#define CDCE9XX_REG_Y1_CTRL 0x02

#define CDCE9XX_Y1_CTRL_M1      BIT(7)
#define CDCE9XX_Y1_CTRL_SPICON  BIT(6)
#define CDCE9XX_Y1_CTRL_ST1     GENMASK(5, 4)
#define CDCE9XX_Y1_CTRL_ST0     GENMASK(3, 2)
#define CDCE9XX_Y1_CTRL_PDIV1_H GENMASK(1, 0)

#define CDCE9XX_Y1_SOURCE_INPUT 0
#define CDCE9XX_Y1_SOURCE_PLL1  1

#define CDCE9XX_OUTPUT_STATE_PD      0
#define CDCE9XX_OUTPUT_STATE_HIZ     1
#define CDCE9XX_OUTPUT_STATE_LOW     2
#define CDCE9XX_OUTPUT_STATE_ENABLED 3

#define CDCE9XX_REG_PDIV1_L 0x03

#define CDCE9XX_PDIV1_L GENMASK(7, 0)

#define CDCE9XX_REG_Y1_STATE 0x04

#define CDCE9XX_Y1_STATE_SEL BIT(7)
#define CDCE9XX_Y1_STATE_7   BIT(6)
#define CDCE9XX_Y1_STATE_6   BIT(5)
#define CDCE9XX_Y1_STATE_5   BIT(4)
#define CDCE9XX_Y1_STATE_4   BIT(3)
#define CDCE9XX_Y1_STATE_3   BIT(2)
#define CDCE9XX_Y1_STATE_2   BIT(1)
#define CDCE9XX_Y1_STATE_1   BIT(0)

#define CDCE9XX_REG_XCSEL 0x05

#define CDCE9XX_XCSEL     GENMASK(7, 3)
#define CDCE9XX_XCSEL_MAX 0x14

#define CDCE9XX_REG_BCOUNT_EEWRITE 0x06

#define CDCE9XX_BCOUNT  GENMASK(7, 1)
#define CDCE9XX_EEWRITE BIT(0)

/* -------------------------------------------------------------------------- */
/* PLL configuration                                                        */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL1_REG_START 0x10
#define CDCE9XX_PLL2_REG_START 0x20
#define CDCE9XX_PLL3_REG_START 0x30
#define CDCE9XX_PLL4_REG_START 0x40

#define CDCE9XX_REG_PLL_SSC_7_OFFSET 0x00

#define CDCE9XX_PLL_SSC_7   GENMASK(7, 5)
#define CDCE9XX_PLL_SSC_6   GENMASK(4, 2)
#define CDCE9XX_PLL_SSC_5_H GENMASK(1, 0)

#define CDCE9XX_REG_PLL_SSC_5_OFFSET 0x01

#define CDCE9XX_PLL_SSC_5_L BIT(7)
#define CDCE9XX_PLL_SSC_4   GENMASK(6, 4)
#define CDCE9XX_PLL_SSC_3   GENMASK(3, 1)
#define CDCE9XX_PLL_SSC_2_H BIT(0)

#define CDCE9XX_REG_PLL_SSC_2_OFFSET 0x02

#define CDCE9XX_PLL_SSC_2_L GENMASK(7, 6)
#define CDCE9XX_PLL_SSC_1   GENMASK(5, 3)
#define CDCE9XX_PLL_SSC_0   GENMASK(2, 0)

#define CDCE9XX_REG_PLL_FREQ_SEL_OFFSET 0x03

#define CDCE9XX_PLL_FS GENMASK(7, 0)

#define CDCE9XX_REG_PLL_MUX_OFFSET 0x04

#define CDCE9XX_PLL_MUX        BIT(7)
#define CDCE9XX_FIRST_OUT_MUX  BIT(6)
#define CDCE9XX_SECOND_OUT_MUX GENMASK(5, 4)
#define CDCE9XX_PLL_ST1        GENMASK(3, 2)
#define CDCE9XX_PLL_ST0        GENMASK(1, 0)

#define CDCE9XX_REG_PLL_OUTPUT_STATE_OFFSET 0x05

#define CDCE9XX_REG_PLL_FIRST_PDIV_OFFSET 0x06

#define CDCE9XX_PLL_SSC_DC     BIT(7)
#define CDCE9XX_PLL_FIRST_PDIV GENMASK(6, 0)

#define CDCE9XX_REG_PLL_SECOND_PDIV_OFFSET 0x07

#define CDCE9XX_PLL_SECOND_PDIV GENMASK(6, 0)

/* -------------------------------------------------------------------------- */
/* PLL frequency                                                              */
/* 30-bit PLL value consists of:                                              */
/* N[11:0]                                                                    */
/* R[8:0]                                                                     */
/* Q[5:0]                                                                     */
/* P[2:0]                                                                     */
/* VCO range[1:0]                                                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* PLL frequency 0 register                                                   */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_REG_PLL_0_N_11_4_OFFSET      0x08
#define CDCE9XX_REG_PLL_0_N_3_0_R_8_5_OFFSET 0x09
#define CDCE9XX_REG_PLL_0_R_4_0_Q_5_3_OFFSET 0x0a
#define CDCE9XX_REG_PLL_0_Q_2_0_P_OFFSET     0x0b

/* -------------------------------------------------------------------------- */
/* PLL frequency 1 register                                                   */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_REG_PLL_1_N_11_4_OFFSET      0x0c
#define CDCE9XX_REG_PLL_1_N_3_0_R_8_5_OFFSET 0x0d
#define CDCE9XX_REG_PLL_1_R_4_0_Q_5_3_OFFSET 0x0e
#define CDCE9XX_REG_PLL_1_Q_2_0_P_OFFSET     0x0f

/* -------------------------------------------------------------------------- */
/* N[11:0]                                                                    */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL_N_11_4 GENMASK(7, 0)
#define CDCE9XX_PLL_N_3_0  GENMASK(7, 4)

/* -------------------------------------------------------------------------- */
/* R[8:0]                                                                     */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL_R_8_5 GENMASK(3, 0)
#define CDCE9XX_PLL_R_4_0 GENMASK(7, 3)

/* -------------------------------------------------------------------------- */
/* Q[5:0]                                                                     */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL_Q_5_3 GENMASK(2, 0)
#define CDCE9XX_PLL_Q_2_0 GENMASK(7, 5)

/* -------------------------------------------------------------------------- */
/* P[2:0]                                                                     */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL_P GENMASK(4, 2)

/* -------------------------------------------------------------------------- */
/* VCO range[1:0]                                                             */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_PLL_VCO_RANGE GENMASK(1, 0)

#define CDCE9XX_VCO_RANGE_125 0
#define CDCE9XX_VCO_RANGE_150 1
#define CDCE9XX_VCO_RANGE_175 2
#define CDCE9XX_VCO_RANGE_MAX 3

/* -------------------------------------------------------------------------- */
/* I2C command code                                                           */
/* -------------------------------------------------------------------------- */
#define CDCE9XX_CMD_BLOCK 0
#define CDCE9XX_CMD_BYTE  BIT(7)

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_TI_CDCE9XX_REGS_H_ */
