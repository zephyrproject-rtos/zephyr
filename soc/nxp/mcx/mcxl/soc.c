/*
 * Copyright 2025-2026 NXP
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

/* GLIKEY SFR_LOCK code that unlocks write access to a guarded register group. */
#define MCXL_GLIKEY_SFR_UNLOCK 0xAU

/* GLIKEY index assignments. */
#define MCXL_GLIKEY_SRAM_XEN_DP_INDEX 2U  /* guards SYSCON SRAM_XEN_DP */
#define MCXL_GLIKEY_MBC0_INDEX        15U /* guards MBC0 (TRDC) registers */

/*
 * glikey_write_enable - run the GLIKEY state machine to grant write access
 * to the register group identified by @index.
 */
static void glikey_write_enable(uint32_t index)
{
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_SFT_RST(1U) | GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(index) | GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(index) | GLIKEY_CTRL_0_WR_EN_0(1U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_WR_EN_1(1U) |
			  GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(index) | GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(index);
}

/*
 * glikey_write_disable - close the GLIKEY write window for @index.
 */
static void glikey_write_disable(uint32_t index)
{
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(index) | GLIKEY_CTRL_0_WR_EN_0(2U);
}

/* SRAM_XEN / SRAM_XEN_DP grant execute permission per SRAM bank. */
#define MCXL_SRAM_XEN_ALL_BANKS                                                 \
	(SYSCON_SRAM_XEN_RAMX0_XEN_MASK | SYSCON_SRAM_XEN_RAMX1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA0_XEN_MASK | SYSCON_SRAM_XEN_RAMA1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA2_XEN_MASK | SYSCON_SRAM_XEN_RAMA3_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB0_XEN_MASK | SYSCON_SRAM_XEN_RAMB1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB2_XEN_MASK | SYSCON_SRAM_XEN_RAMB3_XEN_MASK)

#if defined(CONFIG_NXP_MCXL_FLASH_ACCESS_UNRESTRICTED)
/*
 * TRDC MBC GLBAC value that grants full Read/Write/Execute to all security
 * levels (Secure Priv/User and NonSecure Priv/User).
 *
 * Bit layout per MBC_MEMN_GLBAC:
 *   [0]  NUX, [1]  NUW, [2]  NUR  - NonSecure User  X/W/R
 *   [4]  NPX, [5]  NPW, [6]  NPR  - NonSecure Priv  X/W/R
 *   [8]  SUX, [9]  SUW, [10] SUR  - Secure User     X/W/R
 *   [12] SPX, [13] SPW, [14] SPR  - Secure Priv     X/W/R
 */
#define MCXL_MBC_GLBAC_ALL_RWX 0x7777U

/*
 * MEM0 of the single MBC covers flash; 8 BLK_CFG_W words control all blocks.
 */
#define MCXL_MBC_MEM0_BLK_CFG_W_COUNT 8U

/*
 * Set up TRDC MBC flash access control so that flash is fully accessible
 * (read, write, execute) for all bus masters and security levels.
 *
 * The ROM/bootloader may leave flash blocks configured with a restrictive
 * access policy (GLBAC index that denies writes).  This routine programs
 * GLBAC0 with unrestricted RWX for every security level and then points all
 * flash blocks (MEM0, 8 CFG words x 8 blocks = 64 blocks) at GLBAC0 with
 * NSE=1 so that both secure and non-secure accesses are permitted.
 *
 * BLK_CFG_W word layout (4 bits per block slot, 8 slots per 32-bit word):
 *   bits [2:0] = MBACxSEL (0 -> GLBAC0)
 *   bit  [3]   = NSE      (1 -> block is non-secure; uses NS* bits in GLBAC)
 * All eight slots set to MBACSEL=0/NSE=1 -> 0x88888888.
 */
static void mcxl_flash_access_unrestricted(void)
{
	uint32_t i;

	/* Grant full RWX for all security levels via GLBAC0. */
	MBC0->MBC_INDEX[0].MBC_MEMN_GLBAC[0] = MCXL_MBC_GLBAC_ALL_RWX;

	/* Point every flash block at GLBAC0 and mark it non-secure. */
	for (i = 0U; i < MCXL_MBC_MEM0_BLK_CFG_W_COUNT; i++) {
		MBC0->MBC_INDEX[0].MBC_DOM0_MEM0_BLK_CFG_W[i] = 0x88888888U;
	}
}
#endif /* CONFIG_NXP_MCXL_FLASH_ACCESS_UNRESTRICTED */

void soc_reset_hook(void)
{
	SystemInit();

#if defined(CONFIG_NXP_MCXL_FLASH_ACCESS_UNRESTRICTED)
	/* Open flash access before any flash read/write/execute operations. */
	glikey_write_enable(MCXL_GLIKEY_MBC0_INDEX);
	mcxl_flash_access_unrestricted();
	glikey_write_disable(MCXL_GLIKEY_MBC0_INDEX);
#endif

	/* Enable execute permission for SRAM banks. */
	glikey_write_enable(MCXL_GLIKEY_SRAM_XEN_DP_INDEX);
	SYSCON->SRAM_XEN = MCXL_SRAM_XEN_ALL_BANKS;
	SYSCON->SRAM_XEN_DP = MCXL_SRAM_XEN_ALL_BANKS;
	glikey_write_disable(MCXL_GLIKEY_SRAM_XEN_DP_INDEX);

	__DSB();
	__ISB();
}
