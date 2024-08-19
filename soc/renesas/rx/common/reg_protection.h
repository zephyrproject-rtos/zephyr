/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SOC_RENESAS_REG_PROTECTION_H_
#define ZEPHYR_INCLUDE_SOC_RENESAS_REG_PROTECTION_H_

typedef enum {
	/*
	 * PRC0
	 * Enables writing to the registers related to the clock generation circuit: SCKCR, SCKCR3,
	 * PLLCR, PLLCR2, MOSCCR, SOSCCR, LOCOCR, ILOCOCR, HOCOCR, HOFCR, OSTDCR, OSTDSR, CKOCR,
	 * LOCOTRR, ILOCOTRR, HOCOTRR0.
	 */
	RENESAS_RX_REG_PROTECT_CGC = 0,

	/*
	 * PRC1
	 * Enables writing to the registers related to operating modes, low power consumption,
	 * the clock generation circuit, and software reset: SYSCR1, SBYCR, MSTPCRA, MSTPCRB,
	 * MSTPCRC, MSTPCRD, OPCCR, RSTCKCR, SOPCCR, MOFCR, MOSCWTCR, SWRR.
	 */
	RENESAS_RX_REG_PROTECT_LPC_CGC_SWR,

	/*
	 * PRC2
	 * Enables writing to the registers related to the LPT: LPTCR1, LPTCR2, LPTCR3, LPTPRD,
	 * LPCMR0, LPWUCR.
	 */
	RENESAS_RX_REG_PROTECT_LPT,

	/*
	 * PRC3
	 * Enables writing to the registers related to the LVD: LVCMPCR, LVDLVLR, LVD1CR0, LVD1CR1,
	 * LVD1SR, LVD2CR0, LVD2CR1, LVD2SR.
	 */
	RENESAS_RX_REG_PROTECT_LVD,

	/*
	 * MPC.PWPR
	 * Enables writing to MPC's PFS registers.
	 */
	RENESAS_RX_REG_PROTECT_MPC,

	/*
	 * This entry is used for getting the number of enum items. This must be the last entry. DO
	 * NOT REMOVE THIS ENTRY!
	 */
	RENESAS_RX_REG_PROTECT_TOTAL_ITEMS
} renesas_rx_reg_protect_t;

void renesas_rx_register_protect_open(void);
void renesas_rx_register_protect_enable(renesas_rx_reg_protect_t regs_to_protect);
void renesas_rx_register_protect_disable(renesas_rx_reg_protect_t regs_to_unprotect);

#endif /* ZEPHYR_INCLUDE_SOC_RENESAS_REG_PROTECTION_H_ */
