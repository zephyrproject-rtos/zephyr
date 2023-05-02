/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "pinctrl_soc.h"
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a77961.h>

const struct pfc_drive_reg pfc_drive_regs[] = {
	/* DRVCTRL13 */
	{ 0x0334, {
		  { RCAR_GP_PIN(3,  0), 20, 3 },        /* SD0_CLK */
		  { RCAR_GP_PIN(3,  1), 16, 3 },        /* SD0_CMD */
		  { RCAR_GP_PIN(3,  2), 12, 3 },        /* SD0_DAT0 */
		  { RCAR_GP_PIN(3,  3),  8, 3 },        /* SD0_DAT1 */
		  { RCAR_GP_PIN(3,  4),  4, 3 },        /* SD0_DAT2 */
		  { RCAR_GP_PIN(3,  5),  0, 3 },        /* SD0_DAT3 */
	  } },
	/* DRVCTRL14 */
	{ 0x0338, {
		  { RCAR_GP_PIN(3,  6), 28, 3 },        /* SD1_CLK */
		  { RCAR_GP_PIN(3,  7), 24, 3 },        /* SD1_CMD */
		  { RCAR_GP_PIN(3,  8), 20, 3 },        /* SD1_DAT0 */
		  { RCAR_GP_PIN(3,  9), 16, 3 },        /* SD1_DAT1 */
		  { RCAR_GP_PIN(3, 10), 12, 3 },        /* SD1_DAT2 */
		  { RCAR_GP_PIN(3, 11),  8, 3 },        /* SD1_DAT3 */
		  { RCAR_GP_PIN(4,  0),  4, 3 },        /* SD2_CLK */
		  { RCAR_GP_PIN(4,  1),  0, 3 },        /* SD2_CMD */
	  } },
	/* DRVCTRL15 */
	{ 0x033c, {
		  { RCAR_GP_PIN(4,  2), 28, 3 },        /* SD2_DAT0 */
		  { RCAR_GP_PIN(4,  3), 24, 3 },        /* SD2_DAT1 */
		  { RCAR_GP_PIN(4,  4), 20, 3 },        /* SD2_DAT2 */
		  { RCAR_GP_PIN(4,  5), 16, 3 },        /* SD2_DAT3 */
		  { RCAR_GP_PIN(4,  6), 12, 3 },        /* SD2_DS */
		  { RCAR_GP_PIN(4,  7),  8, 3 },        /* SD3_CLK */
		  { RCAR_GP_PIN(4,  8),  4, 3 },        /* SD3_CMD */
		  { RCAR_GP_PIN(4,  9),  0, 3 },        /* SD3_DAT0 */
	  } },
	/* DRVCTRL16 */
	{ 0x0340, {
		  { RCAR_GP_PIN(4, 10), 28, 3 },        /* SD3_DAT1 */
		  { RCAR_GP_PIN(4, 11), 24, 3 },        /* SD3_DAT2 */
		  { RCAR_GP_PIN(4, 12), 20, 3 },        /* SD3_DAT3 */
		  { RCAR_GP_PIN(4, 13), 16, 3 },        /* SD3_DAT4 */
		  { RCAR_GP_PIN(4, 14), 12, 3 },        /* SD3_DAT5 */
		  { RCAR_GP_PIN(4, 15),  8, 3 },        /* SD3_DAT6 */
		  { RCAR_GP_PIN(4, 16),  4, 3 },        /* SD3_DAT7 */
		  { RCAR_GP_PIN(4, 17),  0, 3 },        /* SD3_DS */
	  } },
	/* DRVCTRL17 */
	{ 0x0344, {
		  { RCAR_GP_PIN(3, 12), 28, 3 },        /* SD0_CD */
		  { RCAR_GP_PIN(3, 13), 24, 3 },        /* SD0_WP */
		  { RCAR_GP_PIN(3, 14), 20, 3 },        /* SD1_CD */
		  { RCAR_GP_PIN(3, 15), 16, 3 },        /* SD1_WP */
		  { RCAR_GP_PIN(5,  0), 12, 3 },        /* SCK0 */
		  { RCAR_GP_PIN(5,  1),  8, 3 },        /* RX0 */
		  { RCAR_GP_PIN(5,  2),  4, 3 },        /* TX0 */
		  { RCAR_GP_PIN(5,  3),  0, 3 },        /* CTS0 */
	  } },
	/* DRVCTRL18 */
	{ 0x0348, {
		  { RCAR_GP_PIN(5,  4), 28, 3 },        /* RTS0 */
		  { RCAR_GP_PIN(5,  5), 24, 3 },        /* RX1 */
		  { RCAR_GP_PIN(5,  6), 20, 3 },        /* TX1 */
		  { RCAR_GP_PIN(5,  7), 16, 3 },        /* CTS1 */
		  { RCAR_GP_PIN(5,  8), 12, 3 },        /* RTS1 */
		  { RCAR_GP_PIN(5,  9),  8, 3 },        /* SCK2 */
		  { RCAR_GP_PIN(5, 10),  4, 3 },        /* TX2 */
		  { RCAR_GP_PIN(5, 11),  0, 3 },        /* RX2 */
	  } },
	{ },
};

