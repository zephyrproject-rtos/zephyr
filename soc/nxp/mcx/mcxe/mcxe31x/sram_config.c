/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsl_common.h"

/* Don't access system RAM when configuring PRAM FT_DIS.  */
void enable_sram_extra_latency(bool en)
{
	if (en) {
		/* Configure SRAM read wait states. */
		PRAMC_0->PRCR1 |= PRAMC_PRCR1_FT_DIS_MASK;
#if defined(PRAMC_1)
		PRAMC_1->PRCR1 |= PRAMC_PRCR1_FT_DIS_MASK;
#endif
	} else {
		PRAMC_0->PRCR1 &= ~PRAMC_PRCR1_FT_DIS_MASK;
#if defined(PRAMC_1)
		PRAMC_1->PRCR1 &= ~PRAMC_PRCR1_FT_DIS_MASK;
#endif
	}
}
