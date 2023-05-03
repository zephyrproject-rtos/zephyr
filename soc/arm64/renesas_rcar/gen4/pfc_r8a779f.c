/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "pinctrl_soc.h"
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a779f.h>

const struct pfc_drive_reg pfc_drive_regs[] = {
	/* DRV1CTRL1 */
	{ 0x884, {
		  { PIN_MMC_SD_D2,  28, 3 },
		  { PIN_MMC_SD_D1,  24, 3 },
		  { PIN_MMC_SD_D0,  20, 3 },
		  { PIN_MMC_SD_CLK, 16, 3 },
	  } },
	/* DRV2CTRL1 */
	{ 0x888, {
		  { PIN_SD_CD,      28, 3 },
		  { PIN_MMC_SD_CMD, 24, 3 },
		  { PIN_MMC_D7,     20, 3 },
		  { PIN_MMC_DS,     16, 3 },
		  { PIN_MMC_D6,     12, 3 },
		  { PIN_MMC_D4,      8, 3 },
		  { PIN_MMC_D5,      4, 3 },
		  { PIN_MMC_SD_D3,   0, 3 },
	  } },
	/* DRV3CTRL1 */
	{ 0x88c, {
		  { PIN_SD_WP, 0, 3 },
	  } },
	{ },
};

#define PFC_BIAS_REG(r1, r2) \
	.puen = r1,	     \
	.pud = r2,	     \
	.pins =

const struct pfc_bias_reg pfc_bias_regs[] = {
	/* PUEN1, PUD1 */
	{ PFC_BIAS_REG(0x8c0, 0x8e0) {
		  [0 ... 11] = PIN_NONE,
		  [12] = PIN_MMC_SD_CLK,
		  [13] = PIN_MMC_SD_D0,
		  [14] = PIN_MMC_SD_D1,
		  [15] = PIN_MMC_SD_D2,
		  [16] = PIN_MMC_SD_D3,
		  [17] = PIN_MMC_D5,
		  [18] = PIN_MMC_D4,
		  [19] = PIN_MMC_D6,
		  [20] = PIN_MMC_DS,
		  [21] = PIN_MMC_D7,
		  [22] = PIN_MMC_SD_CMD,
		  [23] = PIN_SD_CD,
		  [24] = PIN_SD_WP,
		  [25 ... 31] = PIN_NONE,
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
