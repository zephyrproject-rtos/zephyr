/* Copyright(c) 2021 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/arch/xtensa/cache.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <cpu_init.h>
#include "manifest.h"


#define DELAY_COUNT			256
#define LPSRAM_MASK(x)		0x00000003
#define SRAM_BANK_SIZE		(64 * 1024)
#define EBB_SEGMENT_SIZE	32
#define PLATFORM_INIT_HPSRAM
#define PLATFORM_INIT_LPSRAM

#define PLATFORM_HPSRAM_EBB_COUNT (DT_REG_SIZE(DT_NODELABEL(sram0)) / SRAM_BANK_SIZE)
BUILD_ASSERT((DT_REG_SIZE(DT_NODELABEL(sram0)) % SRAM_BANK_SIZE) == 0,
		"sram0 must be divisible by 64*1024 bank size.");

/*
 * Function powers up a number of memory banks provided as an argument
 * and gates remaining memory banks
 */
static __imr void hp_sram_pm_banks(uint32_t banks)
{
#ifdef CONFIG_ADSP_INIT_HPSRAM
	uint32_t status, ebb_mask0, ebb_mask1, ebb_avail_mask0, ebb_avail_mask1,
		total_banks_count = PLATFORM_HPSRAM_EBB_COUNT;

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_HPSRAM_LDO_ON;

	/* Add some delay before touch power register */
	z_idelay(DELAY_COUNT);

	/*
	 * bit masks reflect total number of available EBB (banks) in each
	 * segment; current implementation supports 2 segments 0,1
	 */
	if (total_banks_count > EBB_SEGMENT_SIZE) {
		ebb_avail_mask0 = (uint32_t)GENMASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_avail_mask1 = (uint32_t)GENMASK(total_banks_count -
						    EBB_SEGMENT_SIZE - 1, 0);
	} else {
		ebb_avail_mask0 = (uint32_t)GENMASK(total_banks_count - 1, 0);
		ebb_avail_mask1 = 0;
	}

	/* bit masks of banks that have to be powered up in each segment */
	if (banks > EBB_SEGMENT_SIZE) {
		ebb_mask0 = (uint32_t)GENMASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_mask1 = (uint32_t)GENMASK(banks - EBB_SEGMENT_SIZE - 1,
		0);
	} else {
		/* assumption that ebb_in_use is > 0 */
		ebb_mask0 = (uint32_t)GENMASK(banks - 1, 0);
		ebb_mask1 = 0;
	}

	/* HSPGCTL, HSRMCTL use reverse logic - 0 means EBB is power gated */
	CAVS_L2LM.hspgctl0 = (~ebb_mask0) & ebb_avail_mask0;
	CAVS_L2LM.hsrmctl0 = (~ebb_mask0) & ebb_avail_mask0;
	CAVS_L2LM.hspgctl1 = (~ebb_mask1) & ebb_avail_mask1;
	CAVS_L2LM.hsrmctl1 = (~ebb_mask1) & ebb_avail_mask1;

	/*
	 * Query the power status of first part of HP memory
	 * to check whether it has been powered up. A few
	 * cycles are needed for it to be powered up
	 */
	status = CAVS_L2LM.hspgists0;
	while (status != ((~ebb_mask0) & ebb_avail_mask0)) {
		z_idelay(DELAY_COUNT);
		status = CAVS_L2LM.hspgists0;
	}

	/*
	 * Query the power status of second part of HP memory
	 * and do as above code
	 */
	status = CAVS_L2LM.hspgists1;

	while (status != ((~ebb_mask1) & ebb_avail_mask1)) {
		z_idelay(DELAY_COUNT);
		status = CAVS_L2LM.hspgists1;
	}

	/* Add some delay before touch power register */
	z_idelay(DELAY_COUNT);

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_HPSRAM_LDO_BYPASS;
#endif
}

__imr void hp_sram_init(uint32_t memory_size)
{
	uint32_t ebb_in_use;

	/*
	 * Calculate total number of used SRAM banks (EBB)
	 * to power up only necessary banks
	 */
	ebb_in_use = ceiling_fraction(memory_size, SRAM_BANK_SIZE);

	hp_sram_pm_banks(ebb_in_use);

	bbzero((void *)L2_SRAM_BASE, L2_SRAM_SIZE);
}

__imr void lp_sram_init(void)
{
#ifdef PLATFORM_INIT_LPSRAM
	uint32_t timeout_counter;

	timeout_counter = DELAY_COUNT;

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_LPSRAM_LDO_ON;

	/* Add some delay before writing power registers */
	z_idelay(DELAY_COUNT);

	/* FIXME */
	CAVS_L2LM.lspgctl = CAVS_L2LM.lspgists & ~LPSRAM_MASK(0);

	/* Add some delay before checking the status */
	z_idelay(DELAY_COUNT);

	/*
	 * Query the power status of first part of LP memory
	 * to check whether it has been powered up. A few
	 * cycles are needed for it to be powered up
	 */
	/* FIXME */
	while (CAVS_L2LM.lspgists && timeout_counter--) {
		z_idelay(DELAY_COUNT);
	}

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_LPSRAM_LDO_BYPASS;
	bbzero((void *)LP_SRAM_BASE, LP_SRAM_SIZE);
#endif
}
