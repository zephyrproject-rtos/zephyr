/* * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include "soc.h"
#include "fsl_mu.h"
#include "smt.h"

void soc_sm_init(void)
{
	/* Configure SMT */
	SMT_ChannelConfig(SM_A2P, SM_MU_INSTANCE, SM_DBIR_A2P, SM_SMA_ADDR);

	/* Configure MU */
	MU_Init(SM_MU_PTR);
}

static int soc_init(void)
{
	soc_sm_init();
	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 0);
