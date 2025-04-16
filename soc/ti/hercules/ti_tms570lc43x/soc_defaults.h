/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TMS570_SOC_PRIVATE_H__
#define __TMS570_SOC_PRIVATE_H__

#define OSC_IN_FREQ_MHZ		DT_PROP(DT_NODELABEL(osc_in), clock_frequency) / 1000000

/**
 * - BPOS = 1, Bypass on PLL Slip is enabled
 * - PLLDIV = 31 (R = 32)
 * - REFCLKDIV = OSC_IN_FREQ_MHZ - 1 (ref clock divider)
 * - PLLMUL = 0x9500 (NF, multiplication factor = 0x95+1 = 150)
 *
 *   NOTE: the divider value is changed later on, to be 0 (R=1)
 */
#define PLLCTL1_INIT_VALUE	(1 << PLLCTL1_BPOS_OFFSET | \
	31 << PLLCTL1_PLLDIV_OFFSET | \
	(OSC_IN_FREQ_MHZ - 1) << PLLCTL1_REFCLKDIV_OFFSET | \
	0x9500)

/**
 *- SPREADINGRATE = 255 (NS = 256)
 *- MULMOD = 7
 *- ODPLL = 0 (OD, output divider = 0)
 *- SPR_AMOUNT = 61 (NV = 62/2048)
 */
#define PLLCTL2_INIT_VALUE	(255 << PLLCTL2_SPREADINGRATE_OFFSET | \
	7 << PLLCTL2_MULMOD_OFFSET | \
	61 << PLLCTL2_SPR_AMOUNT_OFFSET)

/**
 *- ODPLL2 = 0 (OD2 = 1)
 *- PLLDIV2 = 31 (R2 = 32)
 *- REFCLKDIV2 = 7 (NR2 = 8)
 *- PLLMUL2 = 0x9500 (NF2 = 150)
 */
#define PLLCTL3_INIT_VALUE	(31 << PLLCTL3_PLLDIV2_OFFSET | \
	7 << PLLCTL3_REFCLKDIV2_OFFSET | \
	0x9500 << PLLCTL3_PLLMUL2_OFFSET)

/**
 *- RWAIT = 3 Random/data Read Wait State
 *- PFUENB = 1 Prefetch Enable for Port B
 *- PFUENA = 1 Prefetch Enable for Port A
 */
#define FRDCNTL_INIT_VALUE	(3 << FRDCNTL_RWAIT_OFFSET | FRDCNTL_PFUENA | FRDCNTL_PFUENB)

/**
 * - EEPROM Wait state Counter = 9
 */
#define EEPROM_CONFIG_INIT_VALUE (9 << EWAIT_OFFSET)

#define FBPWRMODE_INIT_VALUE	(BANKPWR_VAL_ACTIVE << BANKPWR0_OFFSET | \
	BANKPWR_VAL_ACTIVE << BANKPWR1_OFFSET | \
	BANKPWR_VAL_ACTIVE << BANKPWR7_OFFSET)

/* GCLK, HCLK and VCLK source is PLL1 */
#define GHVSRC_INIT_VALUE	(1 << GHVWAKE_OFFSET |  1 << HVLPM_OFFSET |  1 << GHVSRC_OFFSET)

/**
 * - RTI1DIV = 0, divider = 1 (NOTE: divider is bypassed when VCLK is the source)
 * - RTI1SRC = 9, VCLK as source, 8 to 0xF is VCLK
 */
#define RCLKSRC_INIT_VALUE	(0 << RTI1DIV_OFFSET | 9 << RTI1SRC_OFFSET)

/**
 *- VCLKA1S = 9, VCLK is source
 *- VCLKA2S = 9, VCLK is source
 */
#define VCLKASRC_INIT_VALUE	(9 << VCLKA1S_OFFSET | 9 << VCLKA2S_OFFSET)

/*
 * - VCLKA4R = 0, divider=1
 * - VCLKA4_DIV_CDDIS = 0, Disable the VCLKA4 divider output
 * - VCLKA4S = 9, source=VCLK
 */
#define VCLKACON1_INIT_VALUE	(0 << VCLKA4R_OFFSET | VCLKA4_DIV_CDDIS | 9 << VCLKA4S_OFFSET)

#endif /* __TMS570_SOC_PRIVATE_H__ */
