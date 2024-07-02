/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "soc.h"
#include "fsl_mu.h"
#include "smt.h"

void soc_sm_init(void)
{
	MU_Type * const s_muBases[] = MU_BASE_PTRS;
	MU_Type *base = s_muBases[SM_MU_INSTANCE];

	/* Configure SMT */
	SMT_ChannelConfig(SM_A2P, SM_MU_INSTANCE, SM_DBIR_A2P, SM_SMA_ADDR);

	/* Configure MU */
	MU_Init(base);

}

static int soc_init(void)
{
	soc_sm_init();

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 0);