#define PFC_BIAS_REG(r1, r2) \
	.puen = r1,	     \
	.pud = r2,	     \
	.pins =

const struct pfc_bias_reg pfc_bias_regs[] = {
	{ PFC_BIAS_REG(0x040c, 0x044c) {        /* PUEN3, PUD3 */
		  [0 ... 9]  = PIN_NONE,
		  [10] = RCAR_GP_PIN(3,  0),    /* SD0_CLK */
		  [11] = RCAR_GP_PIN(3,  1),    /* SD0_CMD */
		  [12] = RCAR_GP_PIN(3,  2),    /* SD0_DAT0 */
		  [13] = RCAR_GP_PIN(3,  3),    /* SD0_DAT1 */
		  [14] = RCAR_GP_PIN(3,  4),    /* SD0_DAT2 */
		  [15] = RCAR_GP_PIN(3,  5),    /* SD0_DAT3 */
		  [16] = RCAR_GP_PIN(3,  6),    /* SD1_CLK */
		  [17] = RCAR_GP_PIN(3,  7),    /* SD1_CMD */
		  [18] = RCAR_GP_PIN(3,  8),    /* SD1_DAT0 */
		  [19] = RCAR_GP_PIN(3,  9),    /* SD1_DAT1 */
		  [20] = RCAR_GP_PIN(3, 10),    /* SD1_DAT2 */
		  [21] = RCAR_GP_PIN(3, 11),    /* SD1_DAT3 */
		  [22] = RCAR_GP_PIN(4,  0),    /* SD2_CLK */
		  [23] = RCAR_GP_PIN(4,  1),    /* SD2_CMD */
		  [24] = RCAR_GP_PIN(4,  2),    /* SD2_DAT0 */
		  [25] = RCAR_GP_PIN(4,  3),    /* SD2_DAT1 */
		  [26] = RCAR_GP_PIN(4,  4),    /* SD2_DAT2 */
		  [27] = RCAR_GP_PIN(4,  5),    /* SD2_DAT3 */
		  [28] = RCAR_GP_PIN(4,  6),    /* SD2_DS */
		  [29] = RCAR_GP_PIN(4,  7),    /* SD3_CLK */
		  [30] = RCAR_GP_PIN(4,  8),    /* SD3_CMD */
		  [31] = RCAR_GP_PIN(4,  9),    /* SD3_DAT0 */
	  } },
	{ PFC_BIAS_REG(0x0410, 0x0450) {        /* PUEN4, PUD4 */
		  [0]  = RCAR_GP_PIN(4, 10),    /* SD3_DAT1 */
		  [1]  = RCAR_GP_PIN(4, 11),    /* SD3_DAT2 */
		  [2]  = RCAR_GP_PIN(4, 12),    /* SD3_DAT3 */
		  [3]  = RCAR_GP_PIN(4, 13),    /* SD3_DAT4 */
		  [4]  = RCAR_GP_PIN(4, 14),    /* SD3_DAT5 */
		  [5]  = RCAR_GP_PIN(4, 15),    /* SD3_DAT6 */
		  [6]  = RCAR_GP_PIN(4, 16),    /* SD3_DAT7 */
		  [7]  = RCAR_GP_PIN(4, 17),    /* SD3_DS */
		  [8]  = RCAR_GP_PIN(3, 12),    /* SD0_CD */
		  [9]  = RCAR_GP_PIN(3, 13),    /* SD0_WP */
		  [10] = RCAR_GP_PIN(3, 14),    /* SD1_CD */
		  [11] = RCAR_GP_PIN(3, 15),    /* SD1_WP */
		  [12] = RCAR_GP_PIN(5,  0),    /* SCK0 */
		  [13] = RCAR_GP_PIN(5,  1),    /* RX0 */
		  [14] = RCAR_GP_PIN(5,  2),    /* TX0 */
		  [15] = RCAR_GP_PIN(5,  3),    /* CTS0_N */
		  [16] = RCAR_GP_PIN(5,  4),    /* RTS0_N */
		  [17] = RCAR_GP_PIN(5,  5),    /* RX1_A */
		  [18] = RCAR_GP_PIN(5,  6),    /* TX1_A */
		  [19] = RCAR_GP_PIN(5,  7),    /* CTS1_N */
		  [20] = RCAR_GP_PIN(5,  8),    /* RTS1_N */
		  [21] = RCAR_GP_PIN(5,  9),    /* SCK2 */
		  [22] = RCAR_GP_PIN(5, 10),    /* TX2_A */
		  [23] = RCAR_GP_PIN(5, 11),    /* RX2_A */
		  [24 ... 31]  = PIN_NONE,
	  } },
	{ /* sentinel */ },
};
const struct pfc_bias_reg *pfc_rcar_get_bias_regs(void)
{
	return pfc_bias_regs;
}
const struct pfc_drive_reg *pfc_rcar_get_drive_regs(void)
{
	return pfc_drive_regs;
}
