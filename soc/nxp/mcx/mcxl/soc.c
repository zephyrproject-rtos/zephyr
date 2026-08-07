/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxl family
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxl family.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

/* SRAM_XEN / SRAM_XEN_DP grant execute permission per SRAM bank. */
#define MCXL_SRAM_XEN_ALL_BANKS                                                 \
	(SYSCON_SRAM_XEN_RAMX0_XEN_MASK | SYSCON_SRAM_XEN_RAMX1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA0_XEN_MASK | SYSCON_SRAM_XEN_RAMA1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA2_XEN_MASK | SYSCON_SRAM_XEN_RAMA3_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB0_XEN_MASK | SYSCON_SRAM_XEN_RAMB1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB2_XEN_MASK | SYSCON_SRAM_XEN_RAMB3_XEN_MASK)

/* GLIKEY index that guards SRAM_XEN_DP, and its "unlocked" SFR_LOCK code. */
#define MCXL_GLIKEY_SRAM_XEN_DP_INDEX 2U
#define MCXL_GLIKEY_SFR_UNLOCK        0xAU

void soc_reset_hook(void)
{
	SystemInit();

	/* The GLIKEY write-enable state machine to unlock SRAM_XEN_DP */
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_SFT_RST(1U) | GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(1U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_WR_EN_1(1U) |
			  GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX);

	/* Enable execute permission for SRAM banks */
	SYSCON->SRAM_XEN = MCXL_SRAM_XEN_ALL_BANKS;
	SYSCON->SRAM_XEN_DP = MCXL_SRAM_XEN_ALL_BANKS;

	__DSB();
	__ISB();
}
